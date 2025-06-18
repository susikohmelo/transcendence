// ************************************************************************** //
//                                                                            //
//                                                        :::      ::::::::   //
//   game-server.js                                     :+:      :+:    :+:   //
//                                                    +:+ +:+         +:+     //
//   By: lfiestas <lfiestas@student.hive.fi>        +#+  +:+       +#+        //
//                                                +#+#+#+#+#+   +#+           //
//   Created: 2025/04/19 12:30:44 by lfiestas          #+#    #+#             //
//   Updated: 2025/06/09 18:03:58 by lfiestas         ###   ########.fr       //
//                                                                            //
// ************************************************************************** //

const MAX_INPUT_LENGTH = 25;
let g_clients = {};
let g_clientIDCounter = 0;

// ----------------------------------------------------------------------------
// Tournament

const MAX_POINTS = 7;
let g_tournaments = {};

class Tournament
{
    max_players      =  0; // power of 2
    human_players    = [];
    players_in_round = []; // empty when no games running
    winners          = []; // waiting to go to the next round
    winners_length   =  0;
    creator          = -1;
    points           =  0;

    constructor(creator_id, max_players, points) {
        this.max_players = max_players;
        this.human_players.push(creator_id);
        this.creator = creator_id;
        this.points = points;
    }
}

const getNick = (id) => id <= AI_PLAYER ? 'AI' : g_clients[id]?.nick || 'Anonymous';

// Send this to clients
function getTournamentState(tournament, msg, player, player_id, extra_data = null)
{
    return {
        type: 'tournament-state',
        msg: msg,
        players: tournament.human_players.map(getNick),
        players_ids: tournament.human_players,
        players_in_round: tournament.players_in_round.map(getNick),
        players_in_round_ids: tournament.players_in_round,
        players_advancing: tournament.winners.map(getNick),
        players_advancing_ids: tournament.winners,
        player: player, // value of this depends on msg
        player_id: player_id,
        points: tournament.points,
        max_players: tournament.max_players,
        owner: getNick(tournament.creator),
        owner_id: tournament.creator,
        extra_data: extra_data,
    };
}

function tournamentBroadcastState(tournament, msg, player, player_id, extra_data = null)
{
    console.log('Broadcasting to tournament');
    console.log(msg);
    for (let id of tournament.human_players)
        g_clients[id].client.send(JSON.stringify(getTournamentState(
            tournament, msg, player, player_id, extra_data)));
}

// ----------------------------------------------------------------------------

const PADDLE_VELOCITY    = .025;
const BALL_VELOCITY      = .05;

const AI_PLAYER    = -1;
const WAITING      =  0;
const PLAYER1      =  1;
const PLAYER2      =  2;
const SPLIT_SCREEN =  3;

class Game
{
    ball    = { x: 0, y: 0, z: 0 };
    player1 = { id: -1, x: 0, y: 0, points: 0 };
    player2 = { id: -1, x: 0, y: 0, points: 0 };

    ball_current_velocity = 0;
    ball_dir              = { x: 0, y: 0, z: 0 };
    restarting            = false;
    prediction            = { x: 0, y: 0 };
    prediction_for_ai     = { x: 0, y: 0 };
    usingMouse            = false;
    TIME_START            = 0;
    ai_prev               = [{ x: 0, y: 0 }, { x: 0, y: 0 }];
    ball_prev             = [{ x: 0, y: 0 }, { x: 0, y: 0 }];
    frame_counter         = 0;
    points                = 0;
    tournament            = null;
    index_in_tournament   = null;
	is_split_screen       = false;
    started               = false;

    constructor(player1_id, player2_id, tournament_or_points, index) {
        const is_split_screen = player1_id === player2_id && player1_id > AI_PLAYER;
        this.player1.id = player1_id;
        this.player2.id = player2_id;
        if ( ! is_split_screen) {
            this.tournament = tournament_or_points;
            this.index_in_tournament = index;
        }
        this.points = is_split_screen ?
            tournament_or_points
          : tournament_or_points.points;

        if (player1_id > AI_PLAYER) {
            g_clients[player1_id].ready = false;
        }
        if (player2_id > AI_PLAYER) {
            g_clients[player2_id].ready = false;
        }
        if (player1_id <= AI_PLAYER && player2_id <= AI_PLAYER)
            start_game(this);
    }
}

function start_game(game) {
    if (game.started) // some edge cases may cause double start, ignore second
        return;
    game.started = true;
    const msg = {
        type: 'start-game',
    }
    if (game.player1.id > AI_PLAYER) {
        g_clients[game.player1.id].keysPressed['w'] = 0;
        g_clients[game.player1.id].keysPressed['a'] = 0;
        g_clients[game.player1.id].keysPressed['s'] = 0;
        g_clients[game.player1.id].keysPressed['d'] = 0;
        g_clients[game.player1.id].keysPressed['ArrowUp'] = 0;
        g_clients[game.player1.id].keysPressed['ArrowDown'] = 0;
        g_clients[game.player1.id].keysPressed['ArrowLeft'] = 0;
        g_clients[game.player1.id].keysPressed['ArrowRight'] = 0;
        g_clients[game.player1.id].client.send(JSON.stringify(msg));
    }
    if (game.player2.id > AI_PLAYER) {
        g_clients[game.player2.id].keysPressed['w'] = 0;
        g_clients[game.player2.id].keysPressed['a'] = 0;
        g_clients[game.player2.id].keysPressed['s'] = 0;
        g_clients[game.player2.id].keysPressed['d'] = 0;
        g_clients[game.player2.id].keysPressed['ArrowUp'] = 0;
        g_clients[game.player2.id].keysPressed['ArrowDown'] = 0;
        g_clients[game.player2.id].keysPressed['ArrowLeft'] = 0;
        g_clients[game.player2.id].keysPressed['ArrowRight'] = 0;
        g_clients[game.player2.id].client.send(JSON.stringify(msg));
    }
    game.TIME_START = Date.now();
    init_ball(game, null);
    pong(game);
}

