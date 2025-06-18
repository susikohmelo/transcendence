"use client"; // Required for Next.js 13+ to use browser APIs

import { useEffect, useRef, useState } from "react";
import {
	Engine,
	Scene,
	Effect,
	ShaderMaterial,
	MeshBuilder,
	Mesh,
	Vector2,
	Vector3,
	FreeCamera,
} from "@babylonjs/core";
import { sendMsg, getSession } from "@/lib/session";
import { useGameState, useLobby } from "@/context/sessionhandler";

const TIME_START = Date.now();

export default function GameCanvas() {
	const session = getSession();
	const { splitScreen, playerState } = useLobby();
	const gameState = useGameState();

	const pressedKeysRef = useRef(new Set<string>());
	const [mouseX, setMouseX] = useState<number>();
	const [mouseY, setMouseY] = useState<number>();

	const canvasRef = useRef<HTMLCanvasElement>(null);
	const shaderMaterialRef = useRef<ShaderMaterial | null>(null);
	const fragmentShaderRef = useRef<string | null>(null);
	const quadRef = useRef<Mesh>(null);
	const sceneRef = useRef<Scene>(null);
	const engineRef = useRef<Engine>(null);

	const AI_PLAYER = -1;
	const WAITING = 0;
	const PLAYER1 = 1;
	const PLAYER2 = 2;
	const SPLIT_SCREEN = 3;

	const [shaderQuality, setShaderQuality] = useState(session.graphics | 0);
	// let g_shaderQuality = session ? session.graphics : 1;

	// SHADER DROP DOWN
	// const setShaderQuality = (value: number) => {
	// 	g_shaderQuality = value;
	// 	setupShaders();
	// };

	useEffect(() => {

		session.graphics = shaderQuality;
		setupShaders();

	}, [shaderQuality]);

	const initializeBabylon = (engine: Engine) => {
		const scene = sceneRef.current;

		if (scene) {
			const camera = new FreeCamera("camera1", new Vector3(0, 0, 0), scene);
			camera.setTarget(new Vector3(0, 0, 1));

			const quad = MeshBuilder.CreatePlane("quad", { size: 2 }, scene);
			quad.position.z = 1;

			quadRef.current = quad;
		}
	};

	const setupShaders = async () => {
		const canvas = canvasRef.current;
		const scene = sceneRef.current;

		if (!scene || !canvas) return;

		const vertexShader = `
            precision highp float;
            attribute vec3 position;
            void main() { gl_Position = vec4(position, 1.0); }
        `;

		try {
			// Fetch the fragment shader from the public folder
			if (!fragmentShaderRef.current) {
				const response = await fetch("/frag.glsl");
				fragmentShaderRef.current = await response.text();
			}
			const fragmentShader = fragmentShaderRef.current;

			let shaderDefines =
				"#define QUALITY " +
				String(shaderQuality % 3) +
				"\n" +
				"#define " +
				(shaderQuality < 3 ? "BOX_WALLS" : "TUBE") +
				"\n";

			if (splitScreen) shaderDefines += "#define SPLIT_SCREEN\n";

			if (session.playerNum === PLAYER2) {
				console.log(`set player num ${session.playerNum}`);
				shaderDefines += "#define PLAYER2\n";
			}

			let modifiedShader = shaderDefines + fragmentShader;

			if (sceneRef.current && shaderMaterialRef.current)
				sceneRef.current.removeMaterial(shaderMaterialRef.current);

			engineRef.current?.releaseEffects();

			Effect.ShadersStore["customVertexShader"] = vertexShader;
			Effect.ShadersStore["customFragmentShader"] = modifiedShader;

			const uniforms = [
				"u_time",
				"u_resolution",
				"u_ball",
				"u_player1",
				"u_player2",
			];

			const shaderMaterial = new ShaderMaterial(
				`shader_${Date.now()}`,
				scene,
				{ vertex: "custom", fragment: "custom" },
				{ attributes: ["position"], uniforms: uniforms },
			);

			// Initialize uniforms
			if (canvas)
				shaderMaterial.setVector2(
					"u_resolution",
					new Vector2(canvas.width, canvas.height),
				);

			shaderMaterial.setFloat("u_time", (Date.now() - TIME_START) / 1000);
			shaderMaterial.setVector3("u_ball", new Vector3(0, 0, 0));
			shaderMaterial.setVector3("u_player1", new Vector3(0, 0, -3));
			shaderMaterial.setVector3("u_player2", new Vector3(0, 0, +3));

			if (quadRef.current) {
				quadRef.current?.material?.dispose(true);
				shaderMaterialRef.current?.dispose(true);
				quadRef.current.material = shaderMaterial;
			}

			shaderMaterialRef.current = shaderMaterial;
			console.log("Shader compilation OK");
			console.log(shaderDefines);

			setTimeout(restartRenderLoop, 200);
		} catch (error) {
			console.error("Shader compilation error:", error);
		}
	};

	useEffect(() => {
		const canvas = canvasRef.current;

		if (!canvas) return;

		const handleMouseMove = (e: MouseEvent) => {
			const rect = canvas.getBoundingClientRect();
			const canvas_x = e.clientX - rect.left;
			const canvas_y = e.clientY - rect.top;
			const x =
				((2 * canvas_x) / canvas.width - 1) * (canvas.width / canvas.height);
			const y = (-2 * canvas_y) / canvas.height + 1;
			const z = 3;
			const length = 0.4 * Math.sqrt(x * x + y * y + z * z);

			setMouseX(length * x);
			setMouseY(length * y);

			sendMsg({
				type: "move-paddle",
				x: length * x,
				y: length * y,
			});
		};

		const handleKeyDown = (e: KeyboardEvent) => {
			if (pressedKeysRef.current.has(e.key)) return;

			sendMsg({
				type: "keydown",
				key: e.key,
			});

			pressedKeysRef.current.add(e.key);
		};

		const handleKeyUp = (e: KeyboardEvent) => {
			if (pressedKeysRef.current.has(e.key))
				pressedKeysRef.current.delete(e.key);

			sendMsg({
				type: "keyup",
				key: e.key,
			});
		};

		window.addEventListener("mousemove", handleMouseMove);
		window.addEventListener("keydown", handleKeyDown);
		window.addEventListener("keyup", handleKeyUp);

		return () => {
			window.removeEventListener("mousemove", handleMouseMove);
			window.removeEventListener("keydown", handleKeyDown);
			window.removeEventListener("keyup", handleKeyUp);
		};
	}),
		[canvasRef];

	// Session update, update player num
	useEffect(() => {
		setupShaders();
	}, [session.playerNum, splitScreen, ]);

	// Game State update, update shader uniforms
	useEffect(() => {
		//console.log('update material');
		const shaderMaterial = shaderMaterialRef.current;

		if (shaderMaterial) {
			const ball = gameState.ball;
			const player1 = gameState.player1;
			const player2 = gameState.player2;

			shaderMaterial.setFloat("u_time", (Date.now() - TIME_START) / 1000);
			shaderMaterial.setVector3("u_ball", new Vector3(ball.x, ball.y, ball.z));
			shaderMaterial.setVector3(
				"u_player1",
				new Vector3(player1.x, player1.y, -3),
			);
			shaderMaterial.setVector3(
				"u_player2",
				new Vector3(player2.x, player2.y, +3),
			);
		}
	}, [gameState]);

	useEffect(() => {
		if (playerState !== "ready") return;

		const shaderMaterial = shaderMaterialRef.current;

		if (shaderMaterial) {
			if (session.playerNum === 1)
				shaderMaterial.setVector3("u_player1", new Vector3(mouseX, mouseY, -3));
			else if (session.playerNum === 2)
				shaderMaterial.setVector3("u_player2", new Vector3(mouseX, mouseY, +3));
		}
	}, [mouseX, mouseY]);

	const restartRenderLoop = async () => {
		await engineRef.current?.stopRenderLoop();
		pressedKeysRef.current.clear();

		if (!canvasRef.current) return;

		engineRef.current?.runRenderLoop(() => {
			sceneRef.current?.render();
		});
	};

	// Component loaded / unloaded
	useEffect(() => {
		const canvas = canvasRef.current;

		if (!canvas) return;

		const engine = new Engine(canvas, true);
		const scene = new Scene(engine);

		engineRef.current = engine;
		sceneRef.current = scene;

		initializeBabylon(engine);
		setupShaders();

		const resize = () => {
			engine.resize();
			if (shaderMaterialRef.current)
				shaderMaterialRef.current.setVector2(
					"u_resolution",
					new Vector2(canvas.width, canvas.height),
				);
		};
		window.addEventListener("resize", resize);

		return () => {
			window.removeEventListener("resize", resize);
			engine.dispose();
		};
	}, []);

	// ORIGINAL------------------------------------------------------------
	// return <canvas ref={canvasRef} id="renderCanvas" />

	//-----------------------------------------------------------------

	// RADIO BUTTONS------------------------------------------------------------

	// return (
	// 	<div style={{ position: 'relative', width: '100%', height: '100%' }}>
	// 	  <canvas ref={canvasRef} id="renderCanvas" style={{ width: '100%', height: '100%' }} />

	// 		  <div style={{
	// 		position: 'absolute',
	// 		top: 20,
	// 		left: 20,
	// 		backgroundColor: 'rgba(0, 0, 0, 0.5)',
	// 		padding: '5px',
	// 		borderRadius: '8px',
	// 		color: 'white',
	// 	  }}>
	// 		<p>Shader Quality:</p>
	// 		<label><input type="radio" name="quality" value="0" onClick={() => setShaderQuality(0)} /> Normal</label><br />
	// 		<label><input type="radio" name="quality" value="1" onClick={() => setShaderQuality(1)} /> Reflective</label><br />
	// 		<label><input type="radio" name="quality" value="2" onClick={() => setShaderQuality(2)} /> Reflective Anti-Aliasing</label>
	// 	  </div>
	// 	</div>
	//   );

	// SCALED CANVAS ------------------------------------------------------------
	// return (
	// 	<canvas
	// 	  ref={canvasRef}
	// 	  id="renderCanvas"
	// 	  style={{
	// 		position: 'fixed',
	// 		top: '150px',
	// 		left: '50%',
	// 		transform: 'translateX(-500px)',
	// 		width: '1000px',
	// 		height: '600px',
	// 		zIndex: 10,
	// 		pointerEvents: 'none',
	// 	  }}
	// 	/>
	//   );

	// PULL DOWN ------------------------------------------------------------
	return (
		<>
			<div
				style={{
					position: "fixed",
					top: "150px",
					left: "50%",
					transform: "translateX(-500px)",
					width: "1000px",
					height: "600px",
					pointerEvents: "none",
				}}
			>
				{/* Canvas */}
				<canvas
					ref={canvasRef}
					id="renderCanvas"
					style={{
						width: "100%",
						height: "100%",
						zIndex: 10,
						pointerEvents: "none",
					}}
				/>

				<div
					style={{
						position: "absolute",
						top: "-28px",
						left: 0,
						backgroundColor: "rgba(0, 0, 0, 0.5)",
						padding: "4px 6px",
						borderRadius: "0 0 6px 0",
						color: "white",
						fontSize: "12px",
						zIndex: 100,
						pointerEvents: "auto",
					}}
				>
					<label htmlFor="shaderQuality" style={{ marginRight: "6px" }}>
						Shader:
					</label>
					<select
						id="shaderQuality"
						value={shaderQuality}
						onChange={(e) => {
							setShaderQuality(Number(e.target.value));
							e.target.blur();
						}}
						style={{
							backgroundColor: "#222",
							color: "white",
							border: "1px solid #555",
							borderRadius: "4px",
							fontSize: "12px",
							padding: "2px 6px",
						}}
					>
						<option value="0">Low Quality</option>
						<option value="1">Shiny</option>
						<option value="2">Shiny AA</option>
						<option value="3">Low Quality Tube</option>
						<option value="4">Shiny Tube</option>
						<option value="5">Shiny AA Tube</option>
					</select>
				</div>
			</div>
		</>
	);
}
