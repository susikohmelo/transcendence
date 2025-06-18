#ifndef GAMESTATE_INCLUDED
#define GAMESTATE_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define AI_PLAYER		-1
#define WAITING			0
#define PLAYER1			1
#define PLAYER2			2
#define SPLIT_SCREEN	3

#define MENU_SELECT_ACCELERATOR_LENGTH 40

typedef enum menu_state
{
    MENU_GAME_ROOT,
    MENU_SELECT_ACCELERATOR,
} MenuState;

typedef struct s_ball
{
	double	x;
	double	y;
	double	z;
} t_ball;

typedef struct s_player
{
	double	x;
	double	y;
} t_player;

typedef struct s_gamestate
{
	// Mutex etc.
	pthread_mutex_t	lock; // Always use this mutex when both threads running
	bool			game_running;
	bool			exit_thread;
    uint64_t        resize_timer;

    MenuState menu_state;
    size_t    selected_accelerator;
    int       hovered;

	// The actual gamestate stuff
	t_ball		ball;
	t_player	player1;
	t_player	player2;
	int			player;
} t_gamestate;

typedef struct s_events
{
	bool	m1_down;
    bool    exiting_game;
    bool    should_redraw; // window resize etc.
    double* mouse_move; // NULL if mouse not moved, otherwise [0] == x, [1] == y.
    const char* keys_pressed[8];  // These contain the string to be sent to the
    const char* keys_released[8]; // server e.g. "ArrowUp", or NULL if not pressed.
} Events;

typedef struct s_move_paddle
{
	double	x;
	double	y;
} t_move_paddle;

void clipong_set_input_event_hooks(Events* data);
extern int           g_select_menu_interrupt  ;

#if __cplusplus
} // extern "C"
#endif

#endif // GAMESTATE_INCLUDED
