#include <tengi.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>

X11Screens g_X11;
int g_epoll_fd;
bool g_is_gnome;

// While it is possibe to get window size/position in pixels and terminal size
// in cells, there is no way to get terminal data in pixels, so estimate it
// ourselves using simple regression. We can get the pixel and cell from mouse
// clicks, so we'll use that to gather the data.
//
// cell(pixel) = beta*pixel + alpha + .5
// pixel(cell) = (cell - alpha - .5)/beta
typedef struct coord_model
{
    Vec2   alpha;
    Vec2   beta;
    Vec2   length; // number of data points
    size_t size;
    Vec2*  data; // cached averages of max_errors and min_errors
    Vec2*  max_errors;
    Vec2*  min_errors;
} CoordModel;

CoordModel g_coord_model;

static void enable_mouse_tracking(void)
{
    printf("\e[?1000h"); // mouse tracking
    printf("\e[?1015h"); // extended mouse coordinates
    fflush(stdout);
}

static void disable_mouse_tracking(void)
{
    printf("\e[?1000l"); // mouse tracking
    printf("\e[?1015l"); // extended mouse coordinates
    fflush(stdout);
}

void enable_mouse_tracking_and_tcsetattr(void)
{
    printf("\e[?1000h"); // mouse tracking
    printf("\e[?1015h"); // extended mouse coordinates
    printf("\e[?25l");   // hide cursor
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &g_X11.termios_current);
}

void disable_mouse_tracking_and_tcsetattr(void)
{
    printf("\e[?1000l"); // mouse tracking
    printf("\e[?1015l"); // extended mouse coordinates
    printf("\e[?25h");   // unhide cursor
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &g_X11.termios_original);
}

static int xlib_error_handler(Display *display, XErrorEvent *e)
{
    char buffer[256] = "";

    XGetErrorText(display, e->error_code, buffer, sizeof buffer);
    // TODO temp, we want to pass error messages to user instead
	fprintf(stderr, "X11 event %i error: %s\n", e->type, buffer);
	return True;
}

static void destroy_X11Screens(void)
{
    disable_mouse_tracking();
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_X11.termios_original);
    XCloseDisplay(g_X11.display);
    free(g_coord_model.data);
    printf("\e[?25h");   // unhide cursor
    fflush(stdout);
}

static bool init_wsl_query(void)
{
    if (g_X11.wsl_query) // already initialized
        return true;

    if (pipe(g_X11.wsl_query_pipe_in)  == -1 ||
        pipe(g_X11.wsl_query_pipe_out) == -1 ||
        (g_X11.wsl_query = fork())     == -1 )
        return false;

    if (g_X11.wsl_query == 0) // child
        if (dup2(g_X11.wsl_query_pipe_in[0], STDIN_FILENO)        == -1 ||
            dup2(g_X11.wsl_query_pipe_out[1], STDOUT_FILENO)      == -1 ||
            close(g_X11.wsl_query_pipe_in[1])                     == -1 ||
            close(g_X11.wsl_query_pipe_out[0])                    == -1 ||
            execl("./tengine/wslquery.exe", "wslquery.exe", NULL) == -1 )  // TODO fix hard coded path!
            exit(!!fprintf(stderr, "wslquery: %s\n", strerror(errno)));

    close(g_X11.wsl_query_pipe_in[0]) ;
    close(g_X11.wsl_query_pipe_out[1]);
    return true;
}

static bool init_wsl_events(void)
{
    if (g_X11.wsl_events) // already initialized
        return true;

    if (pipe(g_X11.wsl_events_pipe_in)  == -1 ||
        pipe(g_X11.wsl_events_pipe_out) == -1 ||
        (g_X11.wsl_events = fork())     == -1 )
        return false;

    if (g_X11.wsl_events == 0) // child
        if (dup2(g_X11.wsl_events_pipe_in[0], STDIN_FILENO)         == -1 ||
            dup2(g_X11.wsl_events_pipe_out[1], STDOUT_FILENO)       == -1 ||
            close(g_X11.wsl_events_pipe_in[1])                      == -1 ||
            close(g_X11.wsl_events_pipe_out[0])                     == -1 ||
            execl("./tengine/wslevents.exe", "wslevents.exe", NULL) == -1 )  // TODO fix hard coded path!
            exit(!!fprintf(stderr, "wslevents: %s\n", strerror(errno)));

    close(g_X11.wsl_events_pipe_in[0]) ;
    close(g_X11.wsl_events_pipe_out[1]);

    struct epoll_event epoll = {};
    epoll.events  = EPOLLIN;
    epoll.data.fd = g_X11.wsl_events_pipe_out[0];
    if (epoll_ctl(
            g_epoll_fd,
            EPOLL_CTL_ADD,
            g_X11.wsl_events_pipe_out[0],
            &epoll) == -1)
    {
        kill(g_X11.wsl_events, SIGINT);
        g_X11.wsl_events = 0;
        return false;
    }
    return true;
}

