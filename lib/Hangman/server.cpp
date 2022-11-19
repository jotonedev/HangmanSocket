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
#include <type_traits>

#include "server.h"

namespace Server
{
    HangmanServer::HangmanServer(const std::string &ip, uint16_t port)
    {
        // Inizializzazione della socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }
        // Imposta il socket in modalità non bloccante
        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        // Imposta il timeout di 5 ms per la lettura
        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 5000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

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

    void HangmanServer::start(int _max_errors, char* _start_blocked_letters, int _blocked_attemps)
    {
        // Inizializzazione delle variabili
        this->max_errors = _max_errors;
        this->current_errors = 0;
        this->current_attempt = 0;
        this->start_blocked_letters = _start_blocked_letters;
        this->blocked_attemps = _blocked_attemps;
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

    bool HangmanServer::_is_short_phrase_guessed() {
        for (char c : short_phrase) {
            if (c == '_') {
                return false;
            }
        }

        return strncasecmp(short_phrase, short_phrase_masked, SHORTPHRASE_LENGTH) == 0;
    }

    template<typename TypeMessage>
    void HangmanServer::_send(Player* player, TypeMessage message) {
        if (send(player->socket, &message, sizeof(message), 0) < 0) {
            throw std::runtime_error("Errore nell'invio dei dati");
        }
    }

    template<typename TypeMessage>
    void HangmanServer::_broadcast(TypeMessage message) {
        for (auto player : players) {
            _send(&player, message);
        }
    }

    void HangmanServer::_send_action(Player* player, Server::Action action) {
        Message packet;
        bzero(&packet, sizeof(packet));
        packet.action = action;

        _send(player, packet);
    }


    void HangmanServer::_broadcast_action(Server::Action action) {
        Message packet;
        bzero(&packet, sizeof(packet));
        packet.action = action;

        _broadcast(packet);
    }

    void HangmanServer::_broadcast_update_shortphrase() 
    {
        UpdateShortPhraseMessage packet;
        bzero(&packet, sizeof(packet));
        packet.errors = current_errors;
        strncpy(packet.shortphrase, short_phrase_masked, SHORTPHRASE_LENGTH);

        _broadcast(&packet);
    }

    void HangmanServer::_broadcast_update_players() {
        UpdateUserMessage packet;
        bzero(&packet, sizeof(packet));
        packet.user_count = players_connected;

        for (int i=0; i<players_connected; i++) {
            strncpy(packet.usernames[i], players.at(i).username, USERNAME_LENGTH);
        }

        _broadcast(packet);
    }

    void HangmanServer::_next_turn() {
        if (current_player == nullptr) {
            current_player = &players.at(0);
        } else {  // Se non è il primo turno, determina il giocatore successivo
            // Trova l'indice del giocatore corrente
            int index = 0;
            for (auto player : players) {
                if (player.socket == current_player->socket)
                    break;

                index++;
            }

            // Determina il giocatore successivo
            if (index == players_connected - 1) {
                current_player = &players.at(0);
            } else {
                current_player = &players.at(index + 1);
            }
        }

        // Invia il messaggio di turno al giocatore corrente
        _send_action(current_player, Action::YOUR_TURN);

        // Invia il messaggio di turno agli altri giocatori
        for (auto player : players) {
            if (player.socket == current_player->socket)
                continue;

            _send_action(&player, Action::OTHER_TURN);
        }
    }

    void HangmanServer::_generate_shortphrase() {

    }

    void HangmanServer::_accept() {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(sockfd, (struct sockaddr *)&client_address, &client_address_len);

        if (client_socket < 0) {
            throw std::runtime_error("Errore nell'accettazione della connessione");
        }

        // Aggiunge il giocatore alla lista
        // Non possiamo ancora aggiungere il suo nome perché non è ancora stato inviato
        Player player;
        player.socket = client_socket;
        player.ready = false;
        bzero(player.username, USERNAME_LENGTH);

        // Legge il nome del giocatore
        Client::JoinMessage packet;
        if (recv(client_socket, &packet, sizeof(packet), 0) < 0) {
            throw std::runtime_error("Errore nella ricezione dei dati");
        }

        if (strncpy(player.username, packet.username, USERNAME_LENGTH)) {
            throw std::runtime_error("Errore nella lettura dello username");
        }

        players.push_back(player);
        players_connected++;

        // Invia il messaggio di aggiornamento della lista dei giocatori
        _broadcast_update_players();
    }
}
