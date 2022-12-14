#ifndef TERMINAL_UTILS_H
#define TERMINAL_UTILS_H

#include <iostream>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Go to x and y coordinates
*/
void gotoxy(int x, int y) {
#ifdef _WIN32
    COORD c = { static_cast<short>(x), static_cast<short>(y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE) , c);
#else
    printf("%c[%d;%df", 0x1B, y, x);
#endif
}

/**
 * Clear screen
 */
void clear_screen() {
#if defined(_WIN32) || defined(__CYGWIN__)
    system("cls");
#else
    system("clear");
#endif
}

/**
 * Struttura che rappresenta le dimensioni del terminale
 */
struct TerminalSize {
    int width;
    int height;
} typedef TerminalSize;

/**
 * Ritorna le dimensioni del terminale
 * @return Le dimensioni del terminale
 */
inline TerminalSize get_terminal_size() {
    TerminalSize size;
    size.width = 0;
    size.height = 0;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    size.width = columns;
    size.height = rows;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    size.width = w.ws_col;
    size.height = w.ws_row;
#endif

    return size;
}


/**
 * Cancella un certo numero di caratteri dal terminale a partire dalla posizione del cursore
 */
inline void clear_chars(int chars, int x = -1, int y = -1) {
    if (x != -1 && y != -1)
        gotoxy(x, y);

    // Sostituisce con degli spazi
    for (int i = 0; i < chars; i++) {
        printf(" ");
    }

    // Torna indietro
    if (x != -1 && y != -1)
        gotoxy(x, y);
}


#endif  // TERMINAL_UTILS_H
