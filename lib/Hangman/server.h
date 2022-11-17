#ifndef hangman_server_h
#define hangman_server_h


#include "protocol.h"


namespace Server {
    // Classe che rappresenta il server del gioco dell'impiccato
    class Server {
        private:
        // Socket del server
        int socket;
        // Socket del client
        int client_socket;
        // Indirizzo del server
        struct sockaddr_in address;
        // Indirizzo del client
        struct sockaddr_in client_address;
        // Dimensione dell'indirizzo del client
        socklen_t client_address_size;

        // 

        public:
        // Costruttore
        Server();
        // Distruttore
        ~Server();

        // Funzione che permette di avviare il server
        int start(int port);
        // Funzione che permette di chiudere il server
        int stop();

        // Funzione che mette in ascolto il server per una nuova connessione sul thread principale
        int listen();

        // Funzione che accetta una nuova connessione e la gestisce sulla GameSession
        int accept();
    };

}



#endif