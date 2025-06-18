"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { sendMsg, getSession, connectSocket } from "@/lib/session";
import { Lobby, useLobby } from "@/context/sessionhandler";
import { useMessage } from "@/context/messagebox";

export default function Tournament() {

	const [selected, setSelected] = useState<any | null>();
	const router = useRouter();
	const session = getSession();
	const { showError } = useMessage();

	const { lobbyList } = useLobby();
	// const lobbyList = lobby.lobbyList;

	useEffect(() => {}, [lobbyList]);

	useEffect(() => {

		if (session.socket === null) {
			console.log("Socket is null, connecting...");
			connectSocket();
			return;
		}

		queryTournaments();
		const interval = setInterval(queryTournaments, 5000);

		return () => {
			clearInterval(interval);
		};

		
	}, []);

	const queryTournaments = async () => {

		if (session.apiToken) {
			sendMsg({
				type: "query-tournaments",
			});
		}

	};

	const handleJoin = async () => {

		if (session.socket === null) {
			console.log("Socket is null, connecting...");
			connectSocket(); // retry
			return;
		}

		if (!selected) {
			showError("select a room first");
			return;
		}

		if (selected.inProgress) {
			showError("tournament is already in progress");
			return;
		}

		session.lobbyId = selected.name;

		sendMsg({
			type: "join-tournament",
			name: session.lobbyId,
			nick: session.playerName,
		});

		// No need to push to the room here, as the server will handle it
		//router.push('/room');
	};

	const handleCreate = async () => {

		if (session.socket === null) {
			console.log("Socket is null, connecting...");
			connectSocket(); // retry
			return;
		}

		if (!session.apiToken) {
			showError('Action requires sign in');
			return;
		}

		router.push('/create');

	};

	function shortName(name: string | undefined): string {
		if (!name) return "unknown";
		if (name.length <= 9) return name;
		return name.slice(0, 9) + (name.length > 9 ? "..." : "");
	}

	return (
		<main>
			<div className="content">
				<div className="content-line"></div>

				<div className="content-body">
					<h2 className="content-header">Tournaments</h2>

					<div className="content-columns">
						<div className="column">
							<div className="room-content">
								<div className="tournament-lobby-content center">
									<p>Create a new tournament or join existing one!</p>
								</div>
							</div>
						</div>

						<div className="tournament-column">
							{/* ============= Header */}
							<div className="tournament-header-row">
								<span className="th">Name</span>
								<span className="th">Players</span>
								<span className="th">Owner</span>
							</div>

							{/* ======= GAMES LIST ======= */}

							<div className="tournament-table-wrapper">
								{lobbyList.map((game: any) => (
									<div
										key={game.id}
										className={`tournament-row ${selected && selected.name === game.name ? "selected" : ""}`}
										onClick={() => setSelected(game)}
									>
										<span>{game.name}</span>
										<span>
											{game.players}/{game.maxPlayers}
										</span>
										<span>{shortName(game.ownerName)}</span>
									</div>
								))}
							</div>

							{/* ============= Buttons */}
							<div className="tournament-actions">
								<button 
									className="join-game-button" 
									onClick={handleJoin}
								>
									JOIN
								</button>
								<button
									onClick={handleCreate}
									className="create-game-button"
								>
									CREATE
								</button>
							</div>
						</div>
					</div>
				</div>
			</div>
		</main>
	);
}