static int dummy_handler(Display*_, XErrorEvent*__)
{
    (void)_;
    (void)__;
    return True;
}

static sig_atomic_t g_sigchld;
void sigchld_hanlder(int signum)
{
    g_sigchld = signum;
}

char* make_raw(const char* in) // TODO useful for debugging, make public!
{
    static _Thread_local char out_buf[256];
    char* out = out_buf;

    char c;
    while ((c = *in++) != '\0') switch (c) {
    case 0x00: out += sprintf(out, "NUL"); break;
    case 0x01: out += sprintf(out, "SOH"); break;
    case 0x02: out += sprintf(out, "STX"); break;
    case 0x03: out += sprintf(out, "ETX"); break;
    case 0x04: out += sprintf(out, "EOT"); break;
    case 0x05: out += sprintf(out, "ENQ"); break;
    case 0x06: out += sprintf(out, "ACK"); break;
    case 0x07: out += sprintf(out, "\\a"); break;
    case 0x08: out += sprintf(out, "\\b"); break;
    case 0x09: out += sprintf(out, "\\t"); break;
    case 0x0A: out += sprintf(out, "\\n"); break;
    case 0x0B: out += sprintf(out, "\\v"); break;
    case 0x0C: out += sprintf(out, "\\f"); break;
    case 0x0D: out += sprintf(out, "\\r"); break;
    case 0x0E: out += sprintf(out, "SO" ); break;
    case 0x0F: out += sprintf(out, "SI" ); break;
    case 0x10: out += sprintf(out, "DLE"); break;
    case 0x11: out += sprintf(out, "DC1"); break;
    case 0x12: out += sprintf(out, "DC2"); break;
    case 0x13: out += sprintf(out, "DC3"); break;
    case 0x14: out += sprintf(out, "DC4"); break;
    case 0x15: out += sprintf(out, "NAK"); break;
    case 0x16: out += sprintf(out, "SYN"); break;
    case 0x17: out += sprintf(out, "ETB"); break;
    case 0x18: out += sprintf(out, "CAN"); break;
    case 0x19: out += sprintf(out, "EM" ); break;
    case 0x1A: out += sprintf(out, "SUB"); break;
    case 0x1B: out += sprintf(out, "\\e"); break;
    case 0x1C: out += sprintf(out, "FS" ); break;
    case 0x1D: out += sprintf(out, "GS" ); break;
    case 0x1E: out += sprintf(out, "RS" ); break;
    case 0x1F: out += sprintf(out, "US" ); break;
    case 0x20: out += sprintf(out, "\\s"); break;
    case 0x7F: out += sprintf(out, "DEL"); break;
    default: *out++ = c;
    }
    *out = '\0';
    return out_buf;
}

