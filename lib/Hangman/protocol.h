#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>

#define bzero(b, len) (memset((b), '\0', (len)), (void) 0)
#define bcopy(b1, b2, len) (memmove((b2), (b1), (len)), (void) 0)
#define inet_aton(addr, in) ((*(in)).s_addr = inet_addr(addr))

#define SHUT_RDWR SD_BOTH

#define socklen_t int
#define ssize_t int
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <locale>
#include <stdint.h>
#include <cstring>
#include <iostream>
#include <sys/types.h>


using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;


#define SHORTPHRASE_LENGTH 123
#define USERNAME_LENGTH 32
#define GENERIC_ACTION 0xFF


// Il protocollo per il gioco dell'impiccato si basa su sistema di azione (Action) e risposta (Response)
namespace Client {
    // Le azioni che il client può inviare al server o le risposte che il server può inviare al client
    enum Action {
        // Azioni che il client può inviare al server
        // Azione d'ingresso nella partita
        JOIN_GAME,
        // Azione d'invio di una nuova lettera
        LETTER,
        // Azione d'invio della frase
        SHORT_PHRASE,

        // Risposte che il server può inviare al client
        // Risposta di conferma della lettera
        LETTER_ACCEPTED,
        // Risposta di errore della lettera
        LETTER_REJECTED,
        // Risposta di conferma della frase
        SHORT_PHRASE_ACCEPTED,
        // Risposta di errore della frase
        SHORT_PHRASE_REJECTED,

        // Valore da sostituire
        GENERIC = GENERIC_ACTION,
    };

    // Struttura che rappresenta un messaggio base
    struct Message {
        // Azione da inviare
        Action action = GENERIC;

        // Messaggio da inviare
        char data[124];
    } typedef Message;

    // Struttura che rappresenta un messaggio di ingresso nella partita
    struct JoinMessage {
        // Azione da inviare
        Action action = JOIN_GAME;

        // Nome del giocatore
        char username[USERNAME_LENGTH];

        uint8_t pad[124 - USERNAME_LENGTH];
    } typedef JoinMessage;

    // Struttura che rappresenta un messaggio di invio di una nuova lettera
    struct LetterMessage {
        Action action = LETTER;

        // Nuova lettera
        char letter{};

        uint8_t pad[123]{};
    } typedef LetterMessage;

    // Struttura che rappresenta un messaggio di invio della frase
    struct ShortPhraseMessage {
        Action action = SHORT_PHRASE;

        // Frase proposta
        char short_phrase[SHORTPHRASE_LENGTH]{};
        // Bytes in eccesso
        uint8_t pad[1]{};
    } typedef ShortPhraseMessage;


    // Verifica che le struct siano di dimensione corretta
    static_assert(sizeof(Message) == sizeof(JoinMessage), "sizes must match");
    static_assert(sizeof(Message) == sizeof(LetterMessage), "sizes must match");
    static_assert(sizeof(Message) == sizeof(ShortPhraseMessage), "sizes must match");
}


namespace Server {
    // Azioni che il server può inviare al client
    // Per nessuna di queste azioni il server si aspetta una risposta dal client
    enum Action {
        // Segnalazione di un update dello stato di gioco
        UPDATE_SHORTPHRASE,
        // Segnalazione dell'aggiornamento della lista dei giocatori
        UPDATE_USER,
        // Segnalazione dell'aggiornamento della lista dei tentativi
        UPDATE_ATTEMPTS,
        // Segnalazione di vittoria
        WIN,
        // Segnalazione di sconfitta
        LOSE,
        // Segnalazione del turno del giocatore
        YOUR_TURN,
        // Segnalazione di un turno di un altro giocatore
        OTHER_TURN,

        // Valore da sostituire
        GENERIC = GENERIC_ACTION,
    };

    // Struttura che rappresenta un messaggio base
    struct Message {
        Action action = GENERIC;

        // Dati aggiuntivi
        uint8_t data[124];
    } typedef Message;

    // Struttura che rappresenta un messaggio di segnalazione di un update dello stato di gioco
    struct UpdateUserMessage {
        Action action = UPDATE_USER;

        uint8_t user_count{};
        char usernames[3][USERNAME_LENGTH]{};

        // Bytes in eccesso
        uint8_t pad[124 - 1 - 3 * USERNAME_LENGTH]{};
    } typedef UpdateUserMessage;

    // Struttura che rappresenta un messaggio di segnalazione dell'aggiornamento della lista dei giocatori
    struct UpdateShortPhraseMessage {
        Action action = UPDATE_SHORTPHRASE;

        // Numero di errori fatti fino in quel momento
        uint8_t errors{};
        // Parola o frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH]{};
    } typedef UpdateWordMessage;

    // Struttura che rappresenta un messaggio di segnalazione dell'aggiornamento della lista dei tentativi
    struct UpdateAttemptsMessage {
        Action action = UPDATE_ATTEMPTS;

        // Numero di tentativi rimasti
        uint8_t attempts{};
        // Numero degli errori fatti
        uint8_t errors{};
        // Numero degli errori massimi
        uint8_t max_errors{};
        // Lista dei tentativi fatti
        char attempts_list[26]{};

        // Bytes in eccesso
        uint8_t pad[124 - 1 - 1 - 1 - 26]{};
    } typedef UpdateAttemptsMessage;

    // Struttura che rappresenta un messaggio di cambio turno
    struct OtherOneTurnMessage {
        Action action = OTHER_TURN;

        // Nome del giocatore che deve svolgere il turno
        char player_name[USERNAME_LENGTH]{};

        // Byte in eccesso
        uint8_t pad[124 - USERNAME_LENGTH]{};
    } typedef OtherOneTurnMessage;


    // Verifica che le struct siano di dimensione corretta
    static_assert(sizeof(Message) == sizeof(UpdateUserMessage), "sizes must match");
    static_assert(sizeof(Message) == sizeof(UpdateWordMessage), "sizes must match");
    static_assert(sizeof(Message) == sizeof(OtherOneTurnMessage), "sizes must match");
    static_assert(sizeof(Message) == sizeof(UpdateAttemptsMessage), "sizes must match");
}

// Verifica che le struct siano di dimensione corretta
static_assert(sizeof(Client::Message) == sizeof(Server::Message), "sizes must match");

#define MessageSize sizeof(Client::Message)


#endif