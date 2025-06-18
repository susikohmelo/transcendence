#pragma once
#include <functional>
#include <string>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <nlohmann/json.hpp>
#include <our_websocket.hpp>
#include <vector>
#include <gamestate.h>
#include <signal.h>
#include <tengi.h>

using json = nlohmann::json;
using namespace nlohmann::literals;

constexpr char C_GREEN[]= "\033[0;32m";
constexpr char C_RED[]	= "\033[0;31m";
constexpr char C_BLUE[]	= "\033[0;34m";
constexpr char C_CYAN[]	= "\033[0;36m";
constexpr char C_END[]	= "\033[0m";

constexpr char FG_BLACK[]	= "\e[0;30m";
constexpr char FG_RED[]		= "\e[0;31m";
constexpr char FG_GREEN[]	= "\e[0;32m";
constexpr char FG_YELLOW[]	= "\e[0;33m";
constexpr char FG_BLUE[]	= "\e[0;34m";
constexpr char FG_PURPLE[]	= "\e[0;35m";
constexpr char FG_CYAN[]	= "\e[0;36m";
constexpr char FG_WHITE[]	= "\e[0;37m";

constexpr char BG_BLACK[]	= "\e[40m";
constexpr char BG_RED[]		= "\e[41m";
constexpr char BG_GREEN[]	= "\e[42m";
constexpr char BG_YELLOW[]	= "\e[43m";
constexpr char BG_BLUE[]	= "\e[44m";
constexpr char BG_PURPLE[]	= "\e[45m";
constexpr char BG_CYAN[]	= "\e[46m";
constexpr char BG_WHITE[]	= "\e[47m";

// Used for displaying menu items
typedef struct s_menu_item
{
	std::string	name;
	std::string	fg_color;
	std::string	bg_color;
} t_menu_item;

enum payload_type
{
    CONNECTED,
	PING,
	GAME_STATE,
	TOURNAMENT_STATE,
	CREATE_TOURNAMENT,
	JOIN_TOURNAMENT,
	INIT_GAME,
	READY,
	LOGIN,
	REGISTER,
	SPLIT_SCREEN_WINNER,
	SPLIT_SCREEN_START,
	QUERY,
	UNKNOWN
};

extern t_gamestate	 g_gamestate              ;
extern int			 g_create_tournament_state;
extern int			 g_join_tournament_state  ;
extern int			 g_tournament_state		  ;
extern int			 g_splitscreen_state	  ;
extern int			 g_init_game_state		  ;
extern int			 g_ready_state			  ;
extern int			 g_player_count			  ;
extern int			 g_register_state		  ;
extern int			 g_login_state			  ;
extern int           g_select_menu_interrupt  ;
extern int           g_id                     ; // player id
extern std::string   g_username               ;
extern int           g_user_id                ; // used for API token validation, so differs from player id
extern std::string   g_api_token              ;
extern Events		 g_events                 ;
extern sig_atomic_t	 g_should_quit            ;
extern ConnectionHdl g_connection             ;
extern Client        g_client                 ;
extern std::vector<t_menu_item>	g_open_tournaments_list;
extern const std::string g_create_tournament_base_title;
extern const std::string g_join_tournament_base_title;
extern std::string g_create_tournament_title;
extern std::string g_join_tournament_title;

extern "C" { void disable_mouse_tracking_and_tcsetattr(void); }
extern "C" { void enable_mouse_tracking_and_tcsetattr(void); }

// Though it looks ugly, the compiler turns this function into a kind of hashmap
payload_type payload_type_hash(const std::string &type);
int select_from_menu(std::vector<t_menu_item> &menu_vector, std::string &title);
void login_menu(void);
void in_game();
int enter_tournament();
void create_tournament();
void join_tournament();
void create_splitscreen();
void clear_screen();
void error_and_wait(const std::string &toprint, const unsigned int towait);
void send_mouse_paddlestate();
