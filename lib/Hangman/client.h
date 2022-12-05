#ifndef CLIENT_H
#define CLIENT_H

#include <iterator>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cctype>

#include "protocol.h"


namespace Client {
    /**
     * Questa classe rappresenta il client del gioco dell'impiccato.
     *
     * Il client si occupa di connettersi al server e inviare le richieste di gioco rispettando il protocollo.
     *
     * @note Questa classe non è thread-safe
     *
     * @authors Genero Albis Federico, Amato Davide, Toniutti John
     */
    class HangmanClient {
    private:
        /// Descrittore del socket del server
        int sockfd;

        /// Rappresenta i tentativi fatti fino al momento di accessi durante la partita
        char attempts[26];
        /// Rappresenta la lunghezza dell'array attempts
        int attempts_count;

        /// Rappresenta la frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH];
        /// Rappresenta le lettere che non si possono usare all'inizio
        char blocked_letters[6] = "AEIOU";
        /// Rappresenta il numero di turni dopo i quali si possono usare le lettere bloccate
        uint8_t blocked_letters_round = 3;

        template<typename TypeMessage>
        void _send(TypeMessage &message);

        template<typename TypeMessage>
        void _receive(TypeMessage &message);

    public:
        HangmanClient(const char address[], const char port[]);

        ~HangmanClient();

        void join(const char username[]);

        void loop();

        void run();

        void close();

        void sendLetter(char letter);

        void sendShortPhrase(const char phrase[]);

        char *getAttempts() { return attempts; };

        char *getShortPhrase() { return short_phrase; };
    };
}


#endif