static void run_wsl_controllers(void)
{
    // When piping to Windows executables from WSL, signal handlers break in the
    // Windows executables. Therefore, we need to disable keyboard signaling,
    // run our application in a child process, read stdin in parent process,
    // check if Ctrl+c pressed and signal ourselves, then pass the stdin
    // contents trough pipe to the actual application.

    // Disable keyboard signaling
    g_X11.termios_current.c_cc[VINTR] = 0;
    g_X11.termios_current.c_cc[VQUIT] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &g_X11.termios_current);

    init_wsl_query();
    init_wsl_events();

    int app_in[2];
    pid_t app_pid;

    assert(pipe(app_in) != -1);
    assert((app_pid = fork()) != -1);
    if (app_pid == 0) {
        assert(dup2(app_in[0], STDIN_FILENO) != -1);
        close(app_in[1]);
        return;
    }

    // --------------------------------
    // Master controller (parent process)

    enable_mouse_tracking();
    close(g_X11.wsl_query_pipe_in[1]);
    close(g_X11.wsl_query_pipe_out[0]);
    close(g_X11.wsl_events_pipe_in[1]);
    close(g_X11.wsl_events_pipe_out[0]);
    close(app_in[0]);

    struct epoll_event ev    = {};
    struct epoll_event event = {};
    int epoll_fd = epoll_create(1);
    assert(epoll_fd != -1);
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    assert(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) != -1);

    // If child crashes for any reason, it breaks pipe, but we still want to
    // do cleanup.
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld_hanlder);

    int killed_by_signal = false;

    while (true)
    {
        int got_input = epoll_wait(epoll_fd, &event, 1, 2);
        if (g_sigchld)
            break;
        if (got_input <= 0)
            continue;

        char input[64] = "";
        ssize_t input_length = 0;
        if ((input_length = read(event.data.fd, input, sizeof input - sizeof"")) <= 0)
            break;

        input[input_length] = '\0';
        if (input[0] == '\x3') { // ETF, End of TeXt, aka Ctrl+C
            kill(app_pid, SIGINT);
            killed_by_signal = 2; // even if child crashed, first write() may succeed
        }
        if (write(app_in[1], input, input_length) == -1)
        { // broken pipe
            kill(app_pid, SIGTERM); // just in case
            errno = 0;
            break;
        }
        killed_by_signal -= killed_by_signal > 0;
    }
    close(app_in[1]);
    close(g_X11.wsl_query_pipe_in[1]);
    close(g_X11.wsl_events_pipe_in[1]);

    int app_status = -1;
    waitpid(app_pid, &app_status, 0);
    waitpid(g_X11.wsl_query, NULL, 0);
    waitpid(g_X11.wsl_events, NULL, 0);

    // TODO shared cleanup routine
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_X11.termios_original);
    disable_mouse_tracking();
    free(g_coord_model.data);
    printf("\e[?25h");   // unhide cursor
    fflush(stdout);

    printf("Hunky dory!\n");

    if (killed_by_signal) // waitpid() gave 0 for whatever reason. kill() maybe?
        exit(130);
    else
        exit(WEXITSTATUS(app_status));
}

void tengi_init(void)
{
	XInitThreads();
    // Establish connection to the display service
    assert((g_X11.display = XOpenDisplay(NULL)) != NULL);
    g_X11.root = DefaultRootWindow(g_X11.display);
    XGetInputFocus(g_X11.display, &g_X11.window, &(int){0});
    XAllowEvents(g_X11.display, AsyncBoth, CurrentTime);

    // WSL depends on this
    assert((g_epoll_fd = epoll_create(1)) != -1);

    // Set stdin to raw mode
    tcgetattr(STDIN_FILENO, &g_X11.termios_original);
    g_X11.termios_current = g_X11.termios_original;
    g_X11.termios_current.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_X11.termios_current);

    XWindowAttributes xwa;

    // WSL helpers are slow, only use them if X11 fails.
    XSetErrorHandler(dummy_handler); // prevent crash
    if ( ! XGetWindowAttributes(g_X11.display, g_X11.window, &xwa) // X11 not working
        && access("/proc/sys/fs/binfmt_misc/WSLInterop", F_OK) == 0)
    { // in WSL
        XCloseDisplay(g_X11.display);
        run_wsl_controllers();
    } else
        XSetErrorHandler(xlib_error_handler);

    if (xwa.width == 1 && xwa.height == 1) { // GNOME or LX terminal
        Window parent;
        assert(XQueryTree(g_X11.display, g_X11.window, &(Window){0}, &parent, &(Window*){0}, &(unsigned){0}));
        g_X11.window = parent;
        g_is_gnome   = true;
    }

    // Must do this AFTER WSL created: stdin redirected to pipe.
    struct epoll_event epoll = {};
    epoll.events  = EPOLLIN;
    epoll.data.fd = STDIN_FILENO;
    assert(epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &epoll) != -1);

    setlocale(LC_ALL, "C.UTF-8"); // for Unicode rendering ▘▝▀▖▌▞▛▗▚▐▜▄▙▟█

    if ( ! g_X11.wsl_query)
        atexit(destroy_X11Screens);
}