// Send this to clients
function getGameState(game, player) {
    return {
        type: 'game-state',
        ball: {
            x: game.ball.x,
            y: game.ball.y,
            z: game.ball.z
        }, player1: {
            x: game.player1.x,
            y: game.player1.y
        }, player2: {
            x: game.player2.x,
            y: game.player2.y
        },
    };
}

// ----------------------------------------------------------------------------
// Client

function initClient(client) {
    const id = g_clientIDCounter++;
    g_clients[id] = {
        client: client,

        user: null,
        api_token: '',
        nick: 'Anonymous',
        tournament: null,

        keysPressed: {
            'w': 0,
            'a': 0,
            's': 0,
            'd': 0,
            'ArrowUp':    0,
            'ArrowDown':  0,
            'ArrowLeft':  0,
            'ArrowRight': 0,
        },
        game: null,
        ready: false,
        gotPong: true,
    };
    return id;
}

// ----------------------------------------------------------------------------
// Backend Setup

const crypto = require('crypto');
const path = require('path');
const fs = require('fs');

const sqlite3 = require('sqlite3');
const { open } = require('sqlite');

function hashString(str)
{
    const hash = crypto.createHash('sha256');
    hash.update(str);
    return hash.digest('hex');
}

// Open DB async (wrap this in async startup)
let db;

async function initDB() {
	db = await open({
		filename: '/app/data/db.sqlite',  // Adjust path if needed
		driver: sqlite3.Database
	});
}

const fastify = require('fastify')({
    // logger: t, // uncomment to debug connection issues
})

const cors = require('@fastify/cors');

fastify.register(cors, {
    origin: '*', // or specify: ['htt(p://localhost:3000']
    methods: ['GET', 'POST'],
});

function quit(id)
{
    let new_id = null; // of AI that replaces player
    const tournament = g_clients[id].tournament;
    const game = g_clients[id].game;

    console.log(id + " quitting from tournament " + tournament);

    // Quit from game first
    if (game && game.is_split_screen == false) {
        // Replace the player with AI
        if (id === game.player1.id) {
            new_id = game.player1.id = Math.min(...g_tournaments[tournament].players_in_round) - 1;
            if (new_id > AI_PLAYER)
                new_id = game.player1.id = AI_PLAYER;
            if (game.player2.id <= AI_PLAYER || g_clients[game.player2.id].ready)
                start_game(game);
        }
        else if (id === game.player2.id) {
            new_id = game.player2.id = Math.min(...g_tournaments[tournament].players_in_round) - 1;
            if (new_id > AI_PLAYER)
                new_id = game.player2.id = AI_PLAYER;
            if (game.player1.id <= AI_PLAYER || g_clients[game.player1.id].ready)
                start_game(game);
        }
    }
    else if (game) { // end split screen
        game.player1.id = AI_PLAYER;
        game.player2.id = AI_PLAYER;
    }
    g_clients[id].game = null;

    // Quit from tournament
    if (g_tournaments[tournament]) {
        let index = g_tournaments[tournament].human_players.indexOf(id);
        g_tournaments[tournament].human_players.splice(index, 1);
        index = g_tournaments[tournament].players_in_round.indexOf(id);
        g_tournaments[tournament].players_in_round[index] = new_id;
        index = g_tournaments[tournament].winners.indexOf(id);
        g_tournaments[tournament].winners[index] = new_id;

        if (g_tournaments[tournament].creator === id)
            g_tournaments[tournament].creator = new_id;


        g_clients[id].tournament = null;
        if (g_tournaments[tournament].human_players.length > 0)
            g_tournaments[tournament].creator = g_tournaments[tournament].human_players[0];
        tournamentBroadcastState(g_tournaments[tournament], 'player-quit', g_clients[id].nick || 'Anonymous', id, new_id);
        if (g_tournaments[tournament].human_players.length === 0)
            delete g_tournaments[tournament];
    }
}

function check_api_token(id, msg)
{
    // return true; // TODO REMOVE THIS IMMEDIATELY AFTER LOGIN IMPLEMENTED IN FRONTEND!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if ( ! g_clients[id].user)
        msg['error'] = 'User not registered';
    else if ( ! msg.user_id)
        msg['error'] = 'No user id provided for API validation';
    else if (msg.user_id !== g_clients[id].user.id)
        msg['error'] = 'User API id does not match';
    else if ( ! msg.api_token)
        msg['error'] = 'No API token provided';
    else if (msg.api_token !== g_clients[id].api_token)
        msg['error'] = 'Invalid API token';

    if (msg['error']) {
        g_clients[id].client.send(JSON.stringify(msg));
        return false;
    }
    return true;
}

// Client tracking stores all websockets into the websocketServer.clients class as a set.
fastify.register(require('@fastify/websocket'), { options: { clientTracking: true } });

