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
        strncat(message.username, username, USERNAME_LENGTH - 1);
        _send(message);
    }

    void HangmanClient::loop() {
        // Aspetta per un messaggio dal server
        _waitAction();

        // Riceve il messaggio dal server
        Server::Message raw_msg;
        _receive(raw_msg);

        // Converte il messaggio in un messaggio di tipo union
        ServerMessageUnion message = {raw_msg};

        if (game_over) {
            clear_screen();
            game_over = false;
        }

        // Esegue l'azione corrispondente al messaggio ricevuto
        switch (raw_msg.action) {
            case Server::Action::UPDATE_USER: {
                players_count = message.update_user_message.user_count;
                _printPlayerList(&message.update_user_message);
                break;
            }
            case Server::Action::UPDATE_SHORTPHRASE: {
                _printShortPhrase(&message.update_short_phrase_message);
                break;
            }
            case Server::Action::UPDATE_ATTEMPTS: {
                _printAttempts(&message.update_attempts_message);
                break;
            }
            case Server::Action::YOUR_TURN: {
                _printYourTurn();
                break;
            }
            case Server::Action::OTHER_TURN: {
                _printOtherTurn(&message.other_one_turn_message);
                break;
            }
            case Server::Action::WIN: {
                _printWin();
                break;
            }
            case Server::Action::LOSE: {
                _printLose();
                break;
            }
            case Server::Action::NEW_GAME: {
                clear_screen();
                break;
            }
            case Server::Action::SEND_LETTER: {
                _getLetter();
                break;
            }
            case Server::Action::SEND_SHORT_PHRASE: {
                _getShortPhrase();
                break;
            }
            case Server::Action::HEARTBEAT: {
                Message heartbeat;
                heartbeat.action = Action::HEARTBEAT;
                _send(heartbeat);
                break;
            }

            default: {
                break;
            }
        }
    }

    void HangmanClient::run(bool verbose) {
        char username[USERNAME_LENGTH];
        std::cout << "Inserisci lo username: ";
        std::cin >> username;
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        clear_screen();

        try {
            join(username);
        } catch (const std::exception &e) {
            if (verbose)
                std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }

        clear_screen();

        // Loop principale del client
        while (true) {
            try {
                loop();
            } catch (std::exception &e) {
                if (verbose)
                    std::cerr << e.what() << std::endl;
                break;
            }
        }
    }

    void HangmanClient::close() {
        shutdown(sockfd, SHUT_RDWR);
    }

    template<typename TypeMessage>
    bool HangmanClient::_send(TypeMessage &message) {
        return send(sockfd, (char *) (&message), MessageSize, 0);
    }

    template<typename TypeMessage>
    bool HangmanClient::_receive(TypeMessage &message) {
        return recv(sockfd, (char *) (&message), MessageSize, MSG_WAITALL);
    }

    bool HangmanClient::_getLetter() {
        TerminalSize size = get_terminal_size();

        int x = 2;
        int y = size.height - 2;

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
            if (players_count > 1)
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
        std::cin.clear();
        std::cin.ignore(10000, '\n');

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

    bool HangmanClient::_getShortPhrase() {
        TerminalSize size = get_terminal_size();

        int x = 2;
        int y = size.height - 2;

        clear_chars(SHORTPHRASE_LENGTH, x, y);

        std::cout << "> " << std::flush;
        std::string phrase;

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
        strncat(shortPhraseMessage.short_phrase, phrase.c_str(), SHORTPHRASE_LENGTH - 1);

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

    void HangmanClient::_waitAction() {
        // Aspetta che ci siano dati disponibili da leggere sul socket della dimensione di un messaggio
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        select(sockfd + 1, &set, NULL, NULL, NULL);
    }

    void HangmanClient::_printYourTurn() {
        TerminalSize size = get_terminal_size();

        int x = size.width - ((USERNAME_LENGTH - 4) % (size.width / 2));
        int y = (size.height / 8) + 3;

        clear_chars(USERNAME_LENGTH + 14, x, y);
        std::cout << "E' il tuo turno";
    }

    void HangmanClient::_printOtherTurn(Server::OtherOneTurnMessage *message) {
        TerminalSize size = get_terminal_size();

        int x = size.width - ((USERNAME_LENGTH - 4) % (size.width / 2));
        int y = (size.height / 8) + 3;

        // Pulisce il buffer perchè quando compilato con Cygwin questa funzione non funziona
        setbuf(stdout, NULL);

        clear_chars(USERNAME_LENGTH + 14, x, y);
        std::cout << message->player_name << " sta giocando";

        y = size.height - 2;
        clear_chars(SHORTPHRASE_LENGTH, 2, y);
    }

    void HangmanClient::_printPlayerList(Server::UpdateUserMessage *message) {
        // Scrive a metà schermo sulla destra con un certo margine la lista dei giocatori
        TerminalSize size = get_terminal_size();
        int x = size.width - ((USERNAME_LENGTH - 4) % (size.width / 2));
        int y = (size.height / 8);

        clear_chars(USERNAME_LENGTH, x, y - 2);
        std::cout << "Players";

        for (int i = 0; i < 3; i++) {
            clear_chars(USERNAME_LENGTH, x, y + i);
        }

        for (int i = 0; i < message->user_count; i++) {
            gotoxy(x, y + i);
            std::cout << message->usernames[i];
        }
    }

    void HangmanClient::_printShortPhrase(Server::UpdateShortPhraseMessage *message) {
        // Scrive a metà schermo sulla sinistra con un certo margine la shortphrase
        TerminalSize size = get_terminal_size();
        int x = 2;
        int y = size.height / 2;

        clear_chars(SHORTPHRASE_LENGTH, x, y);
        std::cout << message->short_phrase;
    }

    void HangmanClient::_printAttempts(Server::UpdateAttemptsMessage *message) {
        clear_chars(60, 2, 2);

        for (int i = 0; i < message->attempts; i++) {
            char atp = message->attempts_list[i];
            if (atp != '\0')
                std::cout << atp << " ";
        }

        clear_chars(30, 2, 3);
        std::cout << "Errori fatti fin'ora: " << (int) message->errors << "/" << (int) message->max_errors;

        _printHangman(message->errors);
    }

    void HangmanClient::_printHangman(int mistakes){
        TerminalSize size = get_terminal_size();
        int x = (size.width / 2)+3;
        int y = (size.height / 2)-2;

        switch (mistakes){
            case 1: {
                gotoxy(x, y);
                std::cout << "----------";
                
                for(int i=1; i<9; i++) {
                    gotoxy(x+1, y-i);
                    std::cout << "|";
                }
                break;
            }
            case 2: {
                gotoxy(x, y-9);
                std::cout << "------";
                break;
            }
            case 3: {
                gotoxy(x+6,y-8);
                std::cout << "|";
                break;
            }
            case 4: {
                gotoxy(x+6,y-7);
                std::cout << "O";
                break;
            }
            case 5: {
                gotoxy(x+5,y-6);
                std::cout << "-+-";
                break;
            }
            case 6: {
                gotoxy(x+4,y-5);
                std::cout << "/";
                break;
            }
            case 7: {
                gotoxy(x+8,y-5);
                std::cout << "\\";
                break;
            }
            case 8: {
                gotoxy(x+6,y-5);
                std::cout << "|";
                break;
            }
            case 9: {
                gotoxy(x+6,y-4);
                std::cout << "|";
                for(int i=3;i>1;i--){
                    gotoxy(x+5,y-i);
                    std::cout << "|";
                }
                break;
            }
            case 10: {
                for(int i=3;i>1;i--){
                    gotoxy(x+7,y-i);
                    std::cout << "|";
                }
                break;
            }  
            default:
            break;
        }
    }

    void HangmanClient::_printWin() {
        TerminalSize size = get_terminal_size();

        int y = size.height - 2;
        game_over = true;
        gotoxy(2, y);
        std::cout << "You Win!" << std::flush;
    }

    void HangmanClient::_printLose() {
        TerminalSize size = get_terminal_size();

        int y = size.height - 2;
        game_over = true;
        gotoxy(2, y);
        std::cout << "You Lose." << std::flush;
    }
}