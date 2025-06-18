#include <gamestate.h>
#include <unistd.h>
#include <strings.h>
#include <string>
#include <iostream>
#include <memory>
#include <main.hpp>

extern "C" { void *rendering_loop(void *void_gamestate_ptr); }
extern "C" { char* make_raw(const char*); }
extern "C" { void clipong_set_input_event_hooks(Events* data); }
extern "C" { void disable_mouse_tracking_and_tcsetattr(void); }
extern "C" { void enable_mouse_tracking_and_tcsetattr(void); }

t_gamestate		g_gamestate                 = {};
int				g_create_tournament_state	= 0;
int				g_join_tournament_state		= 0;
int				g_tournament_state			= 0;
int				g_splitscreen_state			= 0;
int				g_init_game_state			= 0;
int				g_ready_state				= 0;
int				g_player_count				= 0;
int				g_register_state			= 0;
int				g_login_state				= 0;
int				g_select_menu_interrupt     = 0;
int             g_id                        = -1;
std::string     g_username                  = "";
int             g_user_id                   = -1;
std::string     g_api_token                 = "";
Events			g_events                    = {};
std::vector<t_menu_item>	g_open_tournaments_list;

double			g_last_update_time = 0;

// Tournament menu titles
const std::string g_create_tournament_base_title = "-- Tournament owner menu | Tourney not started | ";
const std::string g_join_tournament_base_title = "-- Waiting to be put into a lobby.. | ";
std::string g_create_tournament_title = "";
std::string g_join_tournament_title = "";

// Websocket stuff
ConnectionHdl g_connection{};
Client g_client{};

// Signal handler
// We need this to close the threads safely.
sig_atomic_t	g_should_quit = 0;
void clipong_sighandler(int signum)
{
	g_should_quit = signum;
}

void clear_screen()
{
    // Instead of erasing screen using escape sequences, we fill the screen with
    // spaces and move cursor back to origin. This prevents scrolling and saves
    // memory.

    static char* space_buf;
    static size_t space_buf_size;
    const char go_home[] = "\e[H";

    IVec2 term_size = tengi_get_terminal_size();
    if (term_size.x*term_size.y != (int)space_buf_size)
    {
        free(space_buf);
        space_buf_size = term_size.x*term_size.y;
        space_buf = (char*)malloc(space_buf_size + 2*strlen(go_home));
        memcpy(space_buf, go_home, strlen(go_home));
        memset(space_buf + strlen(go_home), ' ', space_buf_size);
        memcpy(space_buf + strlen(go_home) + space_buf_size, go_home, strlen(go_home));
    }
    // Writing a big thing, so must not use slow streams, use native instead
    int _ = write(STDOUT_FILENO, space_buf, space_buf_size + 2*strlen(go_home));
    (void)_;
}

void error_and_wait(const std::string &toprint, const unsigned int towait)
{
	std::cout << FG_BLACK << BG_RED << " ERROR: " << toprint << " " << C_END << std::endl;
	usleep(towait);
}

// This **SHOULD** get compiled into a pseudo hashmap
payload_type payload_type_hash(const std::string &type)
{
    if (type == "connected")
        return CONNECTED;
    if (type == "ping")
        return PING;
    if (type == "game-state")
        return GAME_STATE;
    if (type == "tournament-state")
        return TOURNAMENT_STATE;
    if (type == "create-tournament")
        return CREATE_TOURNAMENT;
    if (type == "join-tournament")
        return JOIN_TOURNAMENT;
    if (type == "init-game")
        return INIT_GAME;
    if (type == "ready")
        return READY;
    if (type == "sign-in")
        return LOGIN;
    if (type == "register")
        return REGISTER;
    if (type == "split-screen-winner")
        return SPLIT_SCREEN_WINNER;
    if (type == "start-split-screen")
        return SPLIT_SCREEN_START;
    if (type == "query-tournaments")
        return QUERY;
    return UNKNOWN;
}

void pong(void)
{
	send_message(&g_client, &g_connection, "{\"type\":\"pong\"}");
}


