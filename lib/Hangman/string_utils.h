#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>


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

#endif  // STRING_UTILS_H
