#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <tengi.h>
#include <gamestate.h>
#include <signal.h>

extern t_gamestate *g_gamestate_ptr;

static void handle_mouse_move(IVec2 coords, void*_out)
{
    static double mouse_buf[2];
    Events* out = _out;

    Vec2 window_pos = tengi_estimate_terminal_pos();
    Vec2 resolution = tengi_estimate_terminal_size();
    Vec2 imouse     = vec2(coords.x - window_pos.x, coords.y - window_pos.y);
    Vec2 mouse      = vec_sub(vec_mul(vec_div(imouse, resolution), 2.f), 1.f);

    mouse.y *= -1.f;
    if (resolution.x >= resolution.y)
        mouse.x *= resolution.x/resolution.y;
    else
        mouse.y /= resolution.x/resolution.y;

    mouse    = vec_mul(mouse, .4f*vec_length(vec3(mouse, 3.)));
    mouse_buf[0] = mouse.x;
    mouse_buf[1] = mouse.y;
    out->mouse_move = mouse_buf;

    if ( ! g_gamestate_ptr->game_running)
        return;
    Vec2 mouse_pos = tengi_estimate_pixel_to_cell(coords);

    if (g_gamestate_ptr->menu_state == MENU_GAME_ROOT)
    {
        if (mouse_pos.x >= strlen("Select hardware accelerator") ||
            mouse_pos.y >= 2.f)
            g_gamestate_ptr->hovered = -1;
        else
            g_gamestate_ptr->hovered = mouse_pos.y;
    }
    else if (g_gamestate_ptr->menu_state == MENU_SELECT_ACCELERATOR)
    {
        if (mouse_pos.x >= MENU_SELECT_ACCELERATOR_LENGTH)
            g_gamestate_ptr->hovered = -1;
        else
            g_gamestate_ptr->hovered = mouse_pos.y;
    }
}

static void handle_key_press(XKeyPressedEvent* xev, void*_out)
{
    Events* out   = _out;
    KeySym keysym = XLookupKeysym((void*)xev, 0);

	if (keysym == XK_Escape)
		g_select_menu_interrupt = 1;
    if ((xev->state & ControlMask) && (keysym == XK_c || keysym == XK_C))
        raise(SIGINT);

    switch (keysym) {
        case XK_w: case XK_W: out->keys_pressed[0] = "w";          break;
        case XK_a: case XK_A: out->keys_pressed[1] = "a";          break;
        case XK_s: case XK_S: out->keys_pressed[2] = "s";          break;
        case XK_d: case XK_D: out->keys_pressed[3] = "d";          break;
        case XK_Up:           out->keys_pressed[4] = "ArrowUp";    break;
        case XK_Down:         out->keys_pressed[5] = "ArrowDown";  break;
        case XK_Left:         out->keys_pressed[6] = "ArrowLeft";  break;
        case XK_Right:        out->keys_pressed[7] = "ArrowRight"; break;
    }
}

static void handle_key_release(XKeyReleasedEvent* xev, void*_out)
{
    Events* out   = _out;
    KeySym keysym = XLookupKeysym((void*)xev, 0);

    switch (keysym) {
        case XK_w: case XK_W: out->keys_released[0] = "w";          break;
        case XK_a: case XK_A: out->keys_released[1] = "a";          break;
        case XK_s: case XK_S: out->keys_released[2] = "s";          break;
        case XK_d: case XK_D: out->keys_released[3] = "d";          break;
        case XK_Up:           out->keys_released[4] = "ArrowUp";    break;
        case XK_Down:         out->keys_released[5] = "ArrowDown";  break;
        case XK_Left:         out->keys_released[6] = "ArrowLeft";  break;
        case XK_Right:        out->keys_released[7] = "ArrowRight"; break;
    }
}

static void handle_mouse_press(IVec2 cell, void* _events)
{
    (void)cell; // better to use estimate, buttons are highlighted based on estimate

    Events* events = _events;
	events->m1_down = true;

    Vec2 coords = tengi_estimate_pixel_to_cell(tengi_get_mouse_pos());
    if (g_gamestate_ptr->game_running) switch (g_gamestate_ptr->menu_state)
    {
    case MENU_GAME_ROOT:
        if (coords.y < 1.f && coords.x < strlen("Select hardware accelerator"))
            g_gamestate_ptr->menu_state = MENU_SELECT_ACCELERATOR;
        else if (coords.y < 2.f && coords.x < strlen("Select hardware accelerator")) {
            g_gamestate_ptr->game_running = false;
            events->exiting_game = true;
        }
        break;

    case MENU_SELECT_ACCELERATOR:
        if (coords.x < MENU_SELECT_ACCELERATOR_LENGTH)
            g_gamestate_ptr->selected_accelerator = (size_t)((int)coords.y - 1);
        g_gamestate_ptr->menu_state = MENU_GAME_ROOT;
        break;
    }
}

static void handle_enter_window(XEnterWindowEvent* xev, void*_)
{
    (void)xev;
    (void)_;
    if (g_gamestate_ptr->game_running)
        tengi_grab_keyboard();
}

static void handle_leave_window(XLeaveWindowEvent* xev, void*_)
{
    (void)xev;
    (void)_;
    tengi_ungrab_keyboard();
}

static void handle_resize_window(XConfigureEvent* xev, void*_events)
{
    static int last_width;
    static int last_height;
    static IVec2 last_size;

    // Ignore everything except resize
    if (xev->width == last_width && xev->height == last_height)
        return;
    last_width  = xev->width;
    last_height = xev->height;

    IVec2 term_size = tengi_get_terminal_size();
    if (term_size.x == last_size.x && term_size.y == last_size.y)
        return;
    last_size = term_size;

    // --------------------------------
    // Clear screen // TODO this should be a tengi function

    // Instead of erasing screen using escape sequences, we fill the screen with
    // spaces and move cursor back to origin. This prevents scrolling and saves
    // memory.

    Events* events = _events;
    events->should_redraw = true;

    static char* space_buf;
    static size_t space_buf_size;
    const char go_home[] = "\e[H";

    if (term_size.x*term_size.y != (int)space_buf_size)
    {
        free(space_buf);
        space_buf_size = term_size.x*term_size.y;
        space_buf = (char*)malloc(space_buf_size + 2*strlen(go_home));
        memcpy(space_buf, go_home, strlen(go_home));
        memset(space_buf + strlen(go_home), ' ', space_buf_size);
        memcpy(space_buf + strlen(go_home) + space_buf_size, go_home, strlen(go_home));
    }

    if (g_gamestate_ptr->game_running) {
        g_gamestate_ptr->resize_timer = 0;
        return; // let tengine redraw
    }

    // Writing a big thing, so must not use slow streams
    int _ = write(STDOUT_FILENO, space_buf, space_buf_size + 2*strlen(go_home));
    (void)_;

    // TODO update coord model. Character size remains, so only reallocation of
    // internal buffers needed.
    // Note: it is not necessary to shrink the buffers. Even if the cells do not
    // exist anymore, they contain useful data.
}

void clipong_set_input_event_hooks(Events* data)
{
    tengi_set_key_press_handler(handle_key_press, data);
    tengi_set_key_release_handler(handle_key_release, data);
    tengi_set_mouse_press_handler(handle_mouse_press, data);
    tengi_set_mouse_move_handler(handle_mouse_move, data);
    tengi_set_enter_window_handler(handle_enter_window, data);
    tengi_set_leave_window_handler(handle_leave_window, data);
    tengi_set_resize_window_handler(handle_resize_window, data);
}

