#ifndef hangman_server_h
#define hangman_server_h

#include <iterator>
#include <algorithm>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#include "Markov/src/Markov.hpp"
#include "protocol.h"


#define MAX_CLIENTS 3


namespace Server {
    struct Player {
        // Socket del client
        int sockfd;
        // Nome del client
        char username[USERNAME_LENGTH];
    } typedef Player;


    // Classe che rappresenta il server del gioco dell'impiccato
    class HangmanServer {
    private:
        // Socket del server
        int sockfd;

        // Rappresenta il numero di errori massimo che i giocatori possono commettere
        uint8_t max_errors{};
        // Rappresenta il numero di errori commessi dai giocatori
        uint8_t current_errors{};
        // Rappresenta i tentativi fatti fin'ora
        std::vector<char> attempts;
        // Rappresenta quanti tentativi sono stati fatti
        uint8_t current_attempt{};
        // Rappresenta la parola o frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH]{};
        // Rappresenta la parola o frase da indovinare con i caratteri non ancora indovinati sostituiti da _
        char short_phrase_masked[SHORTPHRASE_LENGTH]{};
        // Rappresenta le lettere che non si possono indovinare all'inizio
        const char *start_blocked_letters{};
        // Rappresenta il numero di tentativi che devono essere fatti prima di poter usare le lettere bloccate
        uint8_t blocked_attempts{};
        // Lista dei client connessi
        std::vector<Player> players;
        // Rappresenta il numero di giocatori connessi
        uint8_t players_connected{};
        // Rappresenta il giocatore corrente
        Player *current_player{};


        // La Markov Chain permette di creare un modello matematico per prevedere
        // quale è la successiva parola più probabile data una parola di partenza
        Markov::Chain chain{1};


        // Funzione che permette di accettare una connessione
        void _accept();

        // Funzione che permette di carica la Markov Chain usando i testi nella cartella data
        void _load_markov_chain();

        // Funzione che permette di generare una nuova parola o frase
        void _generate_short_phrase();

        // Funzione che permette di passare il turno al giocatore successivo
        void _next_turn();

        // Funzione che permette di inviare a tutti i giocatori un'azione
        void _broadcast_action(Server::Action action);

        // Funzione che permette di inviare a tutti i giocatori un messaggio
        template<typename TypeMessage>
        void _broadcast(TypeMessage message);

        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sullo stato del gioco
        void _broadcast_update_short_phrase();

        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sui tentativi che sono stati fatti
        void _broadcast_update_attempts();

        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sulla lista dei giocatori
        void _broadcast_update_players();

        // Funzione che permette di inviare un azione a un certo giocatore
        void _send_action(Player *player, Server::Action action);

        // Funzione che permette di inviare una risposta a un certo giocatore
        void _send_response(Player *player, Client::Action action);

        // Funzione che permette di inviare un messaggio a un certo giocatore
        template<typename TypeMessage>
        void _send(Player *player, TypeMessage message);

        // Funzione che permette di leggere un messaggio da un certo giocatore
        template<typename TypeMessage>
        bool _read(Player *player, TypeMessage message, int timeout = 5);

        // Funzione che permette di eliminare un giocatore dalla lista dei giocatori
        void _remove_player(Player *player);

        // Funzione che permette di verificare se la parola o frase è stata indovinata
        bool _is_short_phrase_guessed();

        // Funzione per ottenere una lettera da un player
        int8_t _get_letter_from_player(Player *player, int timeout = 5);

        // Funzione per ottenere una frase da un player
        int8_t _get_short_phrase_from_player(Player *player, int timeout = 10);

    public:
        // Costruttore
        explicit HangmanServer(const std::string &_ip = "0.0.0.0", uint16_t _port = 8080);

        // Distruttore
        ~HangmanServer();

        // Funzione che permette di avviare il server
        void start(uint8_t _max_errors = 10, const std::string &_start_blocked_letters = "AEIOU", uint8_t _blocked_attempts = 3);

        // Funzione che rappresenta il loop del server
        void loop();

        // Funzione che permette di avviare un nuovo round
        void new_round();

        // Funzione che permette di sapere il giocatore corrente
        [[nodiscard]] Player *get_current_player() { return current_player; };

        // Funzione che permette di sapere i giocatori connessi
        [[nodiscard]] std::vector<Player> getPlayers() { return players; };

        // Funzione che permette di sapere il numero di giocatori connessi
        [[nodiscard]] int get_players_count() const { return players_connected; };

        // Funzione che permette di sapere se il server è pieno
        [[nodiscard]] bool is_full() const { return players_connected >= MAX_CLIENTS; };

        // Funzione che permette di sapere se il server è vuoto
        [[nodiscard]] bool is_empty() const { return players_connected == 0; };

        // Funzione che permette di sapere quanti errori sono stati commessi
        [[nodiscard]] int get_current_errors() const { return current_errors; };

        // Funzione che permette di sapere quanti tentativi sono stati fatti
        [[nodiscard]] int get_current_attempt() const { return current_attempt; };

        // Funzione che permette di sapere qual'è la frase corrente
        [[nodiscard]] const char *get_short_phrase() const { return short_phrase; };

        // Funzione che permette di sapere l'ultimo tentativo
        [[nodiscard]] char get_last_attempt() const { return attempts.back(); };
    };

}


#endif