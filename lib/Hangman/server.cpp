#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"

namespace Server
{
    HangmanServer::HangmanServer(const std::string &ip = "0.0.0.0", uint16_t port = 8080)
    {
        // Inizializzazione della socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }
        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        // Inizializzazione dell'indirizzo del server
        bzero(&address, sizeof(struct sockaddr_in));

        // Assegna l'indirizzo IP e la porta del server
        address.sin_family = AF_INET;
        if (ip == "0.0.0.0")
        {
            address.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            address.sin_addr.s_addr = inet_addr(ip.c_str());
        }
        address.sin_port = htons(port);

        // Collegamento della socket al server
        if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            throw std::runtime_error("Errore nel collegamento della socket al server");
        }
    }

    HangmanServer::~HangmanServer()
    {
        // Chiusura della socket
        if(shutdown(sockfd, SHUT_RDWR) < 0) {
            throw std::runtime_error("Errore nella chiusura della socket");
        }
    }

    void HangmanServer::start(int max_errors, char* start_blocked_letters, int blocked_attemps)
    {
        // Inizializzazione delle variabili
        this->max_errors = max_errors;
        this->current_errors = 0;
        this->current_attempt = 0;
        this->start_blocked_letters = start_blocked_letters;
        this->blocked_attemps = blocked_attemps;
        this->current_player = nullptr;

        // Generazione della parola o frase da indovinare
        _generate_shortphrase();

        // Inizializzazione della lista dei giocatori
        players.clear();

        // Avvio del server
        if (listen(sockfd, MAX_CLIENTS) < 0)
        {
            throw std::runtime_error("Errore nell'avvio del server");
        }
    }

    void HangmanServer::stop()
    {
        // Chiusura della socket
        if(shutdown(sockfd, SHUT_RDWR) < 0) {
            throw std::runtime_error("Errore nella chiusura della socket");
        }
    }

    void HangmanServer::new_round() {
        // Inizializzazione delle variabili
        this->current_errors = 0;
        this->current_attempt = 0;
        this->current_player = nullptr;

        // Generazione della parola o frase da indovinare
        _generate_shortphrase();
    }

    bool HangmanServer::_is_shortphrase_guessed() {
        for (int i = 0; i < SHORTPHRASE_LENGTH; i++) {
            if (shortphrase[i] == '_') {
                return false;
            }
        }

        return strncasecmp(shortphrase, shortphrase_masked, SHORTPHRASE_LENGTH) == 0;
    }
}
