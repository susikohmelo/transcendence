#ifndef TENGI_INCLUDED
#define TENGI_INCLUDED 1

#if __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <err.h>
#include <math.h>
#include <locale.h>
#include <sys/time.h>
#include <termios.h>
#include <vec.h>

// TODO in the future
// - Replace all X11 with ANSI for portability when possible
// - Windows and WSL support

#include <stddef.h>
#include <vec.h>

typedef struct s_X11Screens // TODO make private and rename
{
    Display* display;
    Window   window;
    Window   root;
    pid_t    wsl_query;
    pid_t    wsl_events;
    int      wsl_query_pipe_in[2];
    int      wsl_query_pipe_out[2];
    int      wsl_events_pipe_in[2];
    int      wsl_events_pipe_out[2];
    struct termios termios_original;
    struct termios termios_current;
} X11Screens;

typedef struct s_IVec2
{
	int x;
	int y;
} IVec2;

// Initialize engine. Auto destroyed at exit.
void tengi_init(void);

IVec2 tengi_get_mouse_pos(void);           // in pixels
IVec2 tengi_get_window_pos(void);          // in pixels
IVec2 tengi_get_window_size(void);         // in pixels
IVec2 tengi_get_display_size(void);        // in pixels
IVec2 tengi_get_terminal_size(void);       // in characters
IVec2 tengi_get_global_mouse_pos(void);    // in pixels
IVec2 tengi_get_terminal_resolution(void); // in characters divided to 4 cells

// Estimate terminal attributes in pixels. Estimations are based on click data,
// so mouse press handler has to be set. If any of these change, it might take a
// couple of clicks to recalibrate.
Vec2 tengi_estimate_cell_size(void);
Vec2 tengi_estimate_terminal_pos(void);
Vec2 tengi_estimate_terminal_size(void);
Vec2 tengi_estimate_cell_to_pixel(IVec2 cell);
Vec2 tengi_estimate_pixel_to_cell(IVec2 pixel);

void tengi_set_key_press_handler(void (*handler)(XKeyPressedEvent*, void* param), void* param);
void tengi_set_key_release_handler(void (*handler)(XKeyReleasedEvent*, void* param), void* param);
void tengi_set_mouse_press_handler(void (*handler)(IVec2 coords, void *param), void* param);
void tengi_set_mouse_release_handler(void (*handler)(IVec2 coords, void *param), void* param);
void tengi_set_mouse_move_handler(void (*handler)(IVec2 coords, void* param), void* param);
void tengi_set_enter_window_handler(void (*handler)(XEnterWindowEvent*, void* param), void* param);
void tengi_set_leave_window_handler(void (*handler)(XLeaveWindowEvent*, void* param), void* param);
void tengi_set_resize_window_handler(void (*handler)(XConfigureEvent*, void* param), void* param); // TODO currently misnomer, handles XConfigureEvents

// Taking full control of inputs. WARNING: the inputs are blocked from EVERYONE
// else untill ungrabbing!
void tengi_grab_keyboard(void);
void tengi_ungrab_keyboard(void);
void tengi_grab_mouse(void);
void tengi_ungrab_mouse(void);

// Calls all handlers set by setters above if events pending.
void tengi_handle_events(void);

double tengi_time(void); // in seconds since the first call

// colors is user allocated buffer with size resolution.x*resolution.y
void tengi_draw(IVec2 resolution, const Vec4 colors[], const char text[]);

#if __cplusplus
} // extern "C"
#endif

#endif // TENGI_INCLUDED
