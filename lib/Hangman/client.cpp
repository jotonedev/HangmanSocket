#include "client.h"
#include "terminal_utils.h"

namespace Client {
    HangmanClient::HangmanClient(const char address[], const char port[]) {
        // Creazione del socket
#ifdef _WIN32
        WSADATA wsa_data;
        WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }

        // Creazione dell'indirizzo del server
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(strtol(port, nullptr, 10));
        server_address.sin_addr.s_addr = inet_addr(address);
    }

    HangmanClient::~HangmanClient() {
        // Chiusura della sockfd
        shutdown(sockfd, SHUT_RDWR);

#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        ::close(sockfd);
#endif
    }

    void HangmanClient::join(const char username[]) {
        // Connessione al server
        if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            throw std::runtime_error("Errore nella connessione al server");
        }

        // Invio username
        JoinMessage message;
        strncat(message.username, username, USERNAME_LENGTH-1);

        _send(message);
    }

    void HangmanClient::close() {
        shutdown(sockfd, SHUT_RDWR);
    }

    void HangmanClient::sendLetter(char letter) {
        LetterMessage message;
        message.letter = letter;

        _send(message);
    }

    void HangmanClient::sendShortPhrase(const char phrase[]) {
        ShortPhraseMessage message;
        strncat(message.short_phrase, phrase, SHORTPHRASE_LENGTH-1);

        _send(message);
    }

    template<typename TypeMessage>
    bool HangmanClient::_send(TypeMessage &message) {
        return send(sockfd, (char *) (&message), MessageSize, 0);
    }

    template<typename TypeMessage>
    bool HangmanClient::_receive(TypeMessage &message) {
        return recv(sockfd, (char *) (&message), MessageSize, MSG_WAITALL);
    }

    void HangmanClient::waitAction(){
        // Aspetta che ci siano dati disponibili da leggere sul socket della dimensione di un messaggio
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        select(sockfd + 1, &set, NULL, NULL, NULL);
    }

    void HangmanClient::loop() {
        // Aspetta per un messaggio dal server
        waitAction();

        // Riceve il messaggio dal server
        Server::Message raw_msg;
        _receive(raw_msg);

        // Converte il messaggio in un messaggio di tipo union
        ServerMessageUnion message = {raw_msg};

        // Esegue l'azione corrispondente al messaggio ricevuto
        switch(raw_msg.action) {
            case Server::Action::UPDATE_USER: {
                printPlayerList(&message.update_user_message);
                break;
            }
            case Server::Action::UPDATE_SHORTPHRASE: {
                printShortPhrase(&message.update_short_phrase_message);
                break;
            }
            case Server::Action::UPDATE_ATTEMPTS: {
                printAttempts(&message.update_attempts_message);
                break;
            }
            case Server::Action::YOUR_TURN: {
                printYourTurn();
                break;
            }
            case Server::Action::OTHER_TURN: {
                printOtherTurn(&message.other_one_turn_message);
                break;
            }
            case Server::Action::WIN: {
                printWin();
                break;
            }
            case Server::Action::LOSE: {
                printLose();
                break;
            }
            case Server::Action::NEW_GAME: {
                clear_screen();
                break;
            }
            case Server::Action::SEND_LETTER: {
                _get_letter();
                break;
            }
            case Server::Action::SEND_SHORT_PHRASE: {
                _get_short_phrase();
                break;
            }

            default: {
                break;
            }
        }
    }

    void HangmanClient::run(bool verbose) {
        char username[USERNAME_LENGTH];
        cout << "Inserisci lo username: ";
        cin >> username;
        cin.clear();
        cin.ignore(10000, '\n');

        try {
            join(username);
        } catch (const std::exception &e) {
            if (verbose)
                std::cerr << e.what() << endl;
            exit(EXIT_FAILURE);
        }

        clear_screen();

        // Loop principale del client
        while(true) {
            try {
                loop();
            } catch (std::exception &e) {
                if (verbose)
                    std::cerr << e.what() << std::endl;
                break;
            }
        }
    }

    void HangmanClient::printShortPhrase(Server::UpdateShortPhraseMessage* message) {
        // Scrive a metà schermo sulla sinistra con un certo margine la shortphrase
        TerminalSize size = get_terminal_size();
        int x = 2;
        int y = size.height / 2;

        clear_chars(SHORTPHRASE_LENGTH, x, y);
        std::cout << message->short_phrase;
    }

    void HangmanClient::printAttempts(Server::UpdateAttemptsMessage* message) {
        clear_chars(60, 2, 2);

        for (int i = 0; i < message->attempts; i++) {
            char atp = message->attempts_list[i];
            if (atp != '\0')
                std::cout << atp << " ";
        }

        clear_chars(30, 2, 3);
        std::cout << "Errori fatti fin'ora: " << (int)message->errors << "/" << (int)message->max_errors;
    }

    void HangmanClient::printPlayerList(Server::UpdateUserMessage* message) {
        // Scrive a metà schermo sulla destra con un certo margine la lista dei giocatori
        TerminalSize size = get_terminal_size();
        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8);

        clear_chars(USERNAME_LENGTH, x, y-2);
        std::cout << "Players";

        for (int i = 0; i < message->user_count; i++) {
            clear_chars(USERNAME_LENGTH, x, y+i);
            std::cout << message->usernames[i];
        }
    }

    void HangmanClient::printOtherTurn(Server::OtherOneTurnMessage* message) {
        TerminalSize size = get_terminal_size();

        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8)+4;

        clear_chars(USERNAME_LENGTH+14, x, y);
        std::cout << message->player_name << " sta giocando";

        y = size.height-2;
        clear_chars(SHORTPHRASE_LENGTH, 2, y);
    }

    void HangmanClient::printYourTurn() {
        TerminalSize size = get_terminal_size();

        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8)+3;

        clear_chars(USERNAME_LENGTH+14, x, y);
        std::cout << "E' il tuo turno";
    }

    bool HangmanClient::_get_letter() {
        TerminalSize size = get_terminal_size();

        int x = 2;
        int y = size.height-2;

        clear_chars(SHORTPHRASE_LENGTH, x, y);

        std::cout << "+" << std::flush;
        char letter;

        // Aspetta che l'utente inserisca una lettera per 5 secondi
#ifndef _WIN32
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

        if (rv == -1) {
            clear_chars(SHORTPHRASE_LENGTH, x, y);
            std::cerr << "Errore nella lettura della lettera" << std::endl;
            return false;
        } else if (rv == 0) {
            clear_chars(SHORTPHRASE_LENGTH, x, y);
            std::cout << "Tempo scaduto" << std::endl;
            return false;
        } else {
            std::cin >> letter;
            clear_chars(SHORTPHRASE_LENGTH, x, y);
        }
#else
        // Per implementare il timeout correttamente su windows serve l'uso dei thread
        std::cin >> letter;
        clear_chars(SHORTPHRASE_LENGTH, x, y);
#endif
        // Pulisce il buffer di input
        cin.clear();
        cin.ignore(10000, '\n');

        // Invia la lettera al server
        LetterMessage letterMessage;
        letterMessage.letter = letter;

        bool res = _send(letterMessage);
        if (!res)
            return false;

        // Aspetta la risposta dal server
        Server::Message response;
        _receive(response);

        if (response.action == Server::LETTER_ACCEPTED)
            return true;
        else
            return false;
    }

    bool HangmanClient::_get_short_phrase() {
        TerminalSize size = get_terminal_size();

        int x = 2;
        int y = size.height-2;

        clear_chars(SHORTPHRASE_LENGTH, x, y);

        std::cout << "> " << std::flush;
        string phrase;

#ifndef _WIN32
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

        if (rv == -1) {
            clear_chars(SHORTPHRASE_LENGTH, x, y);
            std::cerr << "Errore nella lettura della lettera" << std::endl;
            return false;
        } else if (rv == 0) {
            clear_chars(SHORTPHRASE_LENGTH, x, y);
            std::cout << "Tempo scaduto" << std::endl;
            return false;
        } else {
            std::getline(std::cin, phrase);
            clear_chars(SHORTPHRASE_LENGTH, x, y);
        }
#else
        // Per implementare il timeout correttamente su windows serve l'uso dei thread
        do {
            std::getline(std::cin, phrase);
        } while (phrase.length() == 0);
#endif
        ShortPhraseMessage shortPhraseMessage;
        strncat(shortPhraseMessage.short_phrase, phrase.c_str(), SHORTPHRASE_LENGTH-1);

        bool res = _send(shortPhraseMessage);

        if (!res)
            return false;

        Server::Message response;
        _receive(response);

        if (response.action == Server::LETTER_ACCEPTED)
            return true;
        else
            return false;
    }

    void HangmanClient::printWin() {
        TerminalSize size = get_terminal_size();

        int y = size.height-2;

        gotoxy(2, y);
        std::cout << "You Win!" << std::flush;
    }

    void HangmanClient::printLose() {
        TerminalSize size = get_terminal_size();

        int y = size.height-2;

        gotoxy(2, y);
        std::cout << "You Lose." << std::flush;
    }

    bool HangmanClient::_wait_for_input(int timeout) {
#ifndef _WIN32
        struct pollfd poller;
        poller.fd = STDIN_FILENO;
        poller.events = POLLIN;
        poller.revents = 0;

        int rc = poll(&poller, 1, timeout);

        if (rc <= 0)
            return false;
        else
            return true;
#else
        for (int i = 0; i < timeout * 10; i++) {
            char c = std::cin.peek();

            if (c != EOF)
                return true;

            // Dorme per 100 millisecond
            usleep(100'000);
        }
        return false;
#endif
    }
}