// Requires enabled mouse tracking and set_raw_mode(). Returned components are
// -1 in case of not getting proper mouse escape sequence.
static IVec2 get_terminal_mouse_click_pos(int* event)
{
    char buf[64] = "";
    ssize_t buf_length = read(STDIN_FILENO, buf, sizeof buf);
    if (buf_length <= 0)
        return (IVec2){-1, -1};
    buf[buf_length - (buf_length == sizeof buf)] = '\0';

    int x = 0;
    int y = 0;
    if (sscanf(buf, "\e[%i;%i;%iM", event, &x, &y) != 3) {
        // Try using ASCII values (Windows Terminal)
        char cevent;
        char cx;
        char cy;
        if (sscanf(buf, "\e[M%c%c%c", &cevent, &cx, &cy) != 3) {
            *event = -1;
            return (IVec2){-1, -1};
        }
        *event = cevent - ' ';
        x = cx - ' ';
        y = cy - ' ';
    }
    *event &= 3; // get rid of DECSET extensions used by XTerm
    return (IVec2){ x-1, y-1 };
}

IVec2 tengi_get_global_mouse_pos(void)
{
    if ( ! g_X11.wsl_query) { // X11
        int root_x;
        int root_y;

        Window root, child;
        int win_x, win_y;
        unsigned mask_return;

        if (XQueryPointer(
            g_X11.display, g_X11.root, &root,
            &child, &root_x, &root_y, &win_x, &win_y,
            &mask_return))
            return (IVec2){ root_x, root_y };
        else
            return (IVec2){ -1, -1 };
    }
    char buf[64] = "";
    if (write(g_X11.wsl_query_pipe_in[1], "m\n", 2) == -1)
        return (IVec2){ -1, -1 };
    ssize_t buf_length = read(g_X11.wsl_query_pipe_out[0], buf, sizeof buf);
    if (buf_length <= 0 || buf[0] != 'm')
        return (IVec2){ -1, -1 };
    IVec2 result;
    if (sscanf(buf, "m%i %i\n", &result.x, &result.y) != 2)
        return (IVec2){ -1, -1 };
    return result;
}

IVec2 tengi_get_window_pos(void)
{
    if ( ! g_X11.wsl_query) { // X11
        int x, y;
        if (XTranslateCoordinates(
            g_X11.display, g_X11.window, g_X11.root, 0, 0, &x, &y, &(Window){0}))
            return (IVec2){ x, y };
        else
            return (IVec2){ -1, -1 };
    }

    char buf[128] = "";
    if (write(g_X11.wsl_query_pipe_in[1], "w\n", 2) == -1)
        return (IVec2){ -1, -1 };
    ssize_t buf_length = read(g_X11.wsl_query_pipe_out[0], buf, sizeof buf);
    if (buf_length <= 0)
        return (IVec2){ -1, -1 };

    int left, top, right, bottom;
    if (sscanf(buf, "w %i %i %i %i\n", &left, &top, &right, &bottom) != 4)
        return (IVec2){ -1, -1 };
    return (IVec2){ left, top };
}

IVec2 tengi_get_mouse_pos(void)
{
    IVec2 mouse  = tengi_get_global_mouse_pos();
    IVec2 window = tengi_get_window_pos();
    return (IVec2){mouse.x - window.x, mouse.y - window.y};
}

IVec2 tengi_get_window_size(void)
{
    if ( ! g_X11.wsl_query) { // X11
        XWindowAttributes xwa;
        if (XGetWindowAttributes(g_X11.display, g_X11.window, &xwa))
            return (IVec2){ xwa.width, xwa.height };
        else
            return (IVec2){ -1, -1 };
    }

    char buf[128] = "";
    if (write(g_X11.wsl_query_pipe_in[1], "w\n", 2) == -1)
        return (IVec2){ -1, -1 };
    ssize_t buf_length = read(g_X11.wsl_query_pipe_out[0], buf, sizeof buf);
    if (buf_length <= 0)
        return (IVec2){ -1, -1 };

    int left, top, right, bottom;
    if (sscanf(buf, "w %i %i %i %i\n", &left, &top, &right, &bottom) != 4)
        return (IVec2){ -1, -1 };
    return (IVec2){ right - left, bottom - top };
}