fastify.register(async function () { fastify.route({
    method: 'GET',
    url: '/ws',
    handler: (req, reply) => {},
    wsHandler: (socket, req) => {
        console.log('Client just joined.');
        const id = initClient(socket);

        socket.send(JSON.stringify({
            type: 'connected',
            player_id: id
        }));

        socket.onclose = (e) => {
            quit(id);
            delete g_clients[id];
            console.log('Client disconnected');
        };

        function ping()
        {
            if ( ! g_clients[id]) // client can leave before the previous timeout is completed
                return;

            if ( ! g_clients[id].gotPong)
                g_clients[id].client.close();
            else {
                g_clients[id].client.send(JSON.stringify({ type: 'ping' }));
                g_clients[id].gotPong = false;
                setTimeout(ping, 10*1000);
            }
        }
        ping();

        // Check leading and trailing whitespaces
        function invalid_spaces(str)
        {
            return (/\s/.test(str[0]) || /\s/.test(str[str.length - 1]));
        }

        // --------------------------------------------------------------------
        // Event Handling

        socket.onmessage = (event) => { let msg = JSON.parse(event.data);
                                        const tournament = g_clients[id].tournament;
                                        const game = g_clients[id].game;

        switch (msg.type) {

        case 'pong':
            g_clients[id].gotPong = true;
            break;

        case 'register':
            if ( ! msg.username)
                msg['error'] = 'No name provided';
            else if (msg.username.length > MAX_INPUT_LENGTH)
                msg['error'] = 'Username too long';
            else if (invalid_spaces(msg.username))
                msg['error'] = 'Trailing or leading whitespace in username';
            else if ( ! msg.password)
                msg['error'] = 'No password provided';
            else if (msg.password.length > MAX_INPUT_LENGTH)
                msg['error'] = 'Password too long';
            else if ( ! msg.passconf)
                msg['error'] = 'No password confirmation provided';
            else if (msg.passconf.length > MAX_INPUT_LENGTH)
                msg['error'] = 'Password confirmation too long';
            else if (msg.password !== msg.passconf)
                msg['error'] = 'Passwords do not match';
            else {
                const query = async () => {
                    try {
                        const existingUser = await db.get(
                            'SELECT username FROM users WHERE username = ?',
                            msg.username
                        );

                        if (existingUser) {
                            msg['error'] = 'Username already exists';
                            console.log('account create fail!');
                        } else {
                            await db.run(
                                'INSERT INTO users (username, password) VALUES (?, ?)',
                                msg.username,
                                hashString(msg.password)
                            );

                            const user = await db.get(
                                'SELECT id, username FROM users WHERE username = ?',
                                msg.username
                            );
                            msg['success'] = true;
                            msg['user'] = user;
                            console.log('User registered:', user);
                        }
                    } catch (error) {
                        console.error('Database error:', error);
                        msg['error'] = 'Database error occurred';
                    }
                };
                query().finally(() => {
                    socket.send(JSON.stringify(msg));
                });
            }
            if (msg['error']) {
                socket.send(JSON.stringify(msg));
                console.log('register attempt');
            }
            break;

        case 'sign-in':
            if (!msg.username)
                msg['error'] = 'No name provided';
            else if (!msg.password)
                msg['error'] = 'No password provided';
            else {
                const query = async () => {
                    try {
                        const user = await db.get(
                            'SELECT id, username FROM users WHERE username = ? AND password = ?',
                            msg.username,
                            hashString(msg.password)
                        );

                        if (user) {
                            msg['success'] = true;
                            g_clients[id].user = msg['user'] = user;
                            console.log('login success!');

                            const api_token_length = 32;
                            const api_token_bytes = new Uint8Array(api_token_length);
                            crypto.getRandomValues(api_token_bytes);
                            g_clients[id].api_token = '';
                            for (let i = 0; i < api_token_length; ++i) {
                                // Paranoidly discard null-byte to not confuse C
                                while (api_token_bytes[i] & 0x7F === 0)
                                    crypto.getRandomValues(api_token_bytes);
                                // Only allowing ASCII is the simplest way to guarantee valid UTF-8
                                g_clients[id].api_token += String.fromCharCode(api_token_bytes[i] & 0x7F);
                            }
                            msg['api_token'] = g_clients[id].api_token;

                        } else {
                            msg['success'] = false;
                            msg['error'] = 'Invalid username or password';
                            console.log('login fail!');
                        }

                    } catch (error) {
                        console.error('Database error:', error);
                        msg['error'] = 'Database error occurred';
                    }
                };

                query().finally(() => {
                    socket.send(JSON.stringify(msg));
                });
            }

            if (msg['error']) {
                socket.send(JSON.stringify(msg));
                console.log('sign in attempt');
            }
            break;

        case 'create-tournament':
            if ( ! check_api_token(id, msg))
                break;
            else if ( ! msg.name)
                msg['error'] = 'No name provided';
            else if (msg.name.length > MAX_INPUT_LENGTH)
                msg['error'] = 'Name too long';
            else if (invalid_spaces(msg.name))
                msg['error'] = 'Trailing or leading whitespace in tournament name';
            else if (msg.nick && invalid_spaces(msg.nick))
                msg['error'] = 'Trailing or leading whitespace in nick';
            else if (g_tournaments[msg.name])
                msg['error'] = 'Tournament name already in use';
            else if (tournament)
                msg['error'] = 'You are already in another tournament';
            else if (game)
                msg['error'] = 'Cannot create a tournament while playing in split screen';
            else if ( ! [2, 4, 8, 16].includes(msg.player_count))
                msg['error'] = 'Tournament must have 2, 4, 8, or 16 players. Got: ' + msg.player_count;
            else if ( ! msg.points || !(1 <= msg.points && msg.points <= MAX_POINTS))
                msg['error'] = 'Invalid points. Expected 1-' + MAX_POINTS + ', got: ' + msg.points;
            else {
                g_tournaments[msg.name] = new Tournament(id, msg.player_count, msg.points);
                g_clients[id].tournament = msg.name;
                if (msg.nick)
                    g_clients[id].nick = msg.nick || 'Anonymous';

                msg['response'] = 'Created tournament: ' + msg.name;
                msg['maxPoints'] = g_tournaments[msg.name].points;
                msg['maxPlayers'] = g_tournaments[msg.name].max_players;
                msg['creator'] = g_tournaments[msg.name].creator;

                // Send state to self
                socket.send(JSON.stringify(getTournamentState(g_tournaments[msg.name], 'tournament-created', g_clients[id].nick, id)));
            }
            socket.send(JSON.stringify(msg));
            console.log('create ' + msg.name);
            break;

        case 'join-tournament':
            if ( ! check_api_token(id, msg))
                break;
            else if ( ! msg.name)
                msg['error'] = 'No name provided';
            else if (msg.name.length > MAX_INPUT_LENGTH)
                msg['error'] = 'Name too long';
            else if ( ! g_tournaments[msg.name])
                msg['error'] = 'No such tournament: ' + msg.name;
            else if (msg.nick && invalid_spaces(msg.nick))
                msg['error'] = 'Trailing or leading whitespace in nick';
            else if (tournament)
                msg['error'] = 'You are already in tournament: ' + tournament;
            else if (game)
                msg['error'] = 'Cannot join a tournament while playing in split screen';
            else if (g_tournaments[msg.name].human_players.length === g_tournaments[msg.name].max_players)
                msg['error'] = 'Tournament full: ' + msg.name;
            else {
                g_tournaments[msg.name].human_players.push(id);
                g_clients[id].tournament = msg.name;
                if (msg.nick)
                    g_clients[id].nick = msg.nick;


                msg['response'] = 'Joined tournament: ' + msg.name;
                msg['maxPoints'] = g_tournaments[msg.name].points;
                msg['maxPlayers'] = g_tournaments[msg.name].max_players;
                msg['creator'] = g_tournaments[msg.name].creator;

                // broadcast to everyone
                console.log('broadcast join');
                tournamentBroadcastState(g_tournaments[msg.name], 'tournament-state', g_clients[id].nick, id);
            }
            socket.send(JSON.stringify(msg));
            console.log('join ' + msg.name);
            break;

        case 'start-tournament':
            if ( ! check_api_token(id, msg))
                break;
            else if ( ! tournament)
                msg['error'] = 'You are not in a tournament';
            else if (id !== g_tournaments[tournament].creator)
                msg['error'] = 'You are not the creator of this tournament: ' + tournament;
            else if (g_tournaments[tournament].players_in_round.length > 0)
                msg['error'] = 'Tournament already started: ' + tournament;
            else { // assign players to random slots and start game
                let temp_players = [...g_tournaments[tournament].human_players];
                let ai_player = AI_PLAYER;
                while (temp_players.length !== g_tournaments[tournament].max_players)
                    temp_players.push(ai_player--);
                while (temp_players.length > 0) {
                    const index = Math.floor(Math.random() * temp_players.length);
                    g_tournaments[tournament].players_in_round.push(temp_players[index]);
                    temp_players.splice(index, 1);

                }
                g_tournaments[tournament].winners = [];
                g_tournaments[tournament].winners_length = 0;

                for (let i = 0; i < g_tournaments[tournament].players_in_round.length; i += 2) {
                    const player1 = g_tournaments[tournament].players_in_round[i + 0];
                    const player2 = g_tournaments[tournament].players_in_round[i + 1];
                    const new_game = new Game(player1, player2, g_tournaments[tournament], i >> 1);
                    if (player1 > AI_PLAYER) {
                        g_clients[player1].game = new_game;
                        g_clients[player1].client.send(JSON.stringify({
                            type: 'init-game',
                            player: PLAYER1
                        }));
                    } if (player2 > AI_PLAYER) {
                        g_clients[player2].game = new_game;
                        g_clients[player2].client.send(JSON.stringify({
                            type: 'init-game',
                            player: PLAYER2
                        }));
                    }
                }
                let players_in_first_round = "";
                for (let id of g_tournaments[tournament].players_in_round)
                    players_in_first_round += id + " ";
                console.log("Players in first round: " + players_in_first_round);
                msg['response'] = 'Starting tournament';

                // broadcast to everyone
                tournamentBroadcastState(g_tournaments[tournament], 'start-tournament', g_clients[id].nick, id);
            }
            socket.send(JSON.stringify(msg));
            console.log('start ' + tournament);
            break;

        case 'start-split-screen':
            if ( ! check_api_token(id, msg))
                break;
            else if (tournament)
                msg['error'] = 'You are already in tournament: ' + tournament;
            else if (game)
                msg['error'] = 'Already playing in split screen';
            else if ( ! msg.points || !(1 <= msg.points && msg.points <= MAX_POINTS))
                msg['error'] = 'Invalid points. Expected 1-' + MAX_POINTS + ', got: ' + msg.points;
            else {

                g_clients[id].game = new Game(id, id, msg.points);
                g_clients[id].client.send(JSON.stringify({
                    type: 'init-game',
                    player: SPLIT_SCREEN
                }));
                msg['response'] = 'Starting split screen';
				console.log("Starting split screen game\n");
				g_clients[id].game.player1.id = id;
				g_clients[id].game.player2.id = id;
				g_clients[id].game.is_split_screen = true;
                start_game(g_clients[id].game);
            }
            socket.send(JSON.stringify(msg));
            break;

        case 'change-nick':
            if ( ! check_api_token(id, msg))
                break;
            else if ( ! msg.nick)
                msg['error'] = 'New nick cannot be empty';
            else if (invalid_spaces(msg.nick))
                msg['error'] = 'Nick cannot have trailing or leading whitespace';
            else {
                const old_nick = g_clients[id].nick ? g_clients[id].nick : 'Anonymous';
                g_clients[id].nick = msg.nick;

                tournamentBroadcastState(
                    g_tournaments[tournament],
                    'change-nick',
                    msg.nick,
                    id,
                    old_nick);
            }
            break;

        case 'ready':
            if ( ! check_api_token(id, msg))
                break;
            else if ( ! tournament)
                msg['error'] = 'You are not in a tournament';
            else if ( ! game)
                msg['error'] = 'You are not assigned to a game';
            else {
                g_clients[id].ready = true;
                const player1_ready = game.player1.id <= AI_PLAYER || g_clients[game.player1.id].ready;
                const player2_ready = game.player2.id <= AI_PLAYER || g_clients[game.player2.id].ready;
                if (player1_ready && player2_ready)
                    start_game(game);
                msg['response'] = 'Starting game';
            }
            socket.send(JSON.stringify(msg));
            break;

        case 'quit':
            if ( ! check_api_token(id, msg))
                break;
            else {
                if (tournament) {

                    msg['response'] = 'Quitted from tournament: ' + tournament;

                } else if (game) {
                    msg = {
                        type: 'split-screen-winner',
                        winner: game.player1.points === game.player2.points ?
                            'tie'
                          : game.player1.points > game.player2.points ?
                            PLAYER1
                          : PLAYER2
                    };
                }

                quit(id);
            }

            socket.send(JSON.stringify(msg));
            break;

        case 'query-tournaments':
            if ( ! check_api_token(id, msg))
                break;

            msg['tournaments'] = [];
            let i_tourn = 0;
            for (let tourn in g_tournaments)
                msg['tournaments'][i_tourn++] = getTournamentState(
                    g_tournaments[tourn],
                    'response',
                    getNick(g_tournaments[tourn].creator),
                    g_tournaments[tourn].creator,
                    tourn);

            msg['current-tournament'] = tournament ?
                getTournamentState(
                    g_tournaments[tournament],
                    'response',
                    getNick(g_tournaments[tournament].creator),
                    g_tournaments[tournament].creator,
                    tournament)
              : null;

            socket.send(JSON.stringify(msg));
            break;

        case 'move-paddle':
            if ( ! check_api_token(id, msg))
                break;

            if (game && game.player1.id !== game.player2.id) {
                if (id === game.player1.id) {
                    game.player1.x = msg.x;
                    game.player1.y = msg.y;
                } else if (id === game.player2.id) {
                    game.player2.x = msg.x;
                    game.player2.y = msg.y;
                }
                game.usingMouse = true;
            }
            break;

        case 'keydown':
            if ( ! check_api_token(id, msg))
                break;

            g_clients[id].keysPressed[msg.key] = 1;
            if (['w', 'a', 's', 'd', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(msg.key)
                && game)
                    game.usingMouse = false;
            break;

        case 'keyup':
            if ( ! check_api_token(id, msg))
                break;

            g_clients[id].keysPressed[msg.key] = 0;
            break;

        case 'keypress':
            if ( ! check_api_token(id, msg))
                break;

            break;

        default:
            msg['error'] = 'Unknown request: ' + msg.type;
            socket.send(JSON.stringify(msg));
        }}
    }
})});

// Run the server
fastify.listen({ port: 3001, host: '0.0.0.0' }, (err, address) => {
    if (err) {
        fastify.log.error(err)
        process.exit(1)
    }
    initDB();
});

function broadcast(msg)
{
    fastify.websocketServer.clients.forEach(function each(client) {
        if (client.readyState === 1) {
            client.send(JSON.stringify(msg));
        }
    });
}

// ----------------------------------------------------------------------------
// Game

function vec3_length(v) {
    return Math.sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

function vec3_normalize(v) {
    const length = vec3_length(v);
    return { x: v.x/length, y: v.y/length, z: v.z/length };
}

function update_prediction(game)
{
    function fract(x) {
        return x - Math.floor(x);
    };
    function tri(x) { // triangle wave
        return 2.*Math.abs(2.*fract(.25*x - .25) - 1.) - 1.;
    };
    function predict(pos, slope) {
        if (game.player1.id <= AI_PLAYER)
            return -tri(+slope*(game.ball.z + 3) - pos);
        else
            return tri(-slope*(game.ball.z - 3) + pos);
    };

    const randomization = .2;
    const rand = {
        x: randomization * (2.*Math.random() - 1),
        y: randomization * (2.*Math.random() - 1)
    };

    game.prediction.x = rand.x + predict(game.ball.x, game.ball_dir.x/game.ball_dir.z);
    game.prediction.y = rand.y + predict(game.ball.y, game.ball_dir.y/game.ball_dir.z);
}

function init_ball(game, winner)
{
    if (winner !== null) { // update score
        winner.points++;
        if (game.player1.id > AI_PLAYER)
            g_clients[game.player1.id].client.send(JSON.stringify({
                type: 'game-score',
                player1: game.player1.points,
                player2: game.player2.points}));
        if (game.player2.id > AI_PLAYER)
            g_clients[game.player2.id].client.send(JSON.stringify({
                type: 'game-score',
                player1: game.player1.points,
                player2: game.player2.points}));
    }
    game.restarting = false;
    game.ball.x = 0;
    game.ball.y = 0;
    game.ball.z = 0;
    game.ball_dir.x = 2.*Math.random() - 1.;
    game.ball_dir.y = 2.*Math.random() - 1.;
    game.ball_dir.z = 2.*Math.random() - 1.;
    game.ball_dir.z = Math.sign(game.ball_dir.z); // limit to sensible angles
    game.ball_dir = vec3_normalize(game.ball_dir);
    game.ball_current_velocity = .5*BALL_VELOCITY;
    update_prediction(game);
}

function move_ball(game) {
    game.ball.x += game.ball_current_velocity*game.ball_dir.x;
    game.ball.y += game.ball_current_velocity*game.ball_dir.y;
    game.ball.z += game.ball_current_velocity*game.ball_dir.z;
}

function update_game_state(game)
{
    if (game.player1.id <= AI_PLAYER && game.player2.id <= AI_PLAYER) { // both disconnected
        if (game.player1.points > game.player2.points)
            game.player1.points = game.points;
        else
            game.player2.points = game.points;
        return check_points(game); // move the winning AI to the next round
    }

    if (game.player1.id > AI_PLAYER)
        g_clients[game.player1.id].client.send(JSON.stringify(getGameState(game, PLAYER1)));
    if (game.player2.id > AI_PLAYER && game.player2.id !== game.player1.id)
        g_clients[game.player2.id].client.send(JSON.stringify(getGameState(game, PLAYER2)));

    return true;
}

function check_points(game)
{
    const player1_playing = game.player1.id > AI_PLAYER;
    const player2_playing = game.player2.id > AI_PLAYER;
    const anybody_playing = player1_playing || player2_playing;

    if (anybody_playing && Math.max(game.player1.points, game.player2.points) < game.points)
        return true;
    // else advance winner

    if (game.player1.id > AI_PLAYER)
        g_clients[game.player1.id].game = null;
    if (game.player2.id > AI_PLAYER)
        g_clients[game.player2.id].game = null;

    if (game.is_split_screen || game.tournament === null)
    { // split screen
        if (game.player1.id > AI_PLAYER/*not disconnected*/&&
            Math.max(game.player1.points, game.player2.points) >= game.points)
		{
            g_clients[game.player1.id].client.send(JSON.stringify({
                type: 'split-screen-winner',
                winner: game.player1.points > game.player2.points ? PLAYER1 : PLAYER2,
            }));
			console.log("Split-screen winner sent a message\n");
		}
        return false;
    }

    const winner = game.player1.points >= game.player2.points ? game.player1.id : game.player2.id;
    const loser  = game.player1.points  < game.player2.points ? game.player1.id : game.player2.id;
    const index  = game.index_in_tournament;
    const winner_loser = { winner: winner, loser: loser };
    game.tournament.winners[index] = winner;
    game.tournament.winners_length++;

    if (game.tournament.players_in_round.length === 2) { // end tournament
        game.tournament.players_in_round = [];
        setTimeout(tournamentBroadcastState, 100, game.tournament, 'won', getNick(winner), winner_loser);
        return false;
    }
    if (winner > AI_PLAYER)
        g_clients[winner].client.send(JSON.stringify({
            type: 'game-result',
            winner: getNick(winner),
            winner_id: winner,
            loser: getNick(loser),
            loser_id: loser,
        }))
    if (loser > AI_PLAYER)
        g_clients[loser].client.send(JSON.stringify({
            type: 'game-result',
            winner: getNick(winner),
            winner_id: winner,
            loser: getNick(loser),
            loser_id: loser,
        }))
    tournamentBroadcastState(game.tournament, 'advanced', getNick(winner), winner_loser);

    if (game.tournament.winners_length === game.tournament.players_in_round.length >> 1)
    { // start new round
        game.tournament.players_in_round = game.tournament.winners;
        game.tournament.winners = [];
        game.tournament.winners_length = 0;

        for (let i = 0; i < game.tournament.players_in_round.length; i += 2) {
            const player1 = game.tournament.players_in_round[i + 0];
            const player2 = game.tournament.players_in_round[i + 1];

            console.log("Players going to next round: ", + player1 + " " + player2);

            const new_game = new Game(player1, player2, game.tournament, i >> 1);
            if (player1 > AI_PLAYER) {
                g_clients[player1].game = new_game;
                g_clients[player1].client.send(JSON.stringify({
                    type: 'init-game',
                    player: PLAYER1
                }));
            } if (player2 > AI_PLAYER) {
                g_clients[player2].game = new_game;
                g_clients[player2].client.send(JSON.stringify({
                    type: 'init-game',
                    player: PLAYER2
                }));
            }
        }
    }
    return false;
}

function pong(game)
{
    if (game.player1.id !== game.player2.id) {
        if (game.player1.id > AI_PLAYER) {
            game.player1.y += PADDLE_VELOCITY * (g_clients[game.player1.id].keysPressed['w'] || g_clients[game.player1.id].keysPressed['ArrowUp']);
            game.player1.x -= PADDLE_VELOCITY * (g_clients[game.player1.id].keysPressed['a'] || g_clients[game.player1.id].keysPressed['ArrowLeft']);
            game.player1.y -= PADDLE_VELOCITY * (g_clients[game.player1.id].keysPressed['s'] || g_clients[game.player1.id].keysPressed['ArrowDown']);
            game.player1.x += PADDLE_VELOCITY * (g_clients[game.player1.id].keysPressed['d'] || g_clients[game.player1.id].keysPressed['ArrowRight']);
        }
        if (game.player2.id > AI_PLAYER) {
            game.player2.y += PADDLE_VELOCITY * (g_clients[game.player2.id].keysPressed['w'] || g_clients[game.player2.id].keysPressed['ArrowUp']);
            game.player2.x -= PADDLE_VELOCITY * (g_clients[game.player2.id].keysPressed['a'] || g_clients[game.player2.id].keysPressed['ArrowLeft']);
            game.player2.y -= PADDLE_VELOCITY * (g_clients[game.player2.id].keysPressed['s'] || g_clients[game.player2.id].keysPressed['ArrowDown']);
            game.player2.x += PADDLE_VELOCITY * (g_clients[game.player2.id].keysPressed['d'] || g_clients[game.player2.id].keysPressed['ArrowRight']);
        }
    } else if (game.player1.id > AI_PLAYER) { // split screen
        game.player1.y += PADDLE_VELOCITY * g_clients[game.player1.id].keysPressed['w'];
        game.player1.x -= PADDLE_VELOCITY * g_clients[game.player1.id].keysPressed['a'];
        game.player1.y -= PADDLE_VELOCITY * g_clients[game.player1.id].keysPressed['s'];
        game.player1.x += PADDLE_VELOCITY * g_clients[game.player1.id].keysPressed['d'];

        game.player2.y -= PADDLE_VELOCITY * g_clients[game.player2.id].keysPressed['ArrowUp'];
        game.player2.x += PADDLE_VELOCITY * g_clients[game.player2.id].keysPressed['ArrowLeft'];
        game.player2.y += PADDLE_VELOCITY * g_clients[game.player2.id].keysPressed['ArrowDown'];
        game.player2.x -= PADDLE_VELOCITY * g_clients[game.player2.id].keysPressed['ArrowRight'];
    }

    if (update_game_state(game) && check_points(game))
    	setTimeout(pong, 1000 / 120, game);
    else // end game
        return;

	time = Date.now() - game.TIME_START;
	if (time <= 2000)
       	return;

    if (game.frame_counter++ % 120)
        game.prediction_for_ai = structuredClone(game.prediction);

    if (game.restarting) {
		move_ball(game);
		return;
 	}

 	const in_court = Math.abs(game.ball.z) <= 3.25;
 	if ( ! in_court) {
	    game.restarting = true;
		setTimeout(init_ball, 1000, game, game.ball.z >= 0 ? game.player1 : game.player2);
		return;
 	}

 	// ------------------------------------------------------------------------
 	// Physics

 	let hit = false;
 	const ball_dir_change   = 8.;
 	const ball_dir_change_z = 1.;

 	if (game.ball.z <= -3. && in_court)
 	{ // player 1 paddle
 	    const diff = { x: game.ball.x - game.player1.x, y: game.ball.y - game.player1.y };
 	    if (Math.abs(diff.x) <= .25 + .25/4 && Math.abs(diff.y) <= .25 + .25/4)
        { // regular hit
 	        game.ball_dir.x = ball_dir_change*diff.x*diff.x*Math.sign(diff.x);
 	        game.ball_dir.y = ball_dir_change*diff.y*diff.y*Math.sign(diff.y);
 	        game.ball_dir.z = ball_dir_change_z;
 	        game.ball_dir = vec3_normalize(game.ball_dir);
 	        game.ball_current_velocity = BALL_VELOCITY;
 	        hit = true;
            if (game.player1.id > AI_PLAYER)
 	            update_prediction(game);
 	    }
        else if (Math.abs(diff.x) <= .25 + .25/2 && Math.abs(diff.y) <= .25 + .25/2)
        { // edge hit
 	        game.ball_dir.x = ball_dir_change*diff.x;
 	        game.ball_dir.y = ball_dir_change*diff.y;
 	        game.ball_dir.z = game.ball.z + 3.;
 	        game.ball_dir = vec3_normalize(game.ball_dir);
 	        game.ball_current_velocity = BALL_VELOCITY;
 	        hit = true;
 	    }
        // else miss for this frame, could hit during the next one though
 	}
 	if (game.ball.z >= 3. && in_court)
 	{ // player 2 paddle
 	    const diff = { x: game.ball.x - game.player2.x, y: game.ball.y - game.player2.y };
 	    if (Math.abs(diff.x) <= .25 + .25/4 && Math.abs(diff.y) <= .25 + .25/4)
        { // regular hit
 	        game.ball_dir.x = ball_dir_change*diff.x*diff.x*Math.sign(diff.x);
 	        game.ball_dir.y = ball_dir_change*diff.y*diff.y*Math.sign(diff.y);
 	        game.ball_dir.z = -ball_dir_change_z;
 	        game.ball_dir = vec3_normalize(game.ball_dir);
 	        game.ball_current_velocity = BALL_VELOCITY;
 	        hit = true;
            if (game.player1.id <= AI_PLAYER)
 	            update_prediction(game);
 	    }
        else if (Math.abs(diff.x) <= .25 + .25/2 && Math.abs(diff.y) <= .25 + .25/2)
        { // edge hit
 	        game.ball_dir.x = ball_dir_change*diff.x;
 	        game.ball_dir.y = ball_dir_change*diff.y;
 	        game.ball_dir.z = game.ball.z - 3.;
 	        game.ball_dir = vec3_normalize(game.ball_dir);
 	        game.ball_current_velocity = BALL_VELOCITY;
 	        hit = true;
 	    }
        // else miss for this frame, could hit during the next one though
 	}
 	if (Math.abs(game.ball.x) >= 1.
 	    && Math.sign(game.ball_dir.x) === Math.sign(game.ball.x) && in_court)
 	{ // wall hit
 	    game.ball_dir.x = -game.ball_dir.x;

        if (Math.abs(game.ball.z) >= 3.)
        { // prevent too many edge hits
            game.ball.z += Math.sign(game.ball.z)*(5.*(Math.abs(game.ball.z) - 3.))**4;
            vec3_normalize(game.ball_dir);
        }
 	    hit = true;
 	}
 	if (Math.abs(game.ball.y) >= 1.
 	    && Math.sign(game.ball_dir.y) === Math.sign(game.ball.y) && in_court)
 	{ // floor or ceil hit
 	    game.ball_dir.y = -game.ball_dir.y;

        if (Math.abs(game.ball.z) >= 3.)
        { // prevent too many edge hits
            game.ball.z += Math.sign(game.ball.z)*(5.*(Math.abs(game.ball.z) - 3.))**4;
            vec3_normalize(game.ball_dir);
        }
 	    hit = true;
 	}
 	move_ball(game);
 	if (hit) { // slightly more explosive bounces
 	    move_ball(game);
 	}

 	// ------------------------------------------------------------------------
 	// AI

    let ai = null;
    if (game.player1.id <= AI_PLAYER)
        ai = game.player1;
    if (game.player2.id <= AI_PLAYER)
        ai = game.player2;
    if (ai !== null)
    {
        // Make AI only follow the ball when it gets close to it
        const follow = ai === game.player2 ? game.ball.z/6 + .5 : -(game.ball.z/6 - .5);

        if (game.usingMouse && game.ball_current_velocity <= .75*BALL_VELOCITY)
        { // ai shouldn't lose at the very beginning
            ai.x = (follow**4)*game.ball.x
            ai.y = (follow**4)*game.ball.y
            game.ball_prev[1] = game.ball_prev[0];
            game.ball_prev[0] = game.ball;
            game.ai_prev[1] = game.ai_prev[0];
            game.ai_prev[0] = ai;
        }
        else if (game.usingMouse)
        {
            const ai_speed = (follow**4)*.06;
            const alpha = 0.; // 0 for slight overshoot and sudden movement, 1 is sluggish
            const direction_k  = .35; // make it harder to catch balls coming in an angle
            const randomization = .45; // for trickshots and fuck ups

            const omega = 2.*Math.PI*ai_speed;

            // Biquad filtering for somewhat human response
            // https://www.w3.org/TR/audio-eq-cookbook/
            const b0 = (1. - Math.cos(omega))/2.;
            const b1 = (1. - Math.cos(omega));
            const b2 = (1. - Math.cos(omega))/2.;
            const a0 = (1. + alpha);
            const a1 = -2.*Math.cos(omega);
            const a2 = (1. - alpha);

            const rand_x = randomization*Math.sin(1.44444*time/1000)**71;
            const rand_y = randomization*Math.cos(1.77777*time/1000)**71;

            const ball_x = game.ball.x - direction_k*game.ball_dir.x + rand_x;
            const ball_y = game.ball.y - direction_k*game.ball_dir.y + rand_y;

            ai.x = (b0/a0)*ball_x + (b1/a0)*game.ball_prev[0].x + (b2/a0)*game.ball_prev[1].x
                                  - (a1/a0)*game.ai_prev[0].x   - (a2/a0)*game.ai_prev[1].x;

            ai.y = (b0/a0)*ball_y + (b1/a0)*game.ball_prev[0].y + (b2/a0)*game.ball_prev[1].y
                                  - (a1/a0)*game.ai_prev[0].y   - (a2/a0)*game.ai_prev[1].y;

            game.ball_prev[1] = game.ball_prev[0];
            game.ball_prev[0] = { x: ball_x, y: ball_y };
            game.ai_prev[1] = game.ai_prev[0];
            game.ai_prev[0] = ai;

        } else {
            const diff = {
                x: game.prediction_for_ai.x - ai.x,
                y: game.prediction_for_ai.y - ai.y
            };
            if (Math.abs(diff.x) > PADDLE_VELOCITY)
                ai.x += PADDLE_VELOCITY*Math.sign(diff.x);
            if (Math.abs(diff.y) > PADDLE_VELOCITY)
                ai.y += PADDLE_VELOCITY*Math.sign(diff.y);
        }
    }
}
