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
    void HangmanClient::_send(TypeMessage &message) {
        send(sockfd, (char *) (&message), MessageSize, 0);
    }

    template<typename TypeMessage>
    void HangmanClient::_receive(TypeMessage &message) {
        recv(sockfd, (char *) (&message), MessageSize, MSG_WAITALL);
    }
}