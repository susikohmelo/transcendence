#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#if __GNUC__
#define ATOMIC _Atomic
#else
#define ATOMIC volatile // does nothing but silences people who don't know better
#endif

ATOMIC bool g_should_quit;
ATOMIC bool g_report_mouse;
ATOMIC bool g_report_window;
ATOMIC bool g_report_enter_leave;

DWORD WINAPI read_input(LPVOID _)
{
    (void)_;

    while ( ! g_should_quit) {
        int c = getchar();
        switch (c) {
        case 'M':
            g_report_mouse = true;
            break;
        case 'm':
            g_report_mouse = false;
            break;
        case 'W':
            g_report_window = true;
            break;
        case 'w':
            g_report_window = false;
            break;
        case 'E':
            g_report_enter_leave = true;
            break;
        case 'e':
            g_report_enter_leave = false;
            break;
        case EOF:
            g_should_quit = true;
            return 0;
        }
    }
    return 0;
}

static uint64_t time_ms(void)
{
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) == 0)
        return -1ull;
    return 1000llu*ts.tv_sec + ts.tv_nsec/(1000llu*1000llu);
}

static bool in_rect(RECT rect, POINT p)
{
    return (rect.left <= p.x && p.x < rect.right) && (rect.top <= p.y && p.y < rect.bottom);
}

int main(void)
{
    HANDLE thread = CreateThread(NULL, 0, read_input, NULL, 0, NULL);

    HWND window = GetActiveWindow();
    if ( ! window)
        window = GetForegroundWindow();

    POINT  p0    = {};
    POINT  p1    = {};
    RECT   rect0 = {};
    RECT   rect1 = {};

    GetCursorPos(&p1);
    GetWindowRect(window, &rect1);
    bool in_window = in_rect(rect1, p1);

    while ( ! g_should_quit) {
        uint64_t t_start = time_ms();

        if (g_report_mouse || g_report_window || g_report_enter_leave)
            GetWindowRect(window, &rect0);
        if (g_report_mouse || g_report_enter_leave)
            GetCursorPos(&p0);

        if (g_report_mouse || g_report_enter_leave)
            if (p0.x != p1.x || p0.y != p1.y)
                printf("m %li %li\n", p0.x - rect0.left, p0.y - rect0.top);

        if (g_report_window || g_report_enter_leave)
            if (rect0.left != rect1.left || rect0.top != rect1.top || rect0.right != rect1.right || rect0.bottom != rect1.bottom)
                printf("w %li %li %li %li\n", rect0.left, rect0.top, rect0.right, rect0.bottom);

        if (g_report_enter_leave) {
            if ( ! in_window && in_rect(rect0, p0))
                printf("enter\n");
            else if (in_window && ! in_rect(rect0, p0))
                printf("leave\n");
            in_window = in_rect(rect0, p0);
        }

        if (g_report_mouse || g_report_enter_leave)
            p1 = p0;
        if (g_report_mouse || g_report_window ||g_report_enter_leave)
            rect1 = rect0;

        fflush(stdout); // IMPORTANT! even with the newlines, something weird happens when piping in WSL

        uint64_t t_end = time_ms();
        if (t_end - t_start < 1000llu/120)
            Sleep(1000llu/120 - (t_end - t_start));
    }
    WaitForMultipleObjects(1, &thread, TRUE, INFINITE);
    CloseHandle(thread);
}
