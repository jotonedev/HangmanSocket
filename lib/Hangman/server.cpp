#include "server.h"


namespace Server {
    HangmanServer::HangmanServer(const string &ip, uint16_t port) {
        // Inizializzazione del socket
#ifdef _WIN32
        WSADATA wsa_data;
        WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }

        // Imposta il sockfd in modalità non bloccante
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sockfd, FIONBIO, &mode);
#else
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

        // Inizializzazione dell'indirizzo del server
        bzero(&address, sizeof(struct sockaddr_in));

        // Imposta il tipo di ip e la porta
        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        // Converte la stringa in un indirizzo ip
        if (inet_aton(ip.c_str(), &address.sin_addr) == 0) {
            throw std::runtime_error("Errore nella conversione dell'indirizzo IP");
        }


        // Associa il socket all'indirizzo ip e alla porta specificati
        if (bind(sockfd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            throw std::runtime_error("Errore nel collegamento della socket al server");
        }
    }

    HangmanServer::~HangmanServer() {
        // Chiusura della sockfd
        shutdown(sockfd, SHUT_RDWR);

#ifdef _WIN32
        WSACleanup();
#endif
        closesocket(sockfd);

        // Elimina i giocatori
        for (auto &player: players) {
            _remove_player(&player);
        }
    }

    void
    HangmanServer::start(uint8_t _max_errors, const string &_start_blocked_letters, uint8_t _blocked_attempts,
                         const string &_filename) {
        // Inizializzazione delle variabili
        this->max_errors = _max_errors;
        this->current_errors = 0;
        this->current_attempt = 0;
        this->start_blocked_letters = _start_blocked_letters.c_str();
        this->blocked_attempts = _blocked_attempts;
        this->current_player = nullptr;
        this->attempts.clear();
        this->players_connected = 0;

        // Carica le frasi dal file
        _load_short_phrases(_filename);

        // Inizializzazione della lista dei giocatori
        players.clear();

        // Avvio del server
        if (listen(sockfd, MAX_CLIENTS) < 0) {
            throw std::runtime_error("Errore nell'avvio del server");
        }

        new_round();
    }

    void HangmanServer::new_round() {
        _broadcast_action(Action::NEW_GAME);

        // Inizializzazione delle variabili
        this->current_errors = 0;
        this->current_attempt = 0;
        this->current_player = nullptr;
        this->attempts.clear();

        // Generazione della parola o frase da indovinare
        _generate_short_phrase();

        // Invia tutti i dati della partita ai player connessi
        for (auto &player: players) {
            _send_update_players(player);
            _send_update_attempts(player);
            _send_update_short_phrase(player);
        }
    }

    inline bool HangmanServer::_is_short_phrase_guessed() {
        return strncasecmp(short_phrase, short_phrase_masked, SHORTPHRASE_LENGTH) == 0;
    }

    void HangmanServer::_remove_player(Player *player) {
        // Elimina il giocatore dalla lista dei player connessi
        bool removed = false;
        for (unsigned int i = 0; i < players_connected; i++) {
            if (players.at(i).sockfd == player->sockfd) {
                players.erase(players.begin() + i);
                removed = true;
                break;
            }
        }

        // Se non è stato rimosso significa che era già stato disconesso
        if (!removed) {
            return;
        }

        // Se il giocatore è il giocatore corrente, passa il turno al giocatore successivo
        if (current_player != nullptr && current_player->sockfd == player->sockfd) {
            current_player = nullptr;
        }

        // Chiude la connessione con il giocatore
        shutdown(player->sockfd, SHUT_RDWR);
        closesocket(player->sockfd);

        // Aggiorna il contatore dei giocatori connessi
        players_connected = players.size();
    }

    void HangmanServer::_check_disconnected_players() {
        // copia la lista dei giocatori connessi
        std::vector<Player> players_copy = players;

        if (players_copy.size() == 0) {
            return;
        }

        // Invia un heartbeat a tutti i giocatori connessi
        // se un giocatore non risponde significa che si è disconesso
        bool removed = false;
        Client::Message heartbeat_resp;
        for (auto &player: players_copy) {
            bool res = _send_action(&player, Action::HEARTBEAT);
            if (!res) {
                removed |= true;
                _remove_player(&player);
                continue;
            }

            res = _read(&player, heartbeat_resp, Client::Action::HEARTBEAT, 1);
            if (!res) {
                removed |= true;
                _remove_player(&player);
            }
        }

        if (removed && players_connected > 0) {
            for (auto &player: players) {
                _send_update_players(player);
            }
        }
    }

    template<class TypeMessage>
    bool HangmanServer::_read(Player *player, TypeMessage &message, Client::Action action, int timeout) {
        int n = 0;

        // Aspetta di ricevere un messaggio entro il timeout in secondi dato
        for (int i = 0; i < timeout * 10; i++) {
            n = recv(player->sockfd, (char *) &message, sizeof(TypeMessage), MSG_NOSIGNAL);
            if (n > 0) {
                break;
            }
            // Dorme per 100 millisecond
            usleep(100'000);
        }


        // Se il giocatore ha chiuso la connessione
        if (n == 0) {
            return false;
        }

        // Se il giocatore ha inviato un messaggio
        if (n == MessageSize) {
            // Se il messaggio inviato ha un action diversa da quella richiesta
            if (action != Client::GENERIC && message.action != action) {
                return false;
            } else {
                return true;
            }
        }

        // Se il giocatore ha chiuso la connessione
        return false;
    }

    template<typename TypeMessage>
    bool HangmanServer::_send(Player *player, TypeMessage &message) {
        int res = send(player->sockfd, (char *) (&message), sizeof(Message), MSG_NOSIGNAL);

        if (res == EAGAIN)
            res = send(player->sockfd, (char *) (&message), sizeof(Message), MSG_NOSIGNAL);

        if (res < 0) {
            return false;
        } else {
            return true;
        }
    }

    bool HangmanServer::_send_action(Player *player, Server::Action action) {
        Message packet;
        packet.action = action;

        return _send(player, packet);
    }

    inline void HangmanServer::_broadcast_action(Server::Action action) {
        Message packet;
        packet.action = action;

        for (auto &player: players) {
            _send(&player, packet);
        }
    }

    inline void HangmanServer::_send_update_short_phrase(Server::Player &player) {
        UpdateShortPhraseMessage packet;
        packet.errors = current_errors;
        strncat(packet.short_phrase, short_phrase_masked, SHORTPHRASE_LENGTH - 1);

        _send(&player, packet);
    }

    inline void HangmanServer::_send_update_players(Server::Player &player) {
        UpdateUserMessage packet;
        packet.user_count = players_connected;

        for (unsigned int i = 0; i < players_connected; i++) {
            strncat(packet.usernames[i], players.at(i).username, USERNAME_LENGTH - 1);
        }

        _send(&player, packet);
    }

    void HangmanServer::_next_turn() {
        if (current_player == nullptr) {
            current_player = &(players.at(0));
        } else {  // Se non è il primo turno, determina il giocatore successivo
            // Trova l'indice del giocatore corrente
            unsigned int i;
            for (i = 0; i < players_connected; i++) {
                if (players.at(i).sockfd == current_player->sockfd)
                    break;
            }

            // Ho uno strano bug per il quale i supera di valore players_connected, seppur ciò sia impossibile
            // Non ho idea da dove provenga il bug se dal compilatore o radiazione cosmica
            // Ma per ora lo fixo così
            if (i >= players_connected)
                i = players_connected - 1;

            // Determina il giocatore successivo
            if (i == players_connected - 1) {
                current_player = &players.at(0);
            } else {
                current_player = &players.at(i + 1);
            }
        }

        // Invia il messaggio di turno agli altri giocatori
        OtherOneTurnMessage packet;
        strncat(packet.player_name, current_player->username, USERNAME_LENGTH - 1);

        for (auto &player: players) {
            if (player.sockfd == current_player->sockfd)
                continue;

            _send(&player, packet);
        }

        // Invia il messaggio di turno al giocatore corrente
        _send_action(current_player, Action::YOUR_TURN);
    }

    void HangmanServer::accept() {
        struct sockaddr_in client_address{};
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = ::accept(sockfd, (struct sockaddr *) &client_address, &client_address_len);

        if (client_socket < 0) {
            return;
        }

        // Aggiunge il giocatore alla lista
        // Non possiamo ancora aggiungere il suo nome perché non è ancora stato inviato
        Player new_player;
        new_player.sockfd = client_socket;

        // Legge il nome del giocatore
        Client::JoinMessage packet;

        bool res = _read(&new_player, packet, packet.action, 1);
        if (res) {
            // Copia il nome del giocatore nella lista
            strncat(new_player.username, packet.username, USERNAME_LENGTH - 1);

            players.push_back(new_player);
            players_connected++;

            // Invia il messaggio di aggiornamento della lista dei giocatori
            for (auto &player: players) {
                _send_update_players(player);
            }

            // Invia la frase al nuovo player
            _send_update_short_phrase(new_player);

            // Invia i tentativi fatti fino ad ora al nuovo player
            _send_update_attempts(new_player);
        } else {
            closesocket(new_player.sockfd);
        }
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

        std::string phrase = all_phrases.at(index);
        strncat(short_phrase, phrase.c_str(), SHORTPHRASE_LENGTH - 1);

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

    int HangmanServer::_get_letter_from_player(Player *player, int timeout) {
        _send_action(player, Action::SEND_LETTER);

        Client::LetterMessage packet;

        if (!_read(player, packet, packet.action, timeout)) {
            return -3;
        }

        // Verifica che la lettere faccia parte dell'alafabeto
        if (isalpha(packet.letter) == 0) {
            _send_action(player, Action::LETTER_REJECTED);
            return -1;
        }

        packet.letter = toupper(packet.letter);

        // Verifica che la lettera non sia bloccata per i primi tre turni
        if (current_attempt < blocked_attempts) {
            for (size_t i = 0; i < strlen(start_blocked_letters); i++) {
                if (start_blocked_letters[i] == packet.letter) {
                    _send_action(player, Action::LETTER_REJECTED);
                    return -1;
                }
            }
        }

        // Controlla se la lettera è già stata usata
        for (auto letter: attempts) {
            if (letter == packet.letter) {
                _send_action(player, Action::LETTER_REJECTED);
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
            _send_action(player, Action::LETTER_ACCEPTED);
            return 1;
        } else {
            current_errors++;
            _send_action(player, Action::LETTER_REJECTED);
            return 0;
        }
    }

    int HangmanServer::_get_short_phrase_from_player(Player *player, int timeout) {
        _send_action(player, Action::SEND_SHORT_PHRASE);

        Client::ShortPhraseMessage packet;

        if (!_read(player, packet, packet.action, timeout)) {
            return -3;
        }

        // Controlla se la frase è corretta
        str_to_upper(packet.short_phrase);
        if (strncmp(packet.short_phrase, short_phrase, SHORTPHRASE_LENGTH) == 0) {
            _send_action(player, Action::SHORT_PHRASE_ACCEPTED);
            return 1;
        } else {
            _send_action(player, Action::SHORT_PHRASE_REJECTED);
            return 0;
        }
    }

    inline void HangmanServer::_send_update_attempts(Player &player) {
        // Invia il messaggio di aggiornamento delle lettere usate
        Server::UpdateAttemptsMessage packet;

        packet.max_errors = max_errors;
        packet.errors = current_errors;
        packet.attempts = current_attempt;
        strncat(packet.attempts_list, attempts.data(), packet.attempts);

        _send(&player, packet);
    }

    void HangmanServer::loop() {
        // Verifica che i giocatori connessi lo siano ancora
        _check_disconnected_players();

        // Seleziona il giocatore successivo
        _next_turn();

        // In rari casi current player potrebbe essere null seppur viene impostato in _next_turn
        // Questo è dovuto al fatto che il giocatore potrebbe essere disconnesso durante il turno
        if (current_player == nullptr) {
            return;
        }

        int res_letter = _get_letter_from_player(current_player);
        for (auto &player: players) {
            _send_update_short_phrase(player);
            _send_update_attempts(player);
        }

        // Controlla se il giocatore ha vinto indovinando l'ultima lettera
        if (_is_short_phrase_guessed()) {
            _broadcast_action(Action::WIN);
            usleep(5'000'000);  // sleep di 5 secondi
            new_round();

            return;
        }
        // Controlla se il giocatore ha perso perchè ha raggiunto il numero massimo di errori
        if (current_errors == max_errors) {
            _broadcast_action(Action::LOSE);
            usleep(5'000'000);
            new_round();

            return;
        }

        int res_phrase = -1;
        if (res_letter >= 0)
            res_phrase = _get_short_phrase_from_player(current_player);

        // Controlla se il giocatore ha vinto indovinando la frase
        if (res_phrase == 1) {
            _broadcast_action(Action::WIN);
            usleep(5'000'000);
            new_round();

            return;
        }
    }

    void HangmanServer::run(const bool verbose) {
        int prev_n_players = 0;

        try {
            start();
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }


        // Scrive a schermo l'indirizzo IP del server e la sua porta
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, str, INET_ADDRSTRLEN);
        std::cout << "Server address: " << str << "\n";
        std::cout << "Server port: " << ntohs(address.sin_port) << "\n\n";


        while (true) {
            try {
                // Controlla se ci sono nuove connessioni
                accept();
                // Fa in modo che l'uso della CPU non sia troppo alto
                usleep(100);

                if (verbose && players_connected > 0 && prev_n_players == 0)
                    std::cout << "Exited idle state" << "\n" << std::endl;

                if (players_connected > 0)
                    loop();

                if (verbose && players_connected > 0 && current_player != nullptr) {
                    std::cout << "Short phrase: " << short_phrase << "\n";
                    std::cout << "Current player: " << current_player->username << "\n";
                    std::cout << "Current attempt: " << current_attempt << "\n" << std::endl;
                }

                if (verbose && players_connected == 0 && prev_n_players > 0)
                    std::cout << "Entered idle state" << "\n" << std::endl;

                prev_n_players = players_connected;
            }
            catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}
