#include <main.hpp>

void join_tournament()
{
	// Just in case :P
	send_api_message("quit");

	g_init_game_state = 0; // Reset this just in case there's garbage here
	disable_mouse_tracking_and_tcsetattr();

	while ( ! g_should_quit )
	{
		usleep(1000);
		clear_screen();

		std::string	nick;
		std::cout << FG_BLACK << BG_GREEN << " -> Press enter on an empty field to go back <- " << C_END << std::endl;
		std::cout << "\nEnter your alias: ";
		std::getline(std::cin, nick);
		if (nick.empty())
			return;

		clear_screen();

		// TOURNAMENT LIST MENU ===============================================
		int index;

		t_menu_item		temp;
		temp.name		= "Exit                ";
		temp.fg_color	= FG_RED;
		temp.bg_color	= BG_BLACK;

		std::string title = "-- List of open tournaments | click to join, ESC to refresh --\n";
		while ( ! g_should_quit )
		{
			g_open_tournaments_list.clear();
			std::cout << "Requesting list of open tournaments from server..." << std::endl;
            send_api_message("query-tournaments");
			usleep(100000);

			g_open_tournaments_list.push_back(temp);

			std::cout << BG_GREEN << FG_BLACK << title << C_END << std::endl;
			index = select_from_menu(g_open_tournaments_list, title);
			if (g_should_quit || index + 1 == (int) g_open_tournaments_list.size())
				return ;
			if (index < 0 || index >= (int) g_open_tournaments_list.size())
				continue ;
			else
				break ;
		}
		if (g_should_quit)
			return ;
		// ------------------------------------------------------------------------

		// Create & send JSON to server
		json	j;
		j["type"]			= "join-tournament";
		j["name"]			= g_open_tournaments_list[index].name;
		j["nick"]			= nick;
		send_api_message(j);

		clear_screen();

		std::cout << "Waiting for response from server..." << std::endl;
		while (!g_should_quit && g_join_tournament_state == 0)
			usleep(1000);
		if (g_should_quit)
			return ;
		if (g_join_tournament_state == -1) // Failure
		{
			usleep(2000 * 1000); // Give error message time to show on screen
			g_join_tournament_state = 0;
			return ;
		}
		g_join_tournament_state = 0;

		// TOURNAMENT JOIN ROOT MENU =============================================
		std::vector<t_menu_item> main_menu;

		temp.name		= "Exit tournament     ";
		temp.fg_color	= FG_RED;
		temp.bg_color	= BG_BLACK;
		main_menu.push_back(temp);

		g_join_tournament_title = g_join_tournament_base_title + std::to_string(g_player_count) + " players joined --\n";

		while ( ! g_should_quit )
		{
			usleep(1000);
			index = select_from_menu(main_menu, g_join_tournament_title);
			if (index < 0 && g_init_game_state == 0)
				continue ;
			if (g_should_quit || index == 0)
			{
				send_api_message("quit");
				return ;
			}
			if (g_init_game_state == 0)
				continue ;
			g_init_game_state = 0;

			int ret = enter_tournament();
			if (ret != 0)
			{
				send_api_message("quit");
				return ;
			}
		}
		// ------------------------------------------------------------------------
	}
}