void send_mouse_paddlestate()
{
	static double mouse_old_x = 0;
	static double mouse_old_y = 0;

	// Nothing to send
	if (mouse_old_x == g_events.mouse_move[0] && mouse_old_y == g_events.mouse_move[1])
		return ;

	json	j;
	j["type"] = "move-paddle";
	mouse_old_x = g_events.mouse_move[0];
	mouse_old_y = g_events.mouse_move[1];
	j["x"] = mouse_old_x;
	j["y"] = mouse_old_y;
	send_api_message(j);
}

void r_connected(json &json_payload)
{
    for (auto& [key, value] : json_payload.items())
    {
        if (key == "player_id")
            g_id = value;
    }
}

void r_update_game_state(json &json_payload)
{
    t_gamestate gamestate = {};
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "ball")
		{
			gamestate.ball.x = value["x"];
			gamestate.ball.y = value["y"];
			gamestate.ball.z = value["z"];
		}
		else if (key == "player1")
		{
			gamestate.player1.x = value["x"];
			gamestate.player1.y = value["y"];
		}
		else if (key == "player2")
		{
			gamestate.player2.x = value["x"];
			gamestate.player2.y = value["y"];
		}
	}
	pthread_mutex_lock(&(g_gamestate.lock));
    g_gamestate.ball    = gamestate.ball;
    g_gamestate.player1 = gamestate.player1;
    g_gamestate.player2 = gamestate.player2;
	pthread_mutex_unlock(&(g_gamestate.lock));

	/* PRINTING THE GAMESTATE FOR DEBUGGING PURPOSES
	clear_screen();
	std::cout << "\nball:\n"
		<< "\t" << g_gamestate.ball.x << "\n"
		<< "\t" << g_gamestate.ball.y << "\n"
		<< "\t" << g_gamestate.ball.z << "\n"
		<< "player1:\n"
		<< "\t" << g_gamestate.player1.x << "\n"
		<< "\t" << g_gamestate.player1.y << "\n"
		<< "player2:\n"
		<< "\t" << g_gamestate.player2.x << "\n"
		<< "\t" << g_gamestate.player2.y << "\n";
	*/
}

void r_ready(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "response")
		{
			if (value == "Starting game")
			{
				g_ready_state = 1;
				return ;
			}
		}
	}
}

void r_init_game(json &json_payload)
{
	g_init_game_state = 1;
	g_select_menu_interrupt = 1;
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "player")
			g_gamestate.player = value;
	}
}

// Sets tournament_state as 1 if we are about to enter another game
// 2 if we won the tournament
// -1 if we lost or the tournament is over
void r_update_tournament_state(json &json_payload)
{
	bool	somebody_won	= false;
	bool	advancing		= false;
	bool	our_match		= false;
	bool	we_advanced		= false;

	for (auto& [key, value] : json_payload.items())
	{
		//std::cout << key << " :: " << value << std::endl;
		if (key == "msg")
		{
			if (value == "won")
				somebody_won = true;
			else if (value == "advanced")
				advancing = true;
		}
		else if (key == "players_advancing_ids")
		{
            for (auto& subvalue : value)
            {
				if (subvalue == g_id)
					we_advanced = true;
            }
		}
		else if (key == "player_id")
		{
            for (auto& [subkey, subvalue]	: value.items())
            {
				(void) subkey;
				if (subvalue == g_id)
					our_match = true;
            }
		}
		else if (key == "players_ids")
		{
            g_player_count = value.size();

			// Update playercounts on menus
			g_create_tournament_title = g_create_tournament_base_title + std::to_string(g_player_count) + " players joined --\n";
			g_join_tournament_title = g_join_tournament_base_title + std::to_string(g_player_count) + " players joined --\n";
			g_select_menu_interrupt = 1; // Refresh menu
		}
	}
	// Order is important, it turns out :)
	if (somebody_won)
	{
		if (we_advanced)
			g_tournament_state = 2;
		else
			g_tournament_state = -1;
		return ;
	}

	if ( ! our_match || ! advancing)
		return ;

	if (we_advanced)
		g_tournament_state = 1;
	else
		g_tournament_state = -1;

}

