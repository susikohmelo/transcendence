#include <main.hpp>

void create_tournament()
{
	// Just in case :P
	send_api_message("quit");

	disable_mouse_tracking_and_tcsetattr();
	g_player_count = 1;
	g_join_tournament_state = 0;
	g_create_tournament_state = 0;

	std::string	nick;
	std::string	tourname;

	std::string	s_playercount;
	int			i_playercount;

	std::string	s_score;
	int			i_score;

	clear_screen();

	// Getting required data ==================================================
	std::cout << FG_BLACK << BG_GREEN << " -> Press enter on an empty field to go back <- " << C_END << std::endl;

	std::cout << "\nEnter your alias: ";
	std::getline(std::cin, nick);
	if (nick.empty())
	    return;

	std::cout << "\nEnter tournament's name: ";
	std::getline(std::cin, tourname);
	if (tourname.empty())
	    return;

	std::cout << "\nEnter player count (2, 4, 8, or 16): ";
	std::getline(std::cin, s_playercount);
    if (s_playercount.empty())
        return;
    try // Convert string to int
        { i_playercount = stoi(s_playercount); }
    catch(...)
        { error_and_wait("Invalid number", 2000 * 1000); return;}
    if (i_playercount <= 1)
        { error_and_wait("Too few players", 2000 * 1000); return;}

	std::cout << "\nEnter score count (1-7): ";
	std::getline(std::cin, s_score);
    if (s_score.empty())
        return;
    try // Convert string to int
        { i_score = stoi(s_score); }
    catch(...)
        { error_and_wait("Invalid number", 2000 * 1000); return;}
    if (i_score <= 0)
        { error_and_wait("Too few points (1 minimum!!!!)", 2000 * 1000); return;}
	// ------------------------------------------------------------------------

	// Create & send JSON to server
	json	j;
	j["type"]			= "create-tournament";
	j["name"]			= tourname;
	j["player_count"]	= i_playercount;
	j["nick"]			= nick;
	j["points"]			= s_score; // In the api this is a string. WHY // idk JS being JS probably lmao
	send_api_message(j);

	tengi_handle_events();
    tengi_ungrab_keyboard();

	clear_screen();

	while (!g_should_quit && g_create_tournament_state == 0)
		usleep(10000);
	if (g_should_quit)
		return ;
	if (g_create_tournament_state == -1) // Failure
	{
		usleep(2000 * 1000); // Give error message time to show on screen
		g_create_tournament_state = 0;
		return ;
	}
	g_create_tournament_state = 0;

	// TOURNAMENT OWNER ROOT MENU =============================================
	int index;
	std::vector<t_menu_item> main_menu;

	// Yeah this is big dumb dumb
	t_menu_item temp;

	temp.name		= "Start tournament    ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	temp.name		= "Exit tournament     ";
	temp.fg_color	= FG_RED;
	temp.bg_color	= BG_BLACK;
	main_menu.push_back(temp);

	g_create_tournament_title = g_create_tournament_base_title + std::to_string(g_player_count) + " players joined --\n";

	while ( g_should_quit == false )
	{
		usleep(1000);
		index = select_from_menu(main_menu, g_create_tournament_title);
		disable_mouse_tracking_and_tcsetattr();
		switch (index) // Honestly a switch case here is stupid but whatever
		{
			case 0: // CREATE
			{
				send_api_message("start-tournament");

				// TOURNAMENT WAITING MENU ========================================
				std::vector<t_menu_item> wait_menu;

				temp.name		= "Exit tournament     ";
				temp.fg_color	= FG_RED;
				temp.bg_color	= BG_BLACK;
				wait_menu.push_back(temp);

				g_join_tournament_title = g_join_tournament_base_title + std::to_string(g_player_count) + " players joined --\n";

				while ( ! g_should_quit )
				{
					usleep(1000);
					std::cout << BG_GREEN << FG_BLACK << g_join_tournament_base_title << C_END << std::endl;
					index = select_from_menu(wait_menu, g_join_tournament_title);
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
				// ----------------------------------------------------------------
				break ;
			}

			case 1: // EXIT
				send_api_message("quit");
				return ;
		}
	}
	// ------------------------------------------------------------------------
	disable_mouse_tracking_and_tcsetattr();
}