IVec2 tengi_get_display_size(void)
{
    if (g_X11.wsl_query)
        return (IVec2){ -1, -1 }; // TODO

    int width, height, screen;
    screen = DefaultScreen(g_X11.display);
    width  = DisplayWidth(g_X11.display, screen);
    height = DisplayHeight(g_X11.display, screen);
    return (IVec2){ width, height };
}

// Returns number of columns and rows in characters
IVec2 tengi_get_terminal_size(void)
{
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	return (IVec2){ winsize.ws_col, winsize.ws_row };
}

IVec2 tengi_get_terminal_resolution(void)
{
    IVec2 size = tengi_get_terminal_size();
    size.x *= 2;
    size.y *= 2;
    return size;
}

double tengi_time(void)
{
    #ifndef __SIZEOF_INT128__ // TODO something more portable
    #define __uint128_t uintmax_t
    #endif

    static bool			initialized = false;
    static __uint128_t	time_start;
    __uint128_t			time_now;
    struct timeval		time;

    if (!initialized)
    {
            gettimeofday(&time, NULL);
            time_start = 1000000*(__uint128_t)time.tv_sec + time.tv_usec;
            initialized = true;
    }
    gettimeofday(&time, NULL);
    time_now = 1000000*(__uint128_t)time.tv_sec + time.tv_usec;
    return (double)(time_now - time_start)/1000000;
}

// ----------------------------------------------------------------------------
// Event handling

static struct // X11
{
    void (*handler)(XEvent*, void* param);
    void* param;
} g_event_handlers[LASTEvent];

typedef enum mouse_event_type
{
    MOUSE_NO_EVENT,
    MOUSE_MOVE,
    MOUSE_PRESS,
    MOUSE_RELEASE,
    MOUSE_EVENT_TYPE_LENGTH
} MouseEventType;

static struct // ANSI mouse clicks
{
    void (*handler)(IVec2, void* param);
    void* param;
} g_mouse_handlers[MOUSE_EVENT_TYPE_LENGTH];

int g_select_mask = 0;
int g_revert;

void tengi_set_key_press_handler(void (*handler)(XKeyPressedEvent*, void* param), void* param)
{
    if (g_X11.wsl_events) // TODO
        return;

    if (handler != NULL)
        g_select_mask |=  KeyPressMask;
    else
        g_select_mask &= ~KeyPressMask;

    XSelectInput(g_X11.display, g_X11.window, g_select_mask);
    g_event_handlers[KeyPress].handler = (void(*)(XEvent*, void*))handler;
    g_event_handlers[KeyPress].param   = param;
}

void tengi_set_key_release_handler(void (*handler)(XKeyReleasedEvent*, void* param), void* param)
{
    if (g_X11.wsl_events) // TODO
        return;

    if (handler != NULL)
        g_select_mask |=  KeyReleaseMask;
    else
        g_select_mask &= ~KeyReleaseMask;

    XSelectInput(g_X11.display, g_X11.window, g_select_mask);
    g_event_handlers[KeyRelease].handler = (void(*)(XEvent*, void*))handler;
    g_event_handlers[KeyRelease].param   = param;
}

void tengi_set_mouse_press_handler(void (*handler)(IVec2 coords, void *param), void* param)
{
    if (handler != NULL)
        enable_mouse_tracking();
    else if (g_mouse_handlers[MOUSE_RELEASE].handler == NULL)
        disable_mouse_tracking();

    g_mouse_handlers[MOUSE_PRESS].handler = handler;
    g_mouse_handlers[MOUSE_PRESS].param   = param;
}

void tengi_set_mouse_release_handler(void (*handler)(IVec2 coords, void *param), void* param)
{
    if (handler != NULL)
        enable_mouse_tracking();
    else if (g_mouse_handlers[MOUSE_PRESS].handler == NULL)
        disable_mouse_tracking();

    g_mouse_handlers[MOUSE_RELEASE].handler = handler;
    g_mouse_handlers[MOUSE_RELEASE].param   = param;
}