void r_game_result(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "winner_id")
		{
			if (value == g_id)
				g_tournament_state = 1;
			else
				g_tournament_state = -1;
		}
	}
}

void r_create_tournament(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "response")
		{
			std::cout << value << std::endl;
			g_create_tournament_state = 1;	// Succesful creation
		}
		else if (key == "error")
		{
            clear_screen();
			std::cout << value << std::endl;
			g_create_tournament_state = -1; // Failed creation
			return;
		}
	}
}


void r_join_tournament(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "response")
		{
			std::cout << value << std::endl;
			g_join_tournament_state = 1;	// Succesful creation
		}
		else if (key == "error")
		{
            clear_screen();
			std::cout << value << std::endl;
			g_join_tournament_state = -1; // Failed creation
			return;
		}
	}
}

void r_login(json &json_payload)
{
    bool success = false;
    int user_id = -1;
    std::string api_token = "";

	for (auto& [key, value] : json_payload.items())
	{
		if (key == "success")
		{
			g_login_state = 1;
            success = true;
		}
        else if (key == "user")
        {
            user_id = value["id"];
        }
        else if (key == "api_token")
        {
            api_token = value;
        }
		else if (key == "error")
		{
			clear_screen();
			std::cout << value << std::endl;
			g_login_state = -1; // Failed creation
			return;
		}
	}

    if (success)
    {
        g_user_id = user_id;
        g_api_token = api_token;
    }
}

void r_register(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "success")
		{
			g_register_state = 1;
		}
		else if (key == "error")
		{
			clear_screen();
			std::cout << value << std::endl;
			g_register_state = -1; // Failed creation
			return;
		}
	}
}


void r_splitscreen_winner(json &json_payload)
{
    for (auto& [key, value] : json_payload.items())
    {
        if (key == "winner")
        {
            if (value.is_number())
                g_tournament_state = value;
            else
                g_tournament_state = -1; // tie in case of quit, safe to ignore
        }
    }

	g_splitscreen_state = 1;
	send_api_message("quit");
}

void r_splitscreen_start(json &json_payload)
{
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "response")
		{
			g_splitscreen_state = 1;
			return;
		}
		else if (key == "error")
		{
			clear_screen();
			std::cout << value << std::endl;
			g_splitscreen_state = -1;
			return;
		}
	}
}

void r_query(json &json_payload)
{
    for (auto& tournament : json_payload["tournaments"])
    {
        t_menu_item temp;

        temp.name = tournament["extra_data"];
		temp.fg_color	= FG_CYAN;
		temp.bg_color	= BG_BLACK;
		g_open_tournaments_list.push_back(temp);
    }

    json null;
    if (json_payload["current-tournament"] != null)
        g_player_count = json_payload["current-tournament"]["players"].size();
}

// Receives the payload and figures out what to do with it
void json_handler(std::string &string_payload)
{
	json json_payload = json::parse(string_payload);

	// Iterate through all of the key/value pairs
	for (auto& [key, value] : json_payload.items())
	{
		if (key == "type")
		{
			payload_type pty = payload_type_hash(value);

            try {
                switch (pty)
                {
                    case CONNECTED:
                        r_connected(json_payload); return;
                    case PING:
                        pong(); return;
                    case GAME_STATE:
                        r_update_game_state(json_payload); return;
                    case TOURNAMENT_STATE:
                        r_update_tournament_state(json_payload); return;
                    case CREATE_TOURNAMENT:
                        r_create_tournament(json_payload); return;
                    case JOIN_TOURNAMENT:
                        r_join_tournament(json_payload); return;
                    case INIT_GAME:
                        r_init_game(json_payload); return;
                    case READY:
                        r_ready(json_payload); return;
                    case LOGIN:
                        r_login(json_payload); return;
                    case REGISTER:
                        r_register(json_payload); return;
                    case SPLIT_SCREEN_WINNER:
                        r_splitscreen_winner(json_payload); return;
                    case SPLIT_SCREEN_START:
                        r_splitscreen_start(json_payload); return;
                    case QUERY:
                        r_query(json_payload); return;
                    default:
                        break;
                }
            } catch (json::exception& e) { // if going to crash anyway, at least tell us where
                std::cerr << "JSON exception in response of type " << value << std::endl;
                std::cerr << e.what() << std::endl;
                exit(EXIT_FAILURE);
            }
		}
	}
}

