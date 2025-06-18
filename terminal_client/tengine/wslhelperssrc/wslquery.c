#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

int main(void)
{
    HWND window = GetActiveWindow();
    if ( ! window)
        window = GetForegroundWindow();
    if ( ! window)
        return !!fprintf(stderr, "wslquery: cannot get window\n");

    POINT p;
    RECT  rect;

    while (true) {
        fflush(stdout); // IMPORTANT! even with the newlines, something weird happens when piping in WSL
        int c = getchar();

        switch (c) {
        case 'm':
            if (GetCursorPos(&p))
                printf("m %li %li\n", p.x, p.y);
            else
                printf("m error\n");
            break;

        case 'w':
            if (GetWindowRect(window, &rect))
                printf("w %li %li %li %li\n", rect.left, rect.top, rect.right, rect.bottom);
            else
                printf("w error\n");
            break;

        case EOF:
            return EXIT_SUCCESS;
        }
    }
}
