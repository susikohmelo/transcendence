#include <main.hpp>

void create_splitscreen()
{
	// Just in case :P
	send_api_message("quit");

	disable_mouse_tracking_and_tcsetattr();

	std::string	s_score;
	int			i_score;

	clear_screen();

	// Getting required data ==================================================
	std::cout << FG_BLACK << BG_GREEN << " -> Press enter on an empty field to go back <- " << C_END << std::endl;
	std::cout << "\nEnter score count: ";
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

	// MENU ===================================================================
	int index;
	std::vector<t_menu_item> login_menu;

	// Yeah this is big dumb dumb
	t_menu_item temp;

	temp.name		= "Start split-screen  ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	login_menu.push_back(temp);

	temp.name		= "Exit                ";
	temp.fg_color	= FG_RED;
	temp.bg_color	= BG_BLACK;
	login_menu.push_back(temp);

	std::string title = "-- Split-screen menu --\n";

	bool done = false;
	while ( g_should_quit == false && done == false)
	{
		usleep(1000);
		index = select_from_menu(login_menu, title);
		switch (index) // Honestly a switch case here is stupid but whatever
		{
			case 0: // Start
				done = true; break;
			case 1: // EXIT
				return ;
		}
	}
	// ------------------------------------------------------------------------

	// Create & send JSON to server
	json	j;
	j["type"]			= "start-split-screen";
	j["points"]			= s_score; // In the api this is a string. WHY
	send_api_message(j);

	clear_screen();

	std::cout << "Waiting for response from server..." << std::endl;
	while (!g_should_quit && g_splitscreen_state == 0)
		usleep(1000);
	if (g_should_quit)
		return ;
	if (g_splitscreen_state == -1) // Failure
	{
		usleep(2500 * 1000); // Give error message time to show on screen
		g_splitscreen_state = 0;
		return ;
	}
	g_splitscreen_state = 0;
	g_tournament_state = 0;

	clear_screen();
	in_game();		// This will block until another ready state is received
	usleep(10000);	// Short sleep to let the rendering thread stop
	clear_screen();
    usleep(1000);
    enable_mouse_tracking_and_tcsetattr(); // silence arrow key presses
    if (g_tournament_state != 0)
        std::cout << "Player " << g_tournament_state << " won :D" << std::endl;
    else
        std::cout << "Tie! :d" << std::endl;
    usleep(2500 * 1000);

	disable_mouse_tracking_and_tcsetattr();
	g_splitscreen_state = 0;
	g_tournament_state = 0;
	// ------------------------------------------------------------------------
}
