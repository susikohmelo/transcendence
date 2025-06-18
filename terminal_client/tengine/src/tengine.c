#include <tengi.h>

Scalar color_value(Vec4 color)
{
	return (color.r + color.g + color.b)/3.f;
}

// We can divide a cell in 4 by using Unicode block elements. However, we only
// have 2 colors at our disposal: background and foreground. We divide the
// colors to back and front based on their values and average them.
size_t draw_cell(size_t cells_length, char cells[], const Vec4 colors[static 4])
{
    if (0) // ASCII, TODO this needs to be parameterized!
    {
        Vec4 color = vec4(0);
        for (size_t i = 0; i < 4; ++i)
            color = vec_add(color, colors[i]);
        color = vec_clamp(vec_div(color, 4.f), 0.f, 1.f);
        return sprintf(cells + cells_length,
            "\e[48;2;%d;%d;%dm ",
            (int)(255*color.r ), (int)(255*color.g ), (int)(255*color.b ));
    }
                                       //0123456789ABCDEF
	static const wchar_t graphemes[] = L" ▘▝▀▖▌▞▛▗▚▐▜▄▙▟█";

	size_t index       = 0;
	Vec4 back_color  = (Vec4){0};
	Vec4 front_color = (Vec4){0};
	size_t back_elems  = 0;
	size_t front_elems = 0;

	Scalar max = 0.f;
	Scalar min = 1.f;
	for (size_t i = 0; i < 4; ++i) {
		max = fmax(max, color_value(colors[i]));
		min = fmin(min, color_value(colors[i]));
	}

	for (size_t i = 0; i < 4; ++i) {

		bool bit = max - color_value(colors[i]) < color_value(colors[i]) - min;
		if (bit) {
		    front_color = vec_add(front_color, colors[i]);
		    front_elems++;
		} else {
		    back_color  = vec_add(back_color, colors[i]);
		    back_elems++;
		}
		index |= bit << i;
	}
	front_color = vec_clamp(vec4_divs(front_color, front_elems), 0.f, 1.f);
	back_color  = vec_clamp(vec4_divs(back_color,  back_elems ), 0.f, 1.f);

	return sprintf(cells + cells_length,
		"\e[38;2;%d;%d;%dm"
		"\e[48;2;%d;%d;%dm%lc",
		(int)(255*front_color.r), (int)(255*front_color.g), (int)(255*front_color.b),
		(int)(255*back_color.r ), (int)(255*back_color.g ), (int)(255*back_color.b ),
		graphemes[index]);
}

static size_t fill_char_buffer(
    char chars[], IVec2 resolution, const Vec4 colors[], const char* text)
{
    if (text == NULL)
        text = "";
    size_t chars_length = 0;

	for (int y = 0; y < resolution.y; y += 2)
	{
        bool line_ended = false;
        if (*text != '\0') {
            memcpy(chars + chars_length, "\e[0m", strlen("\e[0m"));
            chars_length += strlen("\e[0m");
        }
		for (int x = 0; x < resolution.x; x += 2)
        {
            // TODO skipping ANSI escape sequences work with the assumption
            // of having max 2 24 bit color codes (fg and bg) per 2 byte
            // character. Breaking that assumption may overflow. We know that
            // this assumption holds for ft_transcendence (no user input, just
            // hard coded menus), but THIS MUST BE FIXED FOR THE FINAL LIBRARY!
            while ( ! line_ended && *text == '\e') {
                while (*text != 'm')
                    chars[chars_length++] = *text++;
                chars[chars_length++] = *text++;
            }
            if (*text == '\n') {
                line_ended = true;
                ++text;
            }
            if (*text != '\0' && !line_ended) {
                chars[chars_length++] = *text++;
            }
            else if (0 && colors != NULL && x < resolution.x-2 && y < resolution.y-2) { // AA // TODO PARAMETERIZE AA

                Vec4 char_colors9[3][3];
                for (size_t y3 = 0; y3 < 3; ++y3)
                    for (size_t x3 = 0; x3 < 3; ++x3)
                        char_colors9[y3][x3] = vec_clamp(colors[(y + y3)*resolution.x + (x + x3)%resolution.x], 0.f, 1.f);

                Vec4 char_colors[4];
                char_colors[0] = vec_div(vec_add(vec_add(vec_add(char_colors9[0][0], char_colors9[0][1]), char_colors9[1][0]), char_colors9[1][1]), 4.f);
                char_colors[1] = vec_div(vec_add(vec_add(vec_add(char_colors9[0][1], char_colors9[0][2]), char_colors9[1][1]), char_colors9[1][2]), 4.f);
                char_colors[2] = vec_div(vec_add(vec_add(vec_add(char_colors9[1][0], char_colors9[1][1]), char_colors9[2][0]), char_colors9[2][1]), 4.f);
                char_colors[3] = vec_div(vec_add(vec_add(vec_add(char_colors9[1][1], char_colors9[1][2]), char_colors9[2][1]), char_colors9[2][2]), 4.f);

                chars_length += draw_cell(chars_length, chars, char_colors);
            }
            else if (colors != NULL) { // no AA
                Vec4 char_colors[4] = {
                    vec_clamp(colors[(y + 0)*resolution.x + (x + 0)%resolution.x], 0.f, 1.f),
                    vec_clamp(colors[(y + 0)*resolution.x + (x + 1)%resolution.x], 0.f, 1.f),
                    vec_clamp(colors[(y + 1)*resolution.x + (x + 0)%resolution.x], 0.f, 1.f),
                    vec_clamp(colors[(y + 1)*resolution.x + (x + 1)%resolution.x], 0.f, 1.f),
                };
                chars_length += draw_cell(chars_length, chars, char_colors);
            }
		}
		chars[chars_length++] = '\n';
	}
	chars_length -= strlen("\n");
	strcpy(chars + chars_length, "\e[0m\e[0;0H");
	return chars_length + strlen("\e[0m\e[0;0H");
}

// TEMP for ft_transcendence
#include "../../include/gamestate.h"

static char* s_draw_chars = NULL;

static void free_draw_chars(void)
{
    free(s_draw_chars);
}

// TODO text is a temporary hack for rudiemntary menus and debug info. Currently
// only text and ANSI colors are handled properly, any other escape sequences
// break! We should do somehting more sophisticated than a null terminated
// character array.
void tengi_draw(IVec2 resolution, const Vec4 colors[], const char text[])
{
    static IVec2 last_resolution;

    if (resolution.x != last_resolution.x || resolution.y != last_resolution.y)
    {
        if (s_draw_chars == NULL)
            atexit(free_draw_chars);

        IVec2 terminal_size = { resolution.x >> 1, resolution.y >> 1 };
        size_t chars_buffer_size = terminal_size.x*terminal_size.y*snprintf(NULL, 0,
            "\e[38;2;%d;%d;%dm"
            "\e[48;2;%d;%d;%dm%lc",
            255, 255, 255, 255, 255, 255, L'█')
            + terminal_size.y*strlen("\e[0m\n")
            + sizeof"\e[0m\e[0;0H";

        assert((s_draw_chars = malloc(chars_buffer_size)));
        last_resolution = resolution;
    }

    size_t chars_length = fill_char_buffer(s_draw_chars, resolution, colors, text);

    // Temporary ft_transcendence related hack to prevent drawing to outdated
    // terminal size. Not thread safe, but has no impact on the correctness of
    // the program.
    extern Events g_events;
    bool should_redraw = g_events.should_redraw;
    g_events.should_redraw = false;

    if ( ! should_redraw) // don't draw to outdated terminal
        (int){0} = write(STDOUT_FILENO, s_draw_chars, chars_length);
}

