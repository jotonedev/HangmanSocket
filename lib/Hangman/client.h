#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <cstring>
#include <sys/fcntl.h>
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
        /// Contiene il numero di giocatori connessi al server
        uint8_t players_count = 0;
        /// Contiene se la partita è finita
        bool game_over = true;

        /**
         * Questa funzione si occupa di inviare un messaggio al server
         * @tparam TypeMessage Un tipo di messaggio generico di 128 bytes. Quindi può essere un qualsiasi messaggio, anche del client
         * @param message Il messaggio da inviare
         * @return Lo stato di invio del messaggio
         * @retval True se l'invio è andato a buon fine
         * @retval False se l'invio è fallito
         *
         * @note Il messaggio viene inviato in maniera bloccante
         */
        template<typename TypeMessage>
        bool _send(TypeMessage &message);

        /**
         * Questa funzione si occupa di ricevere un messaggio dal server
         * @tparam TypeMessage Un tipo di messaggio generico di 128 bytes. Quindi può essere un qualsiasi messaggio, anche del client
         * @param message Il messaggio passato per reference sui cui verrà scritto il messaggio ricevuto
         * @return Lo stato di ricezione del messaggio
         * @retval True se la ricezione è andata a buon fine
         * @retval False se la ricezione è fallita
         *
         * @note Il messaggio viene inviato in maniera bloccante
         */
        template<typename TypeMessage>
        bool _receive(TypeMessage &message);

    protected:
        /**
         * Questa funzione si occupa di ricevere una lettere in input da tastiera e di inviarla al server
         *
         * @return Se la lettera ricevuta è valida
         * @retval True se è andato tutto a buon fine
         * @retval False se si ha raggiunto il timeout o la lettera è sbagliata
         */
        bool _getLetter();

        /**
         * Questa funzione si occupa di ricevere una frase in input da tastiera e di inviarla al server
         *
         * @return Se la frase ricevuta è valida
         * @retval True se è andato tutto a buon fine
         * @retval False se si ha raggiunto il timeout o la frase è sbagliata
         */
        bool _getShortPhrase();

        /**
         * Questa funzione permette di bloccare l'esecuzione fino a quando non viene ricevuto un messaggio dal server
         */
        void _waitAction();

        /**
         * Questa funzione si occupa di stampare a video che è il tuo turno
         */
        void _printYourTurn();

        /**
         * Questa funzione si occupa di stampare a video che è il turno di un altro giocatore
         * @param message Il messaggio ricevuto dal server
         */
        void _printOtherTurn(Server::OtherOneTurnMessage *message);

        /**
         * Questa funzione si occupa di stampare a video i giocatori connessi
         * @param message Il messaggio ricevuto dal server
         */
        void _printPlayerList(Server::UpdateUserMessage *message);

        /**
         * Questa funzione si occupa di stampare a video la frase da indovinare
         * @param message Il messaggio ricevuto dal server
         */
        void _printShortPhrase(Server::UpdateShortPhraseMessage *message);

        /**
         * Questa funzione si occupa di stampare a video i tentativi fatti
         * @param message Il messaggio ricevuto dal server
         */
        void _printAttempts(Server::UpdateAttemptsMessage *message);

        /**
         * Questa funzione si occupa di stampare a video il disegno dell'impiccato
         * @param mistakes Il numero di errori fatti
         */
        void _printHangman(int mistakes);

        /**
         * Questa funzione si occupa di stampare a video che il gioco è finito e i giocatori hanno vinto
         */
        void _printWin();

        /**
         * Questa funzione si occupa di stampare a video che il gioco è finito e i giocatori hanno perso
         */
        void _printLose();

    public:
        /**
         * Costruttore della classe
         * @param address L'indirizzo ip del server
         * @param port La porta del server
         */
        HangmanClient(const char address[], const char port[]);

        /**
         * Costruttore della classe
         * @param address L'indirizzo ip del server
         * @param port La porta del server
         */
        HangmanClient(const std::string &address, int port) : HangmanClient(address.c_str(),
                                                                            std::to_string(port).c_str()) {};

        /**
         * Costruttore della classe
         * @param address L'indirizzo ip del server
         * @param port La porta del server
         */
        HangmanClient(const std::string &address, const std::string &port) : HangmanClient(address.c_str(),
                                                                                           port.c_str()) {};

        /**
         * Chiude e libera il socket
         */
        ~HangmanClient();

        /**
         * Questa funzione si occupa di connettersi al server
         * @param username Lo username dell'utente
         */
        void join(const char username[]);

        /**
         * Questa funzione si occupa di gestire la partita per il client
         */
        void loop();

        /**
         * Questa funzione si occupa di gestire l'ingresso del client nel gioco e di gestire la partita
         * @param verbose Se true, verranno stampati i messaggi di errore
         */
        void run(bool verbose = true);

        /**
         * Questa funzione si occupa di chiudere la connessione col server
         */
        void close();
    };
}


#endif