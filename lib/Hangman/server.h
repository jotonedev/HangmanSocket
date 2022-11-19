#ifndef hangman_server_h
#define hangman_server_h

#include <iterator>
#include <algorithm>
#include <vector>
#include "protocol.h"


#define MAX_CLIENTS 3


namespace Server {
    struct Player {
        // Socket del client
        int socket;
        // Nome del client
        char username[64];
        // Indica se il client è pronto
        bool ready;
    } typedef Player;


    // Classe che rappresenta il server del gioco dell'impiccato
    class HangmanServer {
        private:
        // Socket del server
        int sockfd;
        // Indirizzo del server
        struct sockaddr_in address;

        // Rappresenta il numero di errori massimo che i giocatori possono commettere
        int max_errors;
        // Rappresenta il numero di errori commessi dai giocatori
        int current_errors;
        // Rappresenta quanti tentativi sono stati fatti
        int current_attempt;
        // Rappresenta la parola o frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH];
        // Rappresenta la parola o frase da indovinare con i caratteri non ancora indovinati sostituiti da _
        char short_phrase_masked[SHORTPHRASE_LENGTH];
        // Rappresenta le lettere che non si possono indovinare all'inizio
        char* start_blocked_letters;
        // Rappresenta il numero di tentativi che devono essere fatti prima di poter usare le lettere bloccate
        int blocked_attemps;
        // Lista dei client connessi
        std::vector<Player> players;
        // Rappresenta il numero di giocatori connessi
        int players_connected;
        // Lista dei client in attesa di essere connessi
        std::vector<Player> waiting_players;
        // Rappresenta il numero di giocatori in attesa di essere connessi
        int waiting_players_connected;
        // Rappresenta il giocatore corrente
        Player* current_player;


        // Funzione che permette di accettare una connessione
        void _accept();
        // Funzione che permette di generare una nuova parola o frase
        void _generate_shortphrase();
        // Funzione che permette di passare il turno al giocatore successivo
        void _next_turn();
        // Funzione che permette di inviare a tutti i giocatori un'azione
        void _broadcast_action(Server::Action action);
        // Funzione che permette di inviare a tutti i giocatori un messaggio
        template<typename TypeMessage> void _broadcast(TypeMessage message);
        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sullo stato del gioco
        void _broadcast_update_shortphrase();
        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sullo lista dei giocatori
        void _broadcast_update_players();
        // Funzione che permette di inviare un azione a un certo giocatore
        void _send_action(Player* player, Server::Action action);
        // Funzione che permette di inviare un messaggio a un certo giocatore
        template<typename TypeMessage> void _send(Player* player, TypeMessage message);

        // Funzione che permette di aspettare per l'invio di un messaggio da parte di un giocatore dato un timeout
        template<typename TypeMessage> bool _wait_for_message(Player* player, int timeout, TypeMessage* message);

        // Funzione che permette di verificare se la parola o frase è stata indovinata
        bool _is_short_phrase_guessed();


        public:
        // Costruttore
        explicit HangmanServer(const std::string & _ip = "0.0.0.0", uint16_t _port=8080);
        // Distruttore
        ~HangmanServer();

        // Funzione che permette di avviare il server
        void start(int _max_errors = 10, char* _start_blocked_letters = "aeiou", int _blocked_attemps = 3);
        // Funzione che permette di chiudere il server
        void stop();

        // Funzione che rappresenta il loop del server
        void loop();

        // Funzione che permette di avviare un nuovo round
        void new_round();

        // Funzione che permette di sapere il giocatore corrente
        [[nodiscard]] Player* get_current_player() { return current_player; };
        // Funzione che permette di sapere i giocatori connessi
        [[nodiscard]] std::vector<Player> getPlayers() { return players; };
        // Funzione che permette di sapere il numero di giocatori connessi
        [[nodiscard]] int get_players_count() const { return players_connected; };
        // Funzione che permette di sapere se il server è pieno
        [[nodiscard]] bool is_full() const { return players_connected >= MAX_CLIENTS; };
        // Funzione che permette di sapere se il server è vuoto
        [[nodiscard]] bool is_empty() const { return players_connected == 0; };
    };

}



#endif