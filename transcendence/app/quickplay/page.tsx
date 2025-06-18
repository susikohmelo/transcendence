"use client";

import GameCanvas from "@/components/gamecanvas";
import Image from "next/image";
import { useLobby } from "@/context/sessionhandler";
import { getSession, sendMsg } from "@/lib/session";
import { useEffect, useState } from "react";

export default function Game() {
	const session = getSession();
	const { lobbyState, setLobbyState, tournamentWinner } = useLobby();
	const [points, setPoints] = useState(5);

	useEffect(() => {
		setLobbyState("wait");
	}, []);

	useEffect(() => {
		const handleBeforeUnload = (e: BeforeUnloadEvent) => {
			quitLobby();
		};

		window.addEventListener("beforeunload", handleBeforeUnload);

		return () => {
			console.log("Unmounting room page — user left!");
			window.removeEventListener("beforeunload", handleBeforeUnload);
			quitLobby();
		};
	}, []);

	const handleStart = async () => {
		sendMsg({ type: "start-split-screen", points });
	};

	function quitLobby() {
		sendMsg({
			type: "quit",
			name: session.lobbyId,
		});

		if (lobbyState === "start") {
			session.lobbyId = null;

			setLobbyState("wait");
		}
	}

	return (
		<>
			<main style={{ position: "relative" }}>
				<div className="content">
					<div className="content-line"></div>
					<div className="content-body">
						{lobbyState === "wait" ? (
							<>
								<h2 className="content-header">Quickplay</h2>
								<div className="content-columns">
									<div className="column">
										<div className="form-wrapper center">
											{/* ====== HELP ========= */}

											<Image
												className="playerKeys"
												src="/playerKeys.png"
												alt="Keys Logo"
												width={324}
												height={148}
											/>

											{/* ====== OPTIONS ========= */}

											<div className="form-row">
												<div className="form-label">Points:</div>
												<div className="form-control points-slider">
													<input
														type="range"
														min="1"
														max="7"
														value={points}
														onChange={(e) => setPoints(Number(e.target.value))}
														className="slider"
													/>
													<span className="points-value">{points}</span>
												</div>
											</div>

											<div className="create-button-wrapper">
												<button className="create-button" onClick={handleStart}>
													START GAME
												</button>
											</div>
										</div>
									</div>
								</div>
							</>
						) : (
							<>
								{lobbyState === "finish" ? (
									<>
										<h2 className="content-header">Quickplay</h2>
										<div className="content-columns">
											<div className="column center">
												<p>PLAYER {tournamentWinner} WON</p>
												<div className="create-button-wrapper">
													<button
														className="create-button"
														onClick={() => setLobbyState("wait")}
													>
														Restart
													</button>
												</div>
											</div>
										</div>
									</>
								) : (
									<>
										<GameCanvas />
									</>
								)}
							</>
						)}
					</div>
				</div>
			</main>
		</>
	);
}
