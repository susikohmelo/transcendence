"use client";

import { createContext, useContext, useEffect, useState, useRef } from "react";
import {
	connectSocket,
	sendMsg,
	setPlayerNum,
	setPlayerID,
	getSession,
	setLobbyId,
} from "@/lib/session";
import { useRouter } from "next/navigation";
import { useMessage } from "./messagebox";
import { usePathname } from 'next/navigation';

export type Player = {
	id: number;
	name: string;
};

type Match = {
	player1: Player;
	player2: Player;
	winner: Player | undefined;
};

export type Lobby = {
	id: number;
	name: string;
	players: number;
	maxPlayers: number;
	ownerName: string | undefined;
	bracket: Match[][];
	lobbyState: string;
	setPlayers: (p: Player[]) => void;
	setLobbyState: (s: string) => void;
	setBracket: (m: Match[][]) => void;
};

// Lobby Context
const LobbyContext = createContext<any>({
	playerList: [],
	lobbyList: [],
	lobbyState: "wait",
	playerState: "wait",
	maxPlayers: 8,
	maxPoints: 3,
	lobbyCreator: undefined,
	splitScreen: false,
	countdown: 30,
	tournamentWinner: undefined,
	bracket: [],
	setLobbyState: (s: string) => {},
	setPlayerState: (s: string) => {},
	clearCountdown: () => {},
});

// TODO: create LobbyContextType
export function useLobby() {
	return useContext(LobbyContext);
}

// Current Game State
type GameStateData = {
	playerId: number;
	ball: { x: number; y: number; z: number };
	player1: { x: number; y: number; z: number };
	player2: { x: number; y: number; z: number };
};

const GameState = createContext<GameStateData>({
	playerId: 0,
	ball: { x: 0, y: 0, z: 0 },
	player1: { x: 0, y: 0, z: -3 },
	player2: { x: 0, y: 0, z: +3 },
});

export function useGameState() {
	return useContext(GameState);
}

