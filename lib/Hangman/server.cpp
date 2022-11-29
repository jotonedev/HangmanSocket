#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cctype>
#include <fstream>
#include <iostream>

#include "utils.h"
#include "server.h"


namespace Server {
    HangmanServer::HangmanServer(const string &ip, uint16_t port) {
        // Inizializzazione del socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }
        // Imposta il sockfd in modalità non bloccante
        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        // Imposta il timeout di 5ms per la lettura
        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 5000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);

        // Inizializzazione dell'indirizzo del server
        bzero(&address, sizeof(struct sockaddr_in));

        // Assegna l'indirizzo IP e la porta del server
        address.sin_family = AF_INET;
        if ( inet_aton( ip.c_str(), &address.sin_addr ) == 0) {
            throw std::runtime_error("Errore nella conversione dell'indirizzo IP");
        }
        address.sin_port = htons(port);

        // Collegamento della sockfd al server
        if (bind(sockfd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            throw std::runtime_error("Errore nel collegamento della socket al server");
        }
    }

    HangmanServer::~HangmanServer() {
        // Chiusura della sockfd
        shutdown(sockfd, SHUT_RDWR);

        // Elimina i giocatori
        for (auto &player: players) {
            _remove_player(&player);
        }
    }

    void HangmanServer::close() {
        // Chiusura della sockfd
        shutdown(sockfd, SHUT_RDWR);

        // Elimina i giocatori
        for (auto &player: players) {
            _remove_player(&player);
        }
    }

    void
    HangmanServer::start(uint8_t _max_errors, const string &_start_blocked_letters, uint8_t _blocked_attempts, const string &filename) {
        // Inizializzazione delle variabili
        this->max_errors = _max_errors;
        this->current_errors = 0;
        this->current_attempt = 0;
        this->start_blocked_letters = _start_blocked_letters.c_str();
        this->blocked_attempts = _blocked_attempts;
        this->current_player = nullptr;
        this->attempts.clear();

        // Carica le frasi dal file
        _load_short_phrases(filename);

        // Generazione della parola o frase da indovinare
        _generate_short_phrase();

        // Inizializzazione della lista dei giocatori
        players.clear();

        // Avvio del server
        if (listen(sockfd, MAX_CLIENTS) < 0) {
            throw std::runtime_error("Errore nell'avvio del server");
        }


    }

    void HangmanServer::new_round() {
        // Inizializzazione delle variabili
        this->current_errors = 0;
        this->current_attempt = 0;
        this->current_player = nullptr;
        this->attempts.clear();

        // Generazione della parola o frase da indovinare
        _generate_short_phrase();
    }

    bool HangmanServer::_is_short_phrase_guessed() {
        for (char c: short_phrase) {
            if (c == '_') {
                return false;
            }
        }

        return strncasecmp(short_phrase, short_phrase_masked, SHORTPHRASE_LENGTH) == 0;
    }

    void HangmanServer::_remove_player(Player *player) {
        // Elimina il giocatore dalla lista dei player connessi
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (players.at(i).sockfd == player->sockfd) {
                players.erase(players.begin() + i);
                break;
            }
        }

        if (current_player != nullptr && current_player->sockfd == player->sockfd) {
            current_player = nullptr;
        }

        // Chiude la connessione con il giocatore
        shutdown(player->sockfd, SHUT_RDWR);
        players_connected--;

