#include <main.hpp>

std::string g_enter_tournament_title;

enum KeyEventType
{
    KEY_PRESSED,
    KEY_RELEASED,
};

static inline std::string make_key_message_buffer(
    const std::string& type, const std::string& key)
{
    json   j;
    j["type"]      = type;
    j["key"]       = key;
    j["user_id"]   = g_user_id;
    j["api_token"] = g_api_token;
    return j.dump();
}

static inline void handle_in_game_inputs()
{
	tengi_handle_events();

    constexpr size_t keys_events_length = sizeof g_events.keys_pressed / sizeof(char*);
    static std::string msgs[2][keys_events_length];
    if (msgs[0][0].empty()) { // must init lazily, we don't have user id nor api token at compile time
        msgs[KEY_PRESSED ][4] = make_key_message_buffer("keydown", "ArrowUp");
        msgs[KEY_RELEASED][4] = make_key_message_buffer("keyup",   "ArrowUp");
        msgs[KEY_PRESSED ][5] = make_key_message_buffer("keydown", "ArrowDown");
        msgs[KEY_RELEASED][5] = make_key_message_buffer("keyup",   "ArrowDown");
        msgs[KEY_PRESSED ][6] = make_key_message_buffer("keydown", "ArrowLeft");
        msgs[KEY_RELEASED][6] = make_key_message_buffer("keyup",   "ArrowLeft");
        msgs[KEY_PRESSED ][7] = make_key_message_buffer("keydown", "ArrowRight");
        msgs[KEY_RELEASED][7] = make_key_message_buffer("keyup",   "ArrowRight");
        msgs[KEY_PRESSED ][0] = make_key_message_buffer("keydown", "w");
        msgs[KEY_RELEASED][0] = make_key_message_buffer("keyup",   "w");
        msgs[KEY_PRESSED ][1] = make_key_message_buffer("keydown", "a");
        msgs[KEY_RELEASED][1] = make_key_message_buffer("keyup",   "a");
        msgs[KEY_PRESSED ][2] = make_key_message_buffer("keydown", "s");
        msgs[KEY_RELEASED][2] = make_key_message_buffer("keyup",   "s");
        msgs[KEY_PRESSED ][3] = make_key_message_buffer("keydown", "d");
        msgs[KEY_RELEASED][3] = make_key_message_buffer("keyup",   "d");
    }

	if (g_events.keys_pressed[4]) // Up
		send_message(&g_client, &g_connection, msgs[KEY_PRESSED][4]);
	else if (g_events.keys_released[4])
		send_message(&g_client, &g_connection, msgs[KEY_RELEASED][4]);

	if (g_events.keys_pressed[5]) // Down
		send_message(&g_client, &g_connection, msgs[KEY_PRESSED][5]);
	else if (g_events.keys_released[5])
		send_message(&g_client, &g_connection, msgs[KEY_RELEASED][5]);

	if (g_events.keys_pressed[6]) // Left
		send_message(&g_client, &g_connection, msgs[KEY_PRESSED][6]);
	else if (g_events.keys_released[6])
		send_message(&g_client, &g_connection, msgs[KEY_RELEASED][6]);

	if (g_events.keys_pressed[7]) // Right
		send_message(&g_client, &g_connection, msgs[KEY_PRESSED][7]);
	else if (g_events.keys_released[7])
		send_message(&g_client, &g_connection, msgs[KEY_RELEASED][7]);

	// We only care about this if we have 2 players :P
	if (g_gamestate.player == SPLIT_SCREEN)
	{
		if (g_events.keys_pressed[0]) // W
			send_message(&g_client, &g_connection, msgs[KEY_PRESSED][0]);
		else if (g_events.keys_released[0])
			send_message(&g_client, &g_connection, msgs[KEY_RELEASED][0]);

		if (g_events.keys_pressed[1]) // A
			send_message(&g_client, &g_connection, msgs[KEY_PRESSED][1]);
		else if (g_events.keys_released[1])
			send_message(&g_client, &g_connection, msgs[KEY_RELEASED][1]);

		if (g_events.keys_pressed[2]) // S
			send_message(&g_client, &g_connection, msgs[KEY_PRESSED][2]);
		else if (g_events.keys_released[2])
			send_message(&g_client, &g_connection, msgs[KEY_RELEASED][2]);

		if (g_events.keys_pressed[3]) // D
			send_message(&g_client, &g_connection, msgs[KEY_PRESSED][3]);
		else if (g_events.keys_released[3])
			send_message(&g_client, &g_connection, msgs[KEY_RELEASED][3]);
	}

	if (g_gamestate.player != SPLIT_SCREEN && g_events.mouse_move)
		send_mouse_paddlestate();

	// Bzero is smarter but this is faster + more importantly I do not care
	g_events.keys_pressed[0] = 0;
	g_events.keys_pressed[1] = 0;
	g_events.keys_pressed[2] = 0;
	g_events.keys_pressed[3] = 0;
	g_events.keys_pressed[4] = 0;
	g_events.keys_pressed[5] = 0;
	g_events.keys_pressed[6] = 0;
	g_events.keys_pressed[7] = 0;

	g_events.keys_released[0] = 0;
	g_events.keys_released[1] = 0;
	g_events.keys_released[2] = 0;
	g_events.keys_released[3] = 0;
	g_events.keys_released[4] = 0;
	g_events.keys_released[5] = 0;
	g_events.keys_released[6] = 0;
	g_events.keys_released[7] = 0;
}

