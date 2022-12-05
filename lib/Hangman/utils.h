//
// Created by johnt on 29/11/2022.
//

#ifndef HANGMAN_GAME_UTILS_H
#define HANGMAN_GAME_UTILS_H

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "protocol.h"


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

/**
 * Rimuove spazi iniziali e finali da una stringa
 * @param s La stringa su cui fare il trim
 */
inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}


/**
 * Cambia il case della stringa in maiuscolo
 * @param str La stringa da modificare
 */
inline void str_to_upper(char *str) {
    std::transform(str, str + strlen(str), str, ::toupper);
}

/**
 * Cambia il case della stringa in maiuscolo
 * @param str La stringa da modificare
 */
inline void str_to_upper(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

/**
 * Go to x and y coordinates
*/
void gotoxy(int x, int y)
{
    printf("%c[%d;%df",0x1B,y,x);
}


/**
 * Clear screen
 */
void clear_screen() {
#ifdef _WIN32
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
inline void clear_chars(int chars) {
    // Sostituisce con degli spazi
    for (int i = 0; i < chars; i++) {
        printf(" ");
    }
    
    // Torna indietro
    for (int i = 0; i < chars; i++) {
        printf("\b");
    }
}
 


#endif //HANGMAN_GAME_UTILS_H
