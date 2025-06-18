"use client";

import React, { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { sendMsg, getSession, connectSocket } from "@/lib/session";

export default function Create() {
	const tempRoomName = `${getSession().playerName}'s room`;
	const [alias, setAlias] = useState<string>(tempRoomName);
	const [playerCount, setPlayerCount] = useState(4);
	const [points, setPoints] = useState(5);
	const playerOptions = [2, 4, 8, 16];
	const [quality, setQuality] = useState("normal");

	const router = useRouter();

	function handleJoin() {
		const socket = connectSocket();

		if (socket === null) {
			console.log("Socket is null, connecting...");
			connectSocket(); // retry
			return;
		}

		// ALIAS SERVER-SIDE VALIDATION

		// // Send alias for validation to the server
		// socket.send(JSON.stringify({
		//   type: "validate-alias",
		//   alias: alias.trim()
		// }));

		sendMsg({
			type: "create-tournament",
			name: alias,
			player_count: Number(playerCount),
			points: Number(points),
			nick: getSession().playerName,
		});

		// No need to push to the room here, as the server will handle it
		//router.push('/room');
	}

	function handleCancel() {
		router.push("/tournament");
	}

	return (
		<main>
			<div className="content">
				<div className="content-line"></div>

				<div className="content-body">
					<h2 className="content-header">Create new Tournament</h2>

					<div className="content-columns">
						<div className="form-wrapper">
							<div className="column">
								{/* ====== OPTIONS ========= */}

								<div className="form-row">
									<input
										className="alias-input"
										type="text"
										value={alias}
										maxLength={25}
										onChange={(e) => {
											setAlias(e.target.value);
										}}
									/>
								</div>
								<div className="form-row">
									<div className="form-label">Players:</div>
									<div className="form-control player-options">
										{playerOptions.map((count) => (
											<button
												key={count}
												className={`player-button ${playerCount === count ? "selected" : ""}`}
												onClick={() => setPlayerCount(count)}
											>
												{count}
											</button>
										))}
									</div>
								</div>
								{/* ====== POINTS ========= */}

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

								<div className="form-row">
									<div className="create-buttons-container">
										<div className="create-button-wrapper">
											<button className="create-button" onClick={handleJoin}>
												CREATE GAME
											</button>
										</div>
										<div className="create-button-wrapper">
											<button className="create-button" onClick={handleCancel}>
												CANCEL
											</button>
										</div>
									</div>
								</div>
							</div>
						</div>

						<div className="column">
							<div className="room-content">
								<p>
									Set up your tournament by choosing how many players will join
									and defining the score required to win a round.
								</p>
								<p>
									A new game room will be created where others can join and wait
									for the match to begin.
								</p>
							</div>
						</div>
					</div>
				</div>
			</div>
		</main>
	);
}
