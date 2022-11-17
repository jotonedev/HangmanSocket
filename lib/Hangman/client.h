#ifndef hangman_client_h
#define hangman_client_h

#include "protocol.h"


namespace Client {
    // Classe che rappresenta il client del gioco dell'impiccato
    class Client {
        private:
        // Socket del client
        int socket;

        public:
        // Costruttore
        Client();
        // Distruttore
        ~Client();

        // Funzione che permette di connettersi al server
        int connect(const char* address, int port);
        // Funzione che permette di inviare un messaggio al server
        int send(Client::Action action);
        // Funzione che permette di ricevere un messaggio dal server
        int receive(Server::Response* response);
        // Funzione che permette di disconnettersi dal server
        int disconnect();
    };
}


#endif