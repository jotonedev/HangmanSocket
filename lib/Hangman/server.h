#ifndef hangman_server_h
#define hangman_server_h

#include <forward_list>
#include <iterator>
#include <algorithm>
#include "protocol.h"


#define MAX_CLIENTS 3


namespace Server {
    struct Player {
        // Socket del client
        int socket;
        // Nome del client
        char name[64];
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
        char shortphrase[SHORTPHRASE_LENGTH];
        // Rappresenta la parola o frase da indovinare con i caratteri non ancora indovinati sostituiti da _
        char shortphrase_masked[SHORTPHRASE_LENGTH];
        // Rappresenta le lettere che non si possono indovinare all'inizio
        char* start_blocked_letters;
        // Rappresenta il numero di tentativi che devono essere fatti prima di poter usare le lettere bloccate
        int blocked_attemps;
        // Lista dei client connessi
        std::forward_list<Player> players;
        // Rappresenta il numero di giocatori connessi
        int players_connected;
        // Rappresenta il giocatore corrente
        Player* current_player;


        // Funzione che permette di accettare una connessione
        void _accept();
        // Funzione che permette di generare una nuova parola o frase
        void _generate_shortphrase();
        // Funzione che permette di passare il turno al giocatore successivo
        void _next_turn();
        // Funzione che permette di inviare a tutti i giocatori un messaggio
        void _broadcast(Server::Action action);
        void _broadcast(void* message);
        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sullo stato del gioco
        void _broadcast_update_shortphrase();
        // Funzione che permette di inviare a tutti i giocatori un aggiornamento sullo lista dei giocatori
        void _broadcast_update_players();
        // Funzione che permette di inviare un azione ad un certo giocatore
        void _send_action(Player* player, Server::Action action);
        template <class TypeMessage> void _send_action(Player* player, TypeMessage message);

        // Funzione che permette di verificare se la parola o frase è stata indovinata
        bool _is_shortphrase_guessed();


        public:
        // Costruttore
        HangmanServer(const std::string & _ip = "0.0.0.0", uint16_t _port=8080);
        // Distruttore
        ~HangmanServer();

        // Funzione che permette di avviare il server
        void start(int max_errors = 10, char* start_blocked_letters = "aeiou", int blocked_attemps = 3);
        // Funzione che permette di chiudere il server
        void stop();

        // Funzione che rappresenta il loop del server
        void loop();

        // Funzione che permette di avviare un nuovo round
        void new_round();

        // Funzione che permette di sapere il giocatore corrente
        Player* getCurrentPlayer() { return current_player; };
        // Funzione che permette di sapere i giocatori connessi
        std::forward_list<Player> getPlayers() { return players; };
        // Funzione che permette di sapere il numero di giocatori connessi
        int getPlayersCount() { return players_connected; };
        // Funzione che permette di sapere se il server è pieno
        bool isFull() { return players_connected >= MAX_CLIENTS; };
        // Funzione che permette di sapere se il server è vuoto
        bool isEmpty() { return players_connected == 0; };
    };

}



#endif