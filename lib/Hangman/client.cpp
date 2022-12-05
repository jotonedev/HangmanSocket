#include "client.h"

namespace Client {
    HangmanClient::HangmanClient(const char address[], const char port[]) {
        // Creazione del socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Errore nell'inizializzazione della socket");
        }

        // Creazione dell'indirizzo del server
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(atoi(port));
        server_address.sin_addr.s_addr = inet_addr(address);

        // Connessione al server
        if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            throw std::runtime_error("Errore nella connessione al server");
        }
    }

    void HangmanClient::join(const char username[]) {
        JoinMessage message;

        if (strlen(username) < USERNAME_LENGTH)
            strncpy(message.username, username, strlen(username));
        else
            strncpy(message.username, username, USERNAME_LENGTH);

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

        if (strlen(phrase) < SHORTPHRASE_LENGTH)
            strncpy(message.short_phrase, phrase, strlen(phrase));
        else
            strncpy(message.short_phrase, phrase, SHORTPHRASE_LENGTH);

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
            case Server::Action::UPDATE_USER:
                printPlayerList(&message.update_user_message);
                break;
            case Server::Action::UPDATE_SHORTPHRASE:
                printShortPhrase(&message.update_short_phrase_message);
                break;
            case Server::Action::UPDATE_ATTEMPTS:
                printAttempts(&message.update_attempts_message);
                break;
            case Server::Action::YOUR_TURN:
                yourTurn(&message.message);
                break;
            case Server::Action::OTHER_TURN:
                printOtherTurn(&message.other_one_turn_message);
                break;
            case Server::Action::WIN:
                printWin();
                break;
            case Server::Action::LOSE:
                printLose();
                break;

            default:
                break;
        }
    }

    void HangmanClient::run(bool verbose) {
        char username[USERNAME_LENGTH];
        cout << "Inserisci lo username: ";
        cin >> username;
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
        gotoxy(4, size.height / 2);
        clear_chars(123);
        std::cout << message->short_phrase;
    }

    void HangmanClient::printAttempts(Server::UpdateAttemptsMessage* message) {
        TerminalSize size = get_terminal_size();
        
        gotoxy(4, 2);
        clear_chars(60);
        for (int i = 0; i < message->attempts; i++) {
            char atp = message->attempts_list[i];
            if (atp != '\0')
                std::cout << atp << " ";
        }

        gotoxy(4, 3);
        clear_chars(30);
        std::cout << "Errori fatti fin'ora" << message->errors << "/" << message->max_errors;
        
    }

    void HangmanClient::printPlayerList(Server::UpdateUserMessage* message) {
        // Scrive a metà schermo sulla destra con un certo margine la lista dei giocatori
        TerminalSize size = get_terminal_size();
        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8);

        for (int i = 0; i < message->user_count; i++) {
            gotoxy(x, y+i);
            clear_chars(USERNAME_LENGTH);
            std::cout << message->usernames[i];
        }
    }

    void HangmanClient::printOtherTurn(Server::OtherOneTurnMessage* message) {
        TerminalSize size = get_terminal_size();

        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8)+3;

        clear_chars(USERNAME_LENGTH+14);
        std::cout << message->player_name << " sta giocando";
    }

    void HangmanClient::yourTurn(Server::Message* message) {
        TerminalSize size = get_terminal_size();

        int x = size.width-((USERNAME_LENGTH-4)%(size.width/2));
        int y = (size.height / 8)+3;

        clear_chars(USERNAME_LENGTH+14);
        std::cout << "E' il tuo turno";

        if (!_get_letter())
            return;
    }

    bool HangmanClient::_get_letter() {
        TerminalSize size = get_terminal_size();

        int x = size.width-4;
        int y = size.height-4;

        std::cout << "+";
        char letter;
        // Aspetta che l'utente inserisca una lettera per 5 secondi
        
        std::cin >> letter;

        LetterMessage letterMessage;
        letterMessage.letter = letter;

        return _send(letterMessage);
    }
}