void tengi_set_mouse_move_handler(void (*handler)(IVec2 coords, void* param), void* param)
{
    if ( ! g_X11.wsl_events) // X11
    {
        if (handler != NULL)
            g_select_mask |=  PointerMotionMask;
        else
            g_select_mask &= ~PointerMotionMask;

        if ( ! g_is_gnome &&
             ! XSelectInput(g_X11.display, g_X11.window, g_select_mask))
            return; // TODO false;
    }
    else if (write(g_X11.wsl_events_pipe_in[1], handler ? "M\n" : "m\n", 2) == -1)
        fprintf(stderr, "Error writing to wslevents pipe\n");

    g_mouse_handlers[MOUSE_MOVE].handler = handler;
    g_mouse_handlers[MOUSE_MOVE].param   = param;
}

void tengi_set_enter_window_handler(void (*handler)(XEnterWindowEvent*, void* param), void* param)
{
    if (g_X11.wsl_events) // TODO
        return;

    if (handler != NULL)
        g_select_mask |=  EnterWindowMask;
    else
        g_select_mask &= ~EnterWindowMask;

    XSelectInput(g_X11.display, g_X11.window, g_select_mask);
    g_event_handlers[EnterNotify].handler = (void(*)(XEvent*, void*))handler;
    g_event_handlers[EnterNotify].param   = param;
}

void tengi_set_leave_window_handler(void (*handler)(XLeaveWindowEvent*, void* param), void* param)
{
    if (g_X11.wsl_events) // TODO
        return;

    if (handler != NULL)
        g_select_mask |=  LeaveWindowMask;
    else
        g_select_mask &= ~LeaveWindowMask;

    XSelectInput(g_X11.display, g_X11.window, g_select_mask);
    g_event_handlers[LeaveNotify].handler = (void(*)(XEvent*, void*))handler;
    g_event_handlers[LeaveNotify].param   = param;
}

void tengi_set_resize_window_handler(void (*handler)(XConfigureEvent*, void* param), void* param)
{
    if (g_X11.wsl_events) // TODO
        return;

    if (handler != NULL)
        g_select_mask |=  StructureNotifyMask;
    else
        g_select_mask &= ~StructureNotifyMask;

    XSelectInput(g_X11.display, g_X11.window, g_select_mask);
    g_event_handlers[ConfigureNotify].handler = (void(*)(XEvent*, void*))handler;
    g_event_handlers[ConfigureNotify].param   = param;
}

