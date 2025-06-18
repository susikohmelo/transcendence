#include <vector>
#include <unistd.h>
#include <string>
#include <main.hpp>

// Index is printed as selected
static inline void print_menu(std::vector<t_menu_item> &menu_vector, int index)
{
    size_t max_length = 0;
    for (size_t i = 0; i < menu_vector.size(); ++i)
        max_length = menu_vector[i].name.length() > max_length ?
            menu_vector[i].name.length()
          : max_length;

    // I only know how to align strings in C, so no std::cout
    for (int i = 0; i <(int)menu_vector.size(); ++i)
    {
        if (i != index)
            printf("%s%s", menu_vector[i].fg_color.c_str(), menu_vector[i].bg_color.c_str());
        else
            printf("%s%s", FG_BLACK, BG_RED);
        printf("%-*s%s\n", (int)max_length, menu_vector[i].name.c_str(), C_END);
    }
}

// Blocking
// Returns index selected
int select_from_menu(std::vector<t_menu_item> &menu_vector, std::string &title)
{
	usleep(90000);

	g_events.m1_down = false;
	enable_mouse_tracking_and_tcsetattr();
	int index = -1;
	while ( ! g_should_quit && ! g_select_menu_interrupt )
	{
		usleep(1000);

		// Print title & selection
		clear_screen();
		std::cout << FG_BLACK << BG_GREEN << title << C_END << std::endl;
		print_menu(menu_vector, index);

		while ( ! g_should_quit && ! g_select_menu_interrupt )
		{
			usleep(1000);
			tengi_handle_events();

            if (g_events.should_redraw)
            {
                std::cout << FG_BLACK << BG_GREEN << title << C_END << std::endl;
                print_menu(menu_vector, index);
                g_events.should_redraw = false;
            }

			if (g_events.m1_down && index != -1)
			{
				g_events.m1_down = false;
				goto done_selecting;
			}
			else
				g_events.m1_down = false;

			constexpr int button_width_in_chars = 20;

            Vec2 fpos = tengi_estimate_pixel_to_cell(tengi_get_mouse_pos());
            IVec2 pos = { (int)fpos.x, (int)fpos.y - 2/*title rows offset*/};

			if (pos.y < 0 || pos.y >= (int)menu_vector.size() || pos.x < 0 || pos.x > button_width_in_chars)
			{
				if (index == -1)
					continue;
				index = -1;
				break;
			}

			if (index != pos.y)
			{
				index = pos.y;
				break ;
			}
		}
	}
	done_selecting:
		clear_screen();
		if ( ! g_select_menu_interrupt )
			return index;
		g_select_menu_interrupt = 0;
		return -1;
}
