"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { sendMsg } from "@/lib/session";
import GameCanvas from "@/components/gamecanvas";

export default function gamestats() {
	const router = useRouter();
	// const [gameStarted, setGameStarted] = useState<boolean>(false);

	// useEffect(() => {
	// 	router.prefetch('/game');
	// }, []);

	// function startGame()
	// {
	// 	sendMsg({
	// 		type: 'start-tournament'
	// 	});

	// 	// if start game ok

	// 	setGameStarted(true);

	// 	//outer.push('/game');
	// }

	function handleLeave() {
		// ALIAS SERVER-SIDE VALIDATION

		// // Send alias for validation to the server
		// socket.send(JSON.stringify({
		//   type: "validate-alias",
		//   alias: alias.trim()
		// }));

		router.push("/tournament");
	}

	const players = [
		{ id: 1, name: "Player 1", status: "win" },
		{ id: 2, name: "Player 2", status: "lose" },
		{ id: 3, name: "Player 3", status: "pending" },
		{ id: 4, name: "Player 4", status: "pending" },
		{ id: 5, name: "Player 5", status: "pending" },
		{ id: 6, name: "Player 6", status: "pending" },
		{ id: 7, name: "Player 7", status: "pending" },
		{ id: 8, name: "Player 8", status: "pending" },
		{ id: 9, name: "Player 9", status: "pending" },
		{ id: 10, name: "Player 10", status: "pending" },
		{ id: 11, name: "Player 11", status: "pending" },
		{ id: 12, name: "Player 12", status: "pending" },
		{ id: 13, name: "Player 13", status: "pending" },
		{ id: 14, name: "Player 14", status: "pending" },
		{ id: 15, name: "Player 15", status: "pending" },
		{ id: 16, name: "Player 16", status: "pending" },
	];

	return (
		<main>
			<div className="content">
				<div className="content-line"></div>

				<div className="content-body">
					<h2 className="content-header">Live scores</h2>

					<div className="content-columns">
						<div className="column">
							<p>Status messages</p>

							<h2 className="status-msg">Game starting</h2>
							<h2 className="status-msg">Waiting for players to finish</h2>
							<h2 className="status-msg">Next round starting</h2>
							<h2 className="status-msg">Tournament finished</h2>
							<h2 className="status-msg"></h2>

							<div className="leave-button-wrapper">
								<button className="leave-button" onClick={handleLeave}>
									LEAVE GAME
								</button>
							</div>
						</div>

						<div className="room-column">
							{/* ============= Header */}
							<div className="room-header-row">
								<span className="th">Round 1/4</span>
							</div>

							{/* ======= PLAYERS LIST ======= */}

							<div className="room-table-wrapper">
								{players.map((_, i) => {
									if (i % 2 !== 0) return null;

									const player1 = players[i];
									const player2 = players[i + 1];

									return (
										<div key={player1.id} className="match-box">
											<span className={`player-name ${player1.status}`}>
												{player1.name}
											</span>
											<span className="score">0:0</span>
											<span className={`player-name ${player2.status}`}>
												{player2.name}
											</span>
										</div>
									);
								})}

								{/* 									
									{players.map((player) => (
									<div key={player.id} className="room-row">
										<span>{player.name}</span>
									</div>
									))}			
									 */}
							</div>
						</div>
					</div>
				</div>
			</div>
		</main>
	);
}