        // Elimina il giocatore
        delete player;
    }

    template<typename TypeMessage>
    bool HangmanServer::_read(Player *player, TypeMessage &message, int timeout) {
        // Legge il messaggio dal giocatore
        int n = read(player->sockfd, &message, sizeof(Message));

        // Se il giocatore ha chiuso la connessione
        if (n == 0) {
            _remove_player(player);
            return false;
        }

        // Se il giocatore ha inviato un messaggio
        if (n > 0) {
            return true;
        }

        // Se il giocatore non ha inviato un messaggio
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Se il giocatore ha superato il timeout
            if (timeout <= 0) {
                // Se il giocatore ha superato il timeout
                return false;
            }

            // Aspetta 1ms
            usleep(1000);

            // Ritenta la lettura (32 è il numero massimo di chiamate ricorsive)
            if (timeout < 32)
                return _read(player, message, timeout - 1);
        }

        // Se il giocatore ha chiuso la connessione
        _remove_player(player);
        return false;
    }

    template<typename TypeMessage>
    void HangmanServer::_send(Player *player, TypeMessage &message) {
        if (send(player->sockfd, &message, sizeof(Message), 0) < 0) {
            _remove_player(player);
        }
    }

    template<typename TypeMessage>
    void HangmanServer::_broadcast(TypeMessage message) {
        for (auto player: players) {
            _send(&player, message);
        }
    }

    void HangmanServer::_send_action(Player *player, Server::Action action) {
        Message packet;
        packet.action = action;

        _send(player, packet);
    }

    void HangmanServer::_send_response(Player *player, Client::Action action) {
        Client::Message packet;
        packet.action = action;

        _send(player, packet);
    }


    void HangmanServer::_broadcast_action(Server::Action action) {
        Message packet;
        packet.action = action;

        _broadcast(packet);
    }

    void HangmanServer::_broadcast_update_short_phrase() {
        UpdateShortPhraseMessage packet;
        packet.errors = current_errors;
        strncpy(packet.short_phrase, short_phrase_masked, SHORTPHRASE_LENGTH);

        _broadcast(&packet);
    }

    void HangmanServer::_broadcast_update_players() {
        UpdateUserMessage packet;
        packet.user_count = players_connected;

        for (uint i = 0; i < players_connected; i++) {
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
            for (auto player: players) {
                if (player.sockfd == current_player->sockfd)
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
        OtherOneTurnMessage packet;
        strncpy(packet.player_name, current_player->username, USERNAME_LENGTH);

        for (auto player: players) {
            if (player.sockfd == current_player->sockfd)
                continue;

            _send(&player, packet);
        }
    }

    void HangmanServer::_accept() {
        struct sockaddr_in client_address{};
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(sockfd, (struct sockaddr *) &client_address, &client_address_len);

        if (client_socket < 0) {
            return;
        }

        // Aggiunge il giocatore alla lista
        // Non possiamo ancora aggiungere il suo nome perché non è ancora stato inviato
        Player player;
        player.sockfd = client_socket;

        // Legge il nome del giocatore
        Client::JoinMessage packet;

        if (recv(client_socket, &packet, sizeof(packet), MSG_WAITALL) < 0) {
            return;
        }

        // Copia il nome del giocatore nella lista
        strncpy(player.username, packet.username, USERNAME_LENGTH);

        players.push_back(player);
        players_connected++;

        // Invia il messaggio di aggiornamento della lista dei giocatori
        _broadcast_update_players();
    }

    void HangmanServer::_load_short_phrases(const std::string &filename) {
        all_phrases.clear();
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("Errore nell'apertura del file");
        }

        std::string temp_text;
        while (std::getline(file, temp_text)) {
            str_to_upper(temp_text);
            trim(temp_text);
            all_phrases.push_back(temp_text);
        }

        file.close();
    }

    void HangmanServer::_generate_short_phrase() {
        // Prende una frase random
        int index = rand() % all_phrases.size();

        bzero(short_phrase, SHORTPHRASE_LENGTH);
        strncpy(short_phrase, all_phrases.at(index).c_str(), SHORTPHRASE_LENGTH);

        // Maschera la frase
        bzero(short_phrase_masked, SHORTPHRASE_LENGTH);
        for (int i = 0; i < SHORTPHRASE_LENGTH; i++) {
            if (short_phrase[i] == ' ') {
                short_phrase_masked[i] = ' ';
            } else if (short_phrase[i] == '\0') {
                continue;
            } else {
                    short_phrase_masked[i] = '_';
            }
        }
    }

    status HangmanServer::_get_letter_from_player(Player *player, int timeout) {
        Client::LetterMessage packet;
        _read(player, packet, timeout);

        if (packet.action != Client::Action::LETTER) {
            _remove_player(player);
            return -2;
        }
        packet.letter = toupper(packet.letter); // NOLINT(cppcoreguidelines-narrowing-conversions)

        if (isalpha(packet.letter) == 0) {
            _send_response(player, Client::Action::LETTER_REJECTED);
            return -1;
        }

        // Controlla se la lettera è bloccata
        if (current_attempt > blocked_attempts) {
            for (int i = 0; i < strlen(start_blocked_letters); i++) {
                if (start_blocked_letters[i] == packet.letter) {
                    _send_response(player, Client::Action::LETTER_REJECTED);
                    return -1;
                }
            }
        }

        // Controlla se la lettera è già stata usata
        for (auto letter: attempts) {
            if (letter == packet.letter) {
                _send_response(player, Client::Action::LETTER_REJECTED);
                return -1;
            }
        }

        // Aggiunge la lettera alla lista delle lettere usate
        current_attempt++;
        attempts.push_back(packet.letter);

        // Controlla se la lettera è presente nella frase
        bool found = false;
        for (int i = 0; i < SHORTPHRASE_LENGTH; i++) {
            if (short_phrase[i] == packet.letter) {
                short_phrase_masked[i] = packet.letter;
                found = true;
            }
        }

        if (found) {
            _send_response(player, Client::Action::LETTER_ACCEPTED);
            return 1;
        } else {
            current_errors++;
            _send_response(player, Client::Action::LETTER_REJECTED);
            return 0;
        }
    }

    status HangmanServer::_get_short_phrase_from_player(Player *player, int timeout) {
        Client::ShortPhraseMessage packet;
        _read(player, packet, timeout);

        // Controlla se il pacchetto è corretto
        if (packet.action != Client::Action::SHORT_PHRASE) {
            _remove_player(player);
            return -2;
        }

        // Controlla se la frase è corretta
        str_to_upper(packet.short_phrase);
        if (strncmp(packet.short_phrase, short_phrase, SHORTPHRASE_LENGTH) == 0) {
            _send_response(player, Client::Action::SHORT_PHRASE_ACCEPTED);
            return 1;
        } else {
            _send_response(player, Client::Action::SHORT_PHRASE_REJECTED);
            return 0;
        }
    }

    void HangmanServer::_broadcast_update_attempts() {
        // Invia il messaggio di aggiornamento delle lettere usate
        Server::UpdateAttemptsMessage packet;
        packet.action = Server::Action::UPDATE_ATTEMPTS;
        packet.max_errors = max_errors;
        packet.errors = current_errors;

        if (current_attempt > 26) {
            packet.attempts = 26;
        } else {
            packet.attempts = current_attempt;
        }

        packet.attempts = current_attempt;
        std::sort(attempts.begin(), attempts.end(),
                  [](unsigned char c1, unsigned char c2) { return c1 < c2; });
        strncpy(packet.attempts_list, attempts.data(), packet.attempts);
    }

    void HangmanServer::loop() {
        // Controlla se ci sono nuove connessioni
        _accept();

        // Se il numero di giocatori è insufficiente, non fa nulla
        if (players_connected == 0) {
            return;
        }

        // Se il numero di giocatori è sufficiente, allora continua con il turno successivo
        _next_turn();
        if (_get_letter_from_player(current_player) < 0)
            return;
        _get_short_phrase_from_player(current_player);

        if (_is_short_phrase_guessed()) {
            _broadcast_action(Action::WIN);
            usleep(10'000);
            new_round();

            return;
        } else if (current_errors == max_errors) {
            _broadcast_action(Action::LOSE);
            usleep(10'000);
            new_round();

            return;
        } else {
            _broadcast_update_short_phrase();
            _broadcast_update_attempts();
        }
    }

    void HangmanServer::run(const bool verbose) {
        int prev_n_players = 0;

        try {
            start();
        } catch (const std::exception &e) {
            if (verbose)
                std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE );
        }

        // Scrive a schermo l'indirizzo IP del server e la sua porta
        if (verbose) {
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, str, INET_ADDRSTRLEN);
            std::cout << "Server address: " << str << "\n";
            std::cout << "Server port: " << ntohs(address.sin_port)  << "\n\n";
        }

        cout << "Short phrase: " << short_phrase << std::endl;

        while (true) {
            try {
                loop();

                if (verbose && players_connected > 0) {
                    cout << "Players connected: " << players_connected << "\n";
                    cout << "Current player: " << current_player->username << "\n";
                    cout << "Current errors: " << current_errors << "\n";
                    cout << "Current attempt: " << current_attempt << "\n";
                    cout << "Short phrase: " << short_phrase << "\n";
                    cout << "Short phrase masked: " << short_phrase_masked << "\n";
                    cout << "Attempts: ";
                    for (char i: attempts)
                        cout << i << ' ';
                    cout << "\n";
                }

                if (verbose && players_connected > 0 && prev_n_players == 0)
                    cout << "Exited idle state" << "\n";
                else if (verbose && players_connected == 0 && prev_n_players > 0)
                    cout << "Entered idle state" << "\n";

                prev_n_players = players_connected;
            }
            catch (const std::exception &e) {
                if (verbose)
                    std::cerr << e.what() << std::endl;
            }
        }
    }
}