void in_game()
{
    // Fix key stuck bug on the server
    g_events.keys_pressed[0]  = (char*)0;
    g_events.keys_pressed[1]  = (char*)0;
    g_events.keys_pressed[2]  = (char*)0;
    g_events.keys_pressed[3]  = (char*)0;
    g_events.keys_pressed[4]  = (char*)0;
    g_events.keys_pressed[5]  = (char*)0;
    g_events.keys_pressed[6]  = (char*)0;
    g_events.keys_pressed[7]  = (char*)0;
    g_events.keys_released[0] = (char*)1;
    g_events.keys_released[1] = (char*)1;
    g_events.keys_released[2] = (char*)1;
    g_events.keys_released[3] = (char*)1;
    g_events.keys_released[4] = (char*)1;
    g_events.keys_released[5] = (char*)1;
    g_events.keys_released[6] = (char*)1;
    g_events.keys_released[7] = (char*)1;

	g_gamestate.game_running = true;
	tengi_handle_events();
    tengi_grab_keyboard();
	enable_mouse_tracking_and_tcsetattr();
	while (!g_should_quit && g_tournament_state == 0 && !g_events.exiting_game)
	{
		handle_in_game_inputs();
        // Wait similar order of magnitude to frame rate to not send too many
        // events to the server.
		usleep(1000.*1000./120.);
	}
	g_events.exiting_game = false;
	g_gamestate.game_running = false;
	usleep(1000);
	disable_mouse_tracking_and_tcsetattr();
	tengi_handle_events();
    tengi_ungrab_keyboard();
	clear_screen();
}

// This function is what runs when we are inside the tournament/ingame
int enter_tournament()
{
	g_tournament_state = 0;
	enable_mouse_tracking_and_tcsetattr();
	clear_screen();

	// TOURNAMENT OWNER ROOT MENU =============================================
	int index;
	std::vector<t_menu_item> main_menu;

	// Yeah this is big dumb dumb
	t_menu_item temp;

	temp.name		= "Ready to start match";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	temp.name		= "Exit tournament     ";
	temp.fg_color	= FG_RED;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	g_enter_tournament_title = " -- Tournament lobby menu | You have been paired to a match -- \n";

	while ( g_should_quit == false )
	{
		index = select_from_menu(main_menu, g_enter_tournament_title);
		if (index == -1)
			continue ;
		if (index == 0) // READY
		{
			send_api_message("ready");

			std::cout << "\nWaiting for others to ready up..." << std::endl;
			while (!g_should_quit && g_ready_state == 0) // Wait for start
				usleep(10000);
			if (g_should_quit)
				return 1;
			if (g_ready_state != 1)
				{ g_ready_state = 0; return 1; }

			clear_screen();
			g_ready_state = 0;
			in_game();		// This will block until another ready state is received

			// For some reason this is unreliable so I'm overdoign it :P
			usleep(100000); clear_screen();
			usleep(100000); clear_screen();
			switch (g_tournament_state)
			{
				case -1:
					std::cout << "You were defeated D:" << std::endl;
					usleep(2500 * 1000);
					g_tournament_state = 0;
					return 1;
				case 1:
					std::cout << "You won this match :D" << std::endl;
					usleep(2500 * 1000);
					g_tournament_state = 0;
					return 0;
				case 2:
					std::cout << "You won this tournament :DDDDD" << std::endl;
					usleep(2500 * 1000);
					g_tournament_state = 0;
					return 1;
				default:
					g_tournament_state = 0;
					return 1;
			}
		}
		else // EXIT
				return 1;
	}
	// ------------------------------------------------------------------------
	disable_mouse_tracking_and_tcsetattr();
	return 1;
}
