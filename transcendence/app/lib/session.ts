type Session = {
	playerID: number;
	accountID: number;
	playerName: string;
	playerNum: number;
	lobbyId: string | null;
	graphics: number;
	socket: WebSocket | null;
	apiToken: string | null; // don't store here, use JWT cookie
};

const session: Session = {
	playerID: -1,
	accountID: -1,
	playerName: "Anonymous",
	playerNum: 0,
	lobbyId: "Unknown",
	graphics: 0,
	socket: null,
	apiToken: null,
};

export const getSession = () => session;
export const getSocket = () => session.socket;

export const connectSocket = () => {
	if (!session.socket || session.socket.readyState !== WebSocket.OPEN) {
		// console.log('connect to backend');
		// TODO: change to wss when reverse proxy is setup
		session.socket = new WebSocket(process.env.NEXT_PUBLIC_WS_URL || 'wss://localhost:8080/ws');

		// check JWT cookies for apiToken
	}
	return session.socket;
};

export const setPlayerID = (id: number) => (session.playerID = id);
export const setPlayerName = (name: string) => (session.playerName = name);
export const setPlayerNum = (num: number) => (session.playerNum = num);
export const setLobbyId = (id: string) => (session.lobbyId = id);
export const setGraphics = (level: number) => (session.graphics = level);

export const sendMsg = (msg: any) => {
	if (session.socket && session.socket.readyState === WebSocket.OPEN) {
		msg["user_id"] = session.accountID;
		msg["api_token"] = session.apiToken;
		session.socket.send(JSON.stringify(msg));
	}
};
