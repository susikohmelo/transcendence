"use client";

import React, { useEffect, useState, useRef } from "react";
import { useRouter } from "next/navigation";
import { sendMsg, getSocket, setPlayerName } from "@/lib/session";
import { useLobby, Player } from "@/context/sessionhandler";
import { getSession } from "@/lib/session";
import GameCanvas from "@/components/gamecanvas";

export default function Room() {
	const router = useRouter();
	const session = getSession();

	const {
		lobbyCreator,
		lobbyState,
		setLobbyState,
		playerList,
		playerState,
		setPlayerState,
		bracket,
		maxPoints,
		maxPlayers,
		countdown,
		clearCountdown,
		tournamentWinner,
	} = useLobby();
	const [lobbyName, setLobbyName] = useState<string | null>(null);

	const [alias, setAlias] = useState<string>(getSession().playerName);

	useEffect(() => {
		setLobbyName(session.lobbyId);
	}, [session.lobbyId]);

	useEffect(() => {}, [playerState]);

	useEffect(() => {}, [playerList]);

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

	useEffect(() => {
		if (lobbyState === "start") {
			console.log("Game is starting...");
		} else if (lobbyState === "finish") {
			console.log("Game is over.");
		} else if (lobbyState === "advanced") {
			console.log("Advanced");
		}
	}, [lobbyState]);

	function startGame() {
		sendMsg({
			type: "start-tournament",
		});
	}

	const leaveRoom = () => {
		quitLobby();
		router.push("/tournament");

		// sendMsg({
		// 	type: 'quit',
		// 	name: session.lobbyId,
		// });
		// session.lobbyId = null;
		// router.push('/tournament');
	};

	function handleReady() {
		clearCountdown();
		setPlayerState("ready");
		sendMsg({ type: "ready" });
	}

	function handleReturnLobby() {
		if (lobbyState !== "finish") quitLobby();

		setLobbyState("wait");
		setPlayerState("wait");
	}

	function quitLobby() {
		clearCountdown();

		sendMsg({
			type: "quit",
			name: session.lobbyId,
		});

		if (session.lobbyId) {
			console.log("leave " + session.lobbyId);
			session.lobbyId = null;
		}
	}

	const BracketDisplay = () => {
		const roundsExceptFinal = bracket.slice(0, -1);
		const finalMatch = bracket.at(-1)?.[0];

		return (
			<>
				<div className="bracket-container">
					{/* Left Side */}
					{roundsExceptFinal.map((round: any, roundIndex: number) => (
						<div key={roundIndex} className="bracket-round">
							<h3 className="round-title">Round {roundIndex + 1}</h3>
							<div
								className={`match-card-container ${round.length > 1 ? "multiple" : ""}`}
							>
								{round
									.slice(0, round.length / 2)
									.map((match: any, matchIndex: number) => (
										<div key={matchIndex} className="match-card">
											<p
												className={`player-name ${match.winner !== undefined && match.winner.id === match.player1.id ? "winner" : ""}`}
											>
												{match.player1.name}
											</p>
											<p
												className={`player-name ${match.winner !== undefined && match.winner.id === match.player2.id ? "winner" : ""}`}
											>
												{match.player2.name}
											</p>
										</div>
									))}
							</div>
						</div>
					))}

					{/* Finals */}
					{finalMatch && (
						<div className="bracket-round">
							<h3 className="round-title">Finals</h3>
							<div className={`match-card-container`}>
								<div className="match-card final-card">
									<p
										className={`player-name ${finalMatch.winner !== undefined && finalMatch.winner.id === finalMatch.player1.id ? "winner" : ""}`}
									>
										{finalMatch.player1.name}
									</p>
									<p
										className={`player-name ${finalMatch.winner !== undefined && finalMatch.winner.id === finalMatch.player2.id ? "winner" : ""}`}
									>
										{finalMatch.player2.name}
									</p>
								</div>
							</div>
						</div>
					)}

					{/* Right Side */}
					{roundsExceptFinal
						.slice() // shallow copy
						.reverse() // reverse round order to mirror
						.map((round: any, roundIndex: number) => (
							<div key={roundIndex} className="bracket-round">
								<h3 className="round-title">
									Round {roundsExceptFinal.length - roundIndex}
								</h3>
								<div
									className={`match-card-container ${round.length > 1 ? "multiple" : ""}`}
								>
									{round
										.slice(round.length / 2)
										.map((match: any, matchIndex: number) => (
											<div key={matchIndex} className="match-card">
												<p
													className={`player-name ${match.winner !== undefined && match.winner.id === match.player1.id ? "winner" : ""}`}
												>
													{match.player1.name}
												</p>
												<p
													className={`player-name ${match.winner !== undefined && match.winner.id === match.player2.id ? "winner" : ""}`}
												>
													{match.player2.name}
												</p>
											</div>
										))}
								</div>
							</div>
						))}
				</div>
			</>
		);
	};

	return (
		<main style={{ position: "relative" }}>
			<div className="content">
				<div className="content-line"></div>
				<div className="content-body">
					{lobbyState === "wait" ? (
						<>
							<h2 className="content-header">
								{lobbyName ? lobbyName : "Tournament"}
							</h2>
							<div className="content-columns">
								<div className="column">
									{/* Room */}
									<div className="room-content center">
										<p>Rules:</p>
										<p>Points to win: {maxPoints ?? "?"}</p>
										<p>
											The tournament will start as soon as all the players have
											joined or lobby owner starts the game.
										</p>
									</div>

									{/* Player UI */}
									<div className="leave-button-wrapper">
										{lobbyCreator === session.playerID && (
											<button className="leave-button" onClick={startGame}>
												START
											</button>
										)}
										<button className="leave-button" onClick={leaveRoom}>
											LEAVE
										</button>
									</div>

									{/* Change alias */}
									<div className="alias-row">
										<div className="alias-label">Alias:</div>
										<input
											className="alias-input"
											type="text"
											placeholder={alias}
											value={alias}
											maxLength={25}
											onChange={(e) => {
												setAlias(e.target.value);
												setPlayerName(e.target.value);
												sendMsg({
													type: "change-nick",
													nick: e.target.value,
												});
											}}
										/>
									</div>
								</div>

								<div className="room-column">
									{/* Header */}
									<div className="room-header-row">
										<span className="th">
											Players ({playerList.length}/{maxPlayers ?? "?"})
										</span>
									</div>

									{/* Player List */}

									<div className="room-table-wrapper">
										{playerList.map((player: Player, index: number) => (
											<div key={index} className="room-row">
												{player.id === lobbyCreator && (
													<img src="/Crown.png" className="icon" />
												)}
												<span>{player.name}</span>
											</div>
										))}
									</div>
								</div>
							</div>
						</>
					) : (
						<>
							{lobbyState === "start" ? (
								<>
									{/* Game View */}
									{playerState === "wait" && (
										<div className="ready-wrapper">
											{countdown ? (
												<h1 className="timer noselect">{countdown}</h1>
											) : (
												""
											)}
											<button className="ready-button" onClick={handleReady}>
												Ready
											</button>
										</div>
									)}
									{playerState === "ready" && (
										<div className="ready-wrapper">
											<h1 className="timer noselect">
												Waiting for other player...
											</h1>
										</div>
									)}
									<GameCanvas />
								</>
							) : (
								<>
									{/* Bracket View (Waiting) */}
									<div className="ready-wrapper">
										{/* TODO: implement bracket view */}

										{lobbyState === "finish" ? (
											<>
												<p className="white">
													{lobbyName ? lobbyName : "Tournament"} Winner:{" "}
													{tournamentWinner}
												</p>
											</>
										) : (
											<>
												<p className="white">
													{playerState === "lost"
														? "Knocked out"
														: "Waiting for next match..."}
												</p>
											</>
										)}

										<BracketDisplay />

										<button
											className="leave-button"
											onClick={handleReturnLobby}
										>
											{lobbyState === "finish"
												? "RETURN TO LOBBY"
												: "LEAVE GAME"}
										</button>
									</div>
								</>
							)}
						</>
					)}
				</div>
			</div>
		</main>
	);
}