export default function SessionHandler({
	children,
}: {
	children: React.ReactNode;
}) {
	const router = useRouter();
	const session = getSession();
	const { showError, showSuccess } = useMessage();
	

	const [initialized, setInitialized] = useState<boolean>(false);
	const [playerList, setPlayerList] = useState<Player[]>([]);
	const [lobbyState, setLobbyState] = useState<string>("wait");
	const [playerState, setPlayerState] = useState<string>("wait");
	const [splitScreen, setSplitScreen] = useState<boolean>(false);
	const [lobbyList, setLobbyList] = useState<any[]>([]);

	const [maxPlayers, setMaxPlayers] = useState<number>(8);
	const [maxPoints, setMaxPoints] = useState<number>(3);
	const [lobbyCreator, setLobbyCreator] = useState<number>(-1);
	const [tournamentWinner, setTournamentWinner] = useState<string>();

	const [countdown, setCountdown] = useState<number | null>(null);
	const countdownRef = useRef<any>(null);

	const bracketRef = useRef<Match[][]>([]);
	const [bracket, setBracket] = useState<Match[][]>([]);
	// const [playerMap, setPlayerMap] = useState<Player[]>([]);

	const unresolvedResults = useState<any[]>([]);

	const [gameState, setGameState] = useState<GameStateData>({
		playerId: 0,
		ball: { x: 0, y: 0, z: 0 },
		player1: { x: 0, y: 0, z: -3 },
		player2: { x: 0, y: 0, z: +3 },
	});

	const createBracket = (names: any, ids: number[]) => {
		console.log("create brackets");

		// player map
		const players = names.map((name: string, index: number) => ({
			id: ids[index],
			name,
		}));

		// console.log(players);

		// setPlayerMap(players);

		const n_bracket: Match[][] = [];
		let curRound = players;

		while (curRound.length > 1) {
			const round: Match[] = [];

			for (let i = 0; i < curRound.length; i += 2) {
				round.push({
					player1: curRound[i],
					player2: curRound[i + 1],
					winner: undefined,
				});
			}

			n_bracket.push(round);
			curRound = round.map(() => ({ id: -99, name: "TBD" }));
		}

		setBracket(n_bracket);
		// console.log(n_bracket);
	};

	const updateBracket = (result: any) => {
		setBracket((prevBracket) => {
			// Deep copy of the previous bracket
			const n_bracket = prevBracket.map((round) =>
				round.map((match) => ({
					player1: { ...match.player1 },
					player2: { ...match.player2 },
					winner: match.winner ? { ...match.winner } : undefined,
				})),
			);

			// console.log(`update brackets`);
			// console.log(result);

			for (let roundIndex = 0; roundIndex < n_bracket.length; roundIndex++) {
				const round = n_bracket[roundIndex];

				for (let matchIndex = 0; matchIndex < round.length; matchIndex++) {
					const match = round[matchIndex];

					if (match.winner !== undefined) continue;

					const ids = [match.player1.id, match.player2.id];
					const isMatch =
						ids.includes(result.winner_id) && ids.includes(result.loser_id);

					if (!isMatch) continue;

					// console.log(`unresolved match ${match.player1.id} : ${match.player1.name}, ${match.player2.id} : ${match.player2.name}`);

					let winner =
						match.player1.id === result.winner_id
							? match.player1
							: match.player2;
					match.winner = winner;

					// console.log(`winner ${winner.name}`);

					const nextRoundIndex = roundIndex + 1;

					if (n_bracket[nextRoundIndex]) {
						const nextMatchIndex = Math.floor(matchIndex / 2);
						const nextMatch = n_bracket[nextRoundIndex][nextMatchIndex];

						if (matchIndex % 2 === 0) {
							nextMatch.player1 = winner;
						} else {
							nextMatch.player2 = winner;
						}
					} else {
						setTournamentWinner(winner.name);
					}

					// Update bracketRef so others can read the current state
					bracketRef.current = n_bracket;

					// console.log(n_bracket);

					return n_bracket; // early return with the updated bracket
				}
			}

			console.log("no match found");
			unresolvedResults.push(result);

			// console.log(n_bracket);

			return prevBracket; // no change, return original bracket
		});

		return true;
	};

	const updatePlayerBracket = (old_id: number, new_id: number) => {
		const n_bracket = bracketRef.current.map((round) =>
			round.map((match) => ({
				player1: { ...match.player1 },
				player2: { ...match.player2 },
				winner: match.winner ? { ...match.winner } : undefined,
			})),
		);

		for (let roundIndex = 0; roundIndex < n_bracket.length; roundIndex++) {
			const round = n_bracket[roundIndex];

			for (let matchIndex = 0; matchIndex < round.length; matchIndex++) {
				const match = round[matchIndex];

				if (match.winner !== undefined) continue;

				const ids = [match.player1.id, match.player2.id];
				const winnerFound = ids.includes(old_id);
				const loserFound = ids.includes(old_id);

				// console.log(`match ${match.player1.id} : ${match.player1.name}, ${match.player2.id} : ${match.player2.name}`);

				if (winnerFound || loserFound) {
					// console.log('correct match');
					// console.log(ids);
					// console.log(`old: ${old_id}, new ${new_id}`);

					if (match.player1.id === old_id) {
						match.player1.id = new_id;
						match.player1.name = match.player1.name + "(AI)";
					}

					if (match.player2.id === old_id) {
						match.player2.id = new_id;
						match.player2.name = match.player2.name + "(AI)";
					}

					setBracket(() => {
						return n_bracket;
					});
					bracketRef.current = n_bracket;

					// console.log('updated player ids');

					// return true;
				}
			}
		}

		// TODO: resolve any unresolved results
		console.log("unresolved results");
		console.log(unresolvedResults);
	};

	const initializeCountdown = async () => {
		if (countdownRef.current) {
			clearInterval(countdownRef.current);
		}

		setCountdown(30);

		const interval = setInterval(() => {
			setCountdown((prev) => {
				if (prev === 1) {
					clearInterval(interval);

					if (session.lobbyId) {
						// moving to tournament page unloads room page
						// triggering disconnection

						router.push("/tournament");
					}
				}
				return prev ? prev - 1 : null;
			});
		}, 1000);

		countdownRef.current = interval;

		return () => {};
	};

	const clearCountdown = async () => {
		if (countdownRef.current) {
			// console.log('clear countdown');
			clearInterval(countdownRef.current);
		}
	};

	useEffect(() => {
		// console.log(JSON.stringify(bracket));
		bracketRef.current = bracket;
	}, [bracket]);

	// dev mode triggers twice
	useEffect(() => {
		// console.log('SessionHandler');

		const socket = connectSocket();

		if (socket === null) {
			console.log("Socket is null");
			return;
		}

		if (!initialized) {
			console.log("SessionHandler Initialize");
			setInitialized(true);

			socket.onopen = () => {
				console.log("WebSocket connection opened");

				socket.onmessage = async (event) => {
					if (event.data instanceof Blob) return;

					const msg = JSON.parse(event.data);

					if (msg.error) {
						showError(msg.error);
						// console.error(msg.error);
						return;
					}

					switch (msg.type) {
						case "connected":
							setPlayerID(msg.player_id);
							break;

						case "join-tournament":
						case "create-tournament":
							// console.log(msg);
							setMaxPlayers(msg.maxPlayers);
							setMaxPoints(msg.maxPoints);
							setLobbyCreator(msg.creator);
							setLobbyId(msg.name);
							setLobbyState("wait");
							router.push("/room");
							break;

						case "tournament-state":
							// console.log(msg);
							const formattedPlayers = msg.players.map(
								(player: string, index: number) => ({
									id: msg.players_ids[index], // Use the index as a unique ID
									name: player, // Use the player's name
								}),
							);
							setPlayerList(formattedPlayers);
							setMaxPoints(msg.points);
							// console.log(msg);
							switch (msg.msg) {
								case "start-tournament":
									// initialize front-end brackets
									createBracket(msg.players_in_round, msg.players_in_round_ids);
									break;

								case "won":
									setTournamentWinner(msg.player);
									setLobbyState("finish");
								case "advanced":
									// console.log(msg);
									setTimeout(() => {
										// TODO: find better solution
										updateBracket({
											winner_id: msg.player_id.winner,
											loser_id: msg.player_id.loser,
										});
									}, 100);
									break;

								case "player-quit":
									// console.error('player-quit');
									// console.log(msg);
									setLobbyCreator(msg.owner_id);
									updatePlayerBracket(msg.player_id, msg.extra_data);
									break;
							}
							break;

						case "query-tournaments":
							// console.log(msg);
							const formattedLobbies = msg["tournaments"].map(
								(tournament: any, index: number) => {
									return {
										id: index,
										name: tournament.extra_data,
										players: tournament.players?.length || 0,
										maxPlayers: tournament.max_players || 0,
										ownerName: tournament.player,
										inProgress: tournament.players_in_round.length > 0,
									};
								},
							);
							setLobbyList(formattedLobbies);
							break;

						case "start-tournament":
							// console.log('start-tournament');
							// console.log(msg);
							//setLobbyState('start');
							setSplitScreen(false);
							break;

						case "start-split-screen":
							// console.log('start-split-screen');
							// console.log(msg);
							setSplitScreen(true);
							break;

						case "split-screen-winner":
							// console.log('split-screen-winner', msg);
							setTournamentWinner(`Player ${msg.winner}`);
							setLobbyState("finish");
							break;

						case "game-over":
							// console.log('game-over');
							// console.log(msg);
							setLobbyState("finish");
							break;

						case "init-game":
							// console.log('init-game', msg);
							setPlayerNum(Number(msg.player));
							setLobbyState("start");
							setPlayerState("wait");
							initializeCountdown();
							break;

						case "start-game":
							setPlayerState("play");
							break;

						case "game-state":
							//console.log('game-state', msg);
							setGameState((prevState) => ({
								...prevState,
								ball: { x: msg.ball.x, y: msg.ball.y, z: msg.ball.z },
								player1: {
									x: msg.player1.x,
									y: msg.player1.y,
									z: prevState.player1.z,
								},
								player2: {
									x: msg.player2.x,
									y: msg.player2.y,
									z: prevState.player2.z,
								},
							}));
							break;

						case "game-result":
							// console.log('game-result');
							// console.log(msg);
							// updateBracket({ winner_id: msg.winner_id, winner: msg.winner, loser_id: msg.loser_id, loser: msg.loser });
							// updateBracketAI();
							setLobbyState("wait-next");
							if (msg.loser_id === getSession().playerID) {
								setPlayerState("lost");
							}
							if (msg.winner_id === getSession().playerID) {
								setPlayerState("wait");
							}
							break;

						case "game-score":
							// console.log('Player1 score: ' + msg.player1 + '; Player2 score: ' + msg.player2);
							break;

						case "ping":
							sendMsg({ type: "pong" });
							break;

						// TODO: if server handles timeout correctly
						// case 'ready-timeout':
						// 	sendMsg({
						// 		type: 'quit',
						// 		name: session.lobbyId,
						// 	});
						// 	session.lobbyId = null;
						// 	router.push('/tournament');
						// 	break;

						case "register":
							if (!msg.success) {
								showError(msg.error);
							} else {
								showSuccess("registration success!");
							}

							break;

						case "sign-in":
							// console.log(msg);

							if (msg.success === true) {
								// console.log('set token', msg['api-token']);
								session.accountID = msg.user.id;
								session.apiToken = msg.api_token;
								showSuccess("sign in successful!");
							}

							break;

						default:
						// console.log('unhandled msg type: ', msg.type);
					}
				};

				socket.onclose = () => {
					console.log("WebSocket connection closed (remote)");
					// Handle socket closure here
				};

				// Push to homepage on new session
				router.push("/");
			};
		} else {
			console.log("WebSocket already initialized");
		}

		return () => {
			// Don't close the socket on unmount unless you're sure you want to clean up completely
			// closeSocket(); <- disable this if you want persistence
		};
	}, []);

	function validateSession() {

		if (!session.socket)
			router.push("/");

		const pathName = usePathname();

		if (pathName !== '/' && !session.apiToken)
			router.push("/");

	}


	function SessionWatcher() {

		const pathname = usePathname();

		useEffect(() => {
			validateSession();
		}, [pathname]);

		return null;
	}

	const lobbyContextValue = {
		playerList,
		lobbyList,
		lobbyState,
		playerState,
		splitScreen,
		bracket,
		maxPlayers,
		maxPoints,
		lobbyCreator,
		countdown,
		tournamentWinner,
		setLobbyState,
		setPlayerState,
		clearCountdown,
	};

	return (
		<>
			{/* <SessionWatcher/> */}
			<LobbyContext.Provider value={lobbyContextValue}>
				<GameState.Provider value={gameState}>{children}</GameState.Provider>
			</LobbyContext.Provider>
		</>
	);
}