void tengi_grab_keyboard(void)
{
    if (g_X11.wsl_events) // TODO
        return;

    XGrabKeyboard(g_X11.display, g_X11.root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void tengi_ungrab_keyboard(void)
{
    if (g_X11.wsl_events) // TODO
        return;

    XUngrabKeyboard(g_X11.display, CurrentTime);
}

void tengi_grab_mouse(void)
{
    if (g_X11.wsl_events) // TODO
        return;

    XGrabPointer(g_X11.display, g_X11.root, False,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
}

void tengi_ungrab_mouse(void)
{
    if (g_X11.wsl_events) // TODO
        return;

    XUngrabPointer(g_X11.display, CurrentTime);
}

void update_model(IVec2 cell)
{
	static IVec2 last_term_size;
	IVec2 term_size = tengi_get_terminal_size();

    if (term_size.x != last_term_size.x || term_size.y != last_term_size.y)
	{ // TODO also invalidate if window size changes in pixels (zoom)
		free(g_coord_model.data);
        memset(&g_coord_model, 0, sizeof g_coord_model);

        IVec2 wind_size = tengi_get_window_size();
        int size = term_size.x >= term_size.y ? term_size.x : term_size.y;
        g_coord_model = (CoordModel) {
            .alpha       = vec2(0), // TODO guesstimate borders
            .beta        = vec_div(vec2(term_size.x, term_size.y), vec2(wind_size.x, wind_size.y)),
            .length      = vec2(0),
            .size = size,
            .data        = calloc(sizeof g_coord_model.data[0], 3*size)
        };
        g_coord_model.max_errors = g_coord_model.data + 1*size;
        g_coord_model.min_errors = g_coord_model.data + 2*size;

		last_term_size = term_size;
    }

    CoordModel*const model = &g_coord_model; // shorter alias for concise math
    IVec2 pixel = tengi_get_mouse_pos();
    if (pixel.x < 0 || pixel.y < 0)
        return;

    // --------------------------------
    // Add entries

    if (model->data[cell.x].x == 0) {
        model->data[cell.x].x = model->max_errors[cell.x].x = model->min_errors[cell.x].x = pixel.x;
        model->length.x++;
    } else {
        model->max_errors[cell.x].x = fmax(model->max_errors[cell.x].x, pixel.x);
        model->min_errors[cell.x].x = fmin(model->min_errors[cell.x].x, pixel.x);
        model->data[cell.x].x = (model->max_errors[cell.x].x + model->min_errors[cell.x].x) / 2.f;
    }

    if (model->data[cell.y].y == 0) {
        model->data[cell.y].y = model->max_errors[cell.y].y = model->min_errors[cell.y].y = pixel.y;
        model->length.y++;
    } else {
        model->max_errors[cell.y].y = fmax(model->max_errors[cell.y].y, pixel.y);
        model->min_errors[cell.y].y = fmin(model->min_errors[cell.y].y, pixel.y);
        model->data[cell.y].y = (model->max_errors[cell.y].y + model->min_errors[cell.y].y) / 2.f;
    }

    // --------------------------------
    // Update model

    if (model->length.x >= 2) { // do linear regerssion
        double pixels_mean = 0.;
        double cells_mean  = 0.;
        for (uint16_t i = 0; i < model->size; ++i) {
            pixels_mean += model->data[i].x;
            if (model->data[i].x != 0)
                cells_mean += i;
        }
        pixels_mean /= model->length.x;
        cells_mean  /= model->length.x;

        double covariance = 0.;
        double variance   = 0.;
        for (uint16_t i = 0; i < model->size; ++i) {
            if (model->data[i].x == 0)
                continue;
            double diff_cells  = i - cells_mean;
            double diff_pixels = model->data[i].x - pixels_mean;
            covariance += diff_cells*diff_pixels;
            variance   += diff_pixels*diff_pixels;
        }
        model->beta.x  = covariance/variance;
        model->alpha.x = cells_mean - model->beta.x*pixels_mean;
    }

    if (model->length.y >= 2) { // do linear regerssion
        double pixels_mean = 0.;
        double cells_mean  = 0.;
        for (uint16_t i = 0; i < model->size; ++i) {
            pixels_mean += model->data[i].y;
            if (model->data[i].y != 0)
                cells_mean += i;
        }
        pixels_mean /= model->length.y;
        cells_mean  /= model->length.y;

        double covariance = 0.;
        double variance   = 0.;
        for (uint16_t i = 0; i < model->size; ++i) {
            if (model->data[i].y == 0)
                continue;
            double diff_cells  = i - cells_mean;
            double diff_pixels = model->data[i].y - pixels_mean;
            covariance += diff_cells*diff_pixels;
            variance   += diff_pixels*diff_pixels;
        }
        model->beta.y  = covariance/variance;
        model->alpha.y = cells_mean - model->beta.y*pixels_mean;
    }
}

Vec2 tengi_estimate_cell_to_pixel(IVec2 icell)
{
    Vec2 cell = vec2(icell.x, icell.y);

    if (g_coord_model.length.x < 2 || g_coord_model.length.y < 2) {
        Vec2 cell_size = tengi_estimate_cell_size();
        return vec_mul(cell, cell_size);
    }
    return vec_div(
            vec_sub(
                vec_sub(
                    cell,
                    g_coord_model.alpha),
                .5f),
            g_coord_model.beta);
}

Vec2 tengi_estimate_pixel_to_cell(IVec2 ipixel)
{
    Vec2 pixel = vec2(ipixel.x, ipixel.y);

    if (g_coord_model.length.x < 2 || g_coord_model.length.y < 2) {
        Vec2 cell_size = tengi_estimate_cell_size();
        return vec_div(pixel, cell_size);
    }
    return vec_add(
            vec_add(
                vec_mul(
                    g_coord_model.beta,
                    pixel),
                g_coord_model.alpha),
            .5f);
}

Vec2 tengi_estimate_terminal_pos(void)
{
    if (g_coord_model.length.x < 2 || g_coord_model.length.y < 2) {
        IVec2 winpos = tengi_get_window_pos();
        return vec2(winpos.x, winpos.y);
    }
    return tengi_estimate_cell_to_pixel((IVec2){0,0});
}

Vec2 tengi_estimate_terminal_size(void)
{
    if (g_coord_model.length.x < 2 || g_coord_model.length.y < 2) {
        IVec2 winsize = tengi_get_window_size();
        return vec2(winsize.x, winsize.y);
    }
    IVec2 isize = tengi_get_terminal_size();
    isize.x++;
    isize.y++;
    Vec2 left_corner  = tengi_estimate_terminal_pos();
    Vec2 right_corner = tengi_estimate_cell_to_pixel(isize);
    return vec_sub(right_corner, left_corner);
}

Vec2 tengi_estimate_cell_size(void)
{
    if (g_coord_model.length.x < 2 || g_coord_model.length.y < 2) {
        IVec2 winsize = tengi_get_window_size();
        IVec2 tersize = tengi_get_terminal_size();
        return vec2((Scalar)winsize.x/tersize.x, (Scalar)winsize.y/tersize.y);
    }
    Vec2  term_size = tengi_estimate_terminal_size();
    IVec2 i_cells   = tengi_get_terminal_size();
    return vec_div(term_size, vec2(i_cells.x, i_cells.y));
}

void tengi_handle_events(void)
{
    static IVec2 last_gnome_mouse_pos;

    int events_length = 0;
    struct epoll_event event = {};

    while ((events_length = epoll_wait(g_epoll_fd, &event, 1, 0)) > 0)
    {
        if (event.data.fd == STDIN_FILENO) // TODO we should also pass pixel coords
        {
            int type = -1;
            IVec2 coords = get_terminal_mouse_click_pos(&type);

            if (coords.x == -1 || coords.y == -1) // TODO do we really just ignore?
                continue;
            if (type == 0 && g_mouse_handlers[MOUSE_PRESS].handler != NULL) { // press
                g_mouse_handlers[MOUSE_PRESS].handler(coords, g_mouse_handlers[MOUSE_PRESS].param);
                update_model(coords);
            }
            if (type == 3 && g_mouse_handlers[MOUSE_RELEASE].handler != NULL) { // release
                g_mouse_handlers[MOUSE_RELEASE].handler(coords, g_mouse_handlers[MOUSE_RELEASE].param);
                update_model(coords);
            }
        }
        else { // wsl helpers
            char buf[64] = "";
            ssize_t buf_length = read(event.data.fd, buf, sizeof buf - sizeof"");
            IVec2 coords = { -1, -1 };
            if (buf_length != -1 && // TODO better error handling
                sscanf(buf, "m %i %i\n", &coords.x, &coords.y) == 2 &&
                g_mouse_handlers[MOUSE_MOVE].handler != NULL)
                g_mouse_handlers[MOUSE_MOVE].handler(coords, g_mouse_handlers[MOUSE_MOVE].param);
        }
    }
    assert(events_length != -1);
    if (g_X11.wsl_events)
        return;

    while (XPending(g_X11.display))
    {
        XEvent xev;
        XNextEvent(g_X11.display, &xev);

        if (xev.type == MotionNotify && g_mouse_handlers[MOUSE_MOVE].handler != NULL) {
            IVec2 coords = { xev.xmotion.x, xev.xmotion.y };
            g_mouse_handlers[MOUSE_MOVE].handler(coords, g_mouse_handlers[MOUSE_MOVE].param);
        } else if (g_event_handlers[xev.type].handler != NULL)
            g_event_handlers[xev.type].handler(&xev, g_event_handlers[xev.type].param);
    }

    if (g_is_gnome && g_mouse_handlers[MOUSE_MOVE].handler != NULL) // poll mouse move
    {
        IVec2 coords = tengi_get_mouse_pos();
        if (coords.x != last_gnome_mouse_pos.x || coords.y != last_gnome_mouse_pos.y)
            g_mouse_handlers[MOUSE_MOVE].handler(coords, g_mouse_handlers[MOUSE_MOVE].param);
        last_gnome_mouse_pos = coords;
    }
}
