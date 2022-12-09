#ifndef CLIENT_H
#define CLIENT_H

#include <iterator>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cctype>

#ifdef _WIN32
#include <chrono>
#endif

#include "protocol.h"


namespace Client {
    typedef union {
        Server::Message message;
        Server::UpdateUserMessage update_user_message;
        Server::UpdateShortPhraseMessage update_short_phrase_message;
        Server::UpdateAttemptsMessage update_attempts_message;
        Server::OtherOneTurnMessage other_one_turn_message;
    } ServerMessageUnion;


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

        /// Struttura contenente le informazioni del server
        struct sockaddr_in server_address{};

        /// Rappresenta i tentativi fatti fino al momento di accessi durante la partita
        char attempts[26]{};
        /// Rappresenta la lunghezza dell'array attempts
        int attempts_count{};

        /// Rappresenta la frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH]{};
        /// Rappresenta le lettere che non si possono usare all'inizio
        char blocked_letters[6] = "AEIOU";
        /// Rappresenta il numero di turni dopo i quali si possono usare le lettere bloccate
        uint8_t blocked_letters_round = 3;

        template<typename TypeMessage>
        bool _send(TypeMessage &message);

        template<typename TypeMessage>
        bool _receive(TypeMessage &message);

    protected:
        bool _get_letter();

        bool _get_short_phrase();

        bool _wait_for_input(int timeout);

    public:
        HangmanClient(const char address[], const char port[]);
        HangmanClient(const std::string& address, int port) : HangmanClient(address.c_str(), std::to_string(port).c_str()) {};
        HangmanClient(const std::string& address, const std::string& port) : HangmanClient(address.c_str(), port.c_str()) {};

        ~HangmanClient();

        void join(const char username[]);

        void loop();

        void run(bool verbose=true);

        void close();

        void printYourTurn(Server::Message* message);

        void waitAction();

        void printPlayerList(Server::UpdateUserMessage* message);

        void printShortPhrase(Server::UpdateShortPhraseMessage* message);

        void printAttempts(Server::UpdateAttemptsMessage* message);

        void printOtherTurn(Server::OtherOneTurnMessage* message);

        void printWin();
        
        void printLose();

        void sendLetter(char letter);

        void sendShortPhrase(const char phrase[]);
    };
}


#endif