static inline void init_gamestate(void)
{
	bzero(&g_gamestate, sizeof(t_gamestate));
	g_gamestate.player = 1;
	g_gamestate.selected_accelerator = -1;
}

static inline void print_logo(void)
{
	std::cout
		<< C_RED
		<< "▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n"
		<< "██▙ ▟█▙ █▖ █ ▟█▙\n"
		<< "█ █ █ █ ██▖█ █ ▀\n"
		<< "██▛ █ █ █▝██ █▝█\n"
		<< "█   ▜█▛ █ ▝█ ▜█▜\n"
		<< "▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄\n"
		<< C_END
		<< std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "\nInvalid format; use: ./<program> <hostname>\n";
		std::cerr << "Example: ./clipong localhost:3001//\n" << std::endl;
		return 1;
	}

    tengi_init(); // auto destroyed

	clear_screen();
	print_logo();

	signal(SIGINT, clipong_sighandler);

	// Init everything except the lock
	init_gamestate();

	// Init gamestate mutex lock
	if (pthread_mutex_init(&(g_gamestate.lock), NULL) != 0)
		{ std::cerr << "Mutex failed" << std::endl; return 1; }

	// Create thread for rendering
	pthread_t	render_thread;
	if (pthread_create(&render_thread, NULL, rendering_loop, (void*)&g_gamestate) != 0)
	{
		pthread_mutex_destroy(&g_gamestate.lock);
		std::cerr << "Thread creation failed" << std::endl;
		return 1;
	}

	// WEBSOCKET SHENANIGANS ==================================================
	turn_off_logging(g_client);

	g_client.init_asio();

	set_tls_init_handler(g_client);
	set_open_handler(g_client, &g_connection);
	set_message_handler(g_client);

	std::string hostname(argv[1]);
	set_url(g_client, "wss://" + hostname);

	websocketpp::lib::thread t1(&Client::run, &g_client);
	// ========================================================================

	usleep(1500 * 1000); // Give the websocket some time to connect

    if (!g_is_connected)
    {
        g_should_quit = true;
        g_gamestate.exit_thread = true;
        pthread_join(render_thread, NULL); // prevent clearing screen before error message shown
        std::cout << "Websocket connection failed" << std::endl;
    }
    else
    {
        clipong_set_input_event_hooks(&g_events);
        login_menu(); // Handles logging in and all that jazz
    }

	// MAIN MENU LOOP =========================================================
	int index;
	std::vector<t_menu_item> main_menu;

	// Yeah this is big dumb dumb but it only runs once so idc for now
	t_menu_item temp;

	temp.name		= "Create tournament   ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	temp.name		= "Join tournament     ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	temp.name		= "Split screen        ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	temp.name		= "Exit                ";
	temp.fg_color	= FG_RED;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	std::string title = " -- Main menu | You are not in a tournament -- \n";

	while (!g_should_quit)
	{
		index = select_from_menu(main_menu, title);
		switch (index)
		{
			case 0: // CREATE
				create_tournament();
				break;
			case 1:
				join_tournament();
				break;
			case 2:
				create_splitscreen();
				break;
			case 3: // EXIT
				g_should_quit = true;
				break;
		}
	}
	// ------------------------------------------------------------------------

	// Tell render thread to quit
	pthread_mutex_lock(&(g_gamestate.lock));
	g_gamestate.exit_thread = true;
	g_gamestate.game_running = false;
	pthread_mutex_unlock(&(g_gamestate.lock));

	pthread_join(render_thread, NULL);
	pthread_mutex_destroy(&g_gamestate.lock);

	// Join websocket thread
	close_connection(&g_client, &g_connection);
	t1.join();

	return (0);
}
