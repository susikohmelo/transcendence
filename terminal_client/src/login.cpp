#include <main.hpp>
#include <iostream>
#include <string>

static inline int register_nick()
{

	disable_mouse_tracking_and_tcsetattr();
	clear_screen();

	std::string	nick;
	std::string	pass;

	// Getting required data ==================================================
	std::cout << FG_BLACK << BG_GREEN << " -> Press enter on an empty field to go back <- " << C_END << std::endl;
	std::cout << "\nEnter nickname: ";
	std::getline(std::cin, nick);
	if (nick.empty())
		return -1;

	std::cout << "\nEnter password: ";
    enable_mouse_tracking_and_tcsetattr(); // hide input
	std::getline(std::cin, pass);
    disable_mouse_tracking_and_tcsetattr();
	if (pass.empty())
		return -1;
	// ------------------------------------------------------------------------

	// Create & send JSON to server
	json	j;
	j["type"]			= "register";
	j["username"]		= nick;
	j["password"]		= pass;
	j["passconf"]		= pass;
	std::string js = j.dump();
	send_message(&g_client, &g_connection, js); // Send stringified JSON

	clear_screen();

	std::cout << "Waiting for response from server..." << std::endl;
	while (!g_should_quit && g_register_state == 0)
		usleep(10000);
	if (g_should_quit)
		return -1;
	if (g_register_state == -1) // Failure
	{
		g_register_state = 0;
		usleep(2500 * 1000); // Give error message time to show on screen
		return -1;
	}
	g_register_state = 0;
	return 0;
}

static inline int login_nick()
{
	clear_screen();

	std::string	username;
	std::string	pass;

	// Getting required data ==================================================
	std::cout << FG_BLACK << BG_GREEN << " -> Press enter on an empty field to go back <- " << C_END << std::endl;
	std::cout << "\nEnter username: ";
	std::getline(std::cin, username);
	if (username.empty())
		return -1;

	std::cout << "\nEnter password: ";
    enable_mouse_tracking_and_tcsetattr(); // hide input
	std::getline(std::cin, pass);
    disable_mouse_tracking_and_tcsetattr();
	// ------------------------------------------------------------------------

	// Create & send JSON to server
	json	j;
	j["type"]			= "sign-in";
	j["username"]		= username;
	j["password"]		= pass;
	std::string js = j.dump();
	send_message(&g_client, &g_connection, js); // Send stringified JSON

	clear_screen();

	std::cout << "Waiting for response from server..." << std::endl;
	while (!g_should_quit && g_login_state == 0)
		usleep(10000);
	if (g_should_quit)
		return -1;
	if (g_login_state == -1) // Failure
	{
		g_login_state = 0;
		usleep(2500 * 1000); // Give error message time to show on screen
		return -1;
	}
	g_login_state = 0;
    g_username = username;
	return 0;
}

void login_menu(void)
{
	disable_mouse_tracking_and_tcsetattr();
	clear_screen();

	// TOURNAMENT OWNER ROOT MENU =============================================
	int index;
	std::vector<t_menu_item> login_menu;

	// Yeah this is big dumb dumb
	t_menu_item temp;

	temp.name		= "Log into account    ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	login_menu.push_back(temp);

	temp.name		= "Register account    ";
	temp.fg_color	= FG_CYAN;
	temp.bg_color	= BG_BLACK;
	login_menu.push_back(temp);

	temp.name		= "Exit                ";
	temp.fg_color	= FG_RED;
	temp.bg_color	= BG_BLACK;
	login_menu.push_back(temp);

	std::string title = "-- Login menu | You are not logged in --\n";

	while ( g_should_quit == false )
	{
		usleep(1000);
		index = select_from_menu(login_menu, title);
		disable_mouse_tracking_and_tcsetattr();
		switch (index) // Honestly a switch case here is stupid but whatever
		{
			case 0: // Login
			{
				int ret = login_nick();
				if (ret != 0)
					break ;
				return ;
			}
			case 1: // Register
			{
				// Regardless of what happens we go back to menu anyway
				register_nick();
				break ;
			}
			case 2: // EXIT
				g_should_quit = true;
				return ;
		}
	}
	// ------------------------------------------------------------------------
}
