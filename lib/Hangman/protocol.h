#ifndef hangman_protocol_h
#define hangman_protocol_h


#include <sys/socket.h>


// Il protocollo per il gioco dell'impiccato si basa su sistema di azione (Action) e risposta (Response)
namespace Client {
    // Azioni che il client può inviare al server
    enum Action {
        // Azione di ingresso nella partita
        JOIN_GAME,
        // Azione di richiesta di una nuova lettera
        NEW_LETTER,
        // Azione di richiesta di una nuova parola o frase
        NEW_WORD,
        // Azione di richiesta di uscita dal gioco
        QUIT
    };

    // Risposte che il server può inviare al client
    enum Response {
        // Risposta di conferma di ingresso nella partita
        GAME_JOINED,
        // Risposta di conferma di una nuova lettera
        LETTER_ACCEPTED,
        // Risposta di errore di una nuova lettera
        LETTER_REJECTED,
        // Risposta di conferma di una nuova parola o frase
        WORD_ACCEPTED,
        // Risposta di errore di una nuova parola o frase
        WORD_REJECTED,
        
        // Risposta di conferma di uscita dal gioco
        GAME_QUIT,
        // Risposta di errore
        ERROR
    };

    struct Message {
        // Azione da inviare
        Action action;

        // Messaggio da inviare
        char message[256];
    } typedef Message;


    struct LetterMessage {
        Action action = NEW_LETTER;

        // Nuova lettera
        char letter;
        // Bytes in eccesso
        uint8_t excess_data[255];
    } typedef Message;
};


namespace Server {
    // Azioni che il server può inviare al client
    // Per nessuna di queste azioni il server si aspetta una risposta dal client
    enum Action {
        // Segnalazione del timeout per l'invio della risposta
        TIMEOUT,
        // Segnalazione di un update dello stato di gioco
        UPDATE_WORD,
        // Segnalazione dell'aggiornamento della lista dei giocatori
        UPDATE_USER,
        // Segnalazione di chiusura del server per un errore
        SERVER_ERROR,
        // Segnalazione di vittoria
        WIN,
        // Segnalazione di sconfitta
        LOSE,
        // Segnalazione del turno del giocatore
        YOUR_TURN,
        // Segnalazione di un turno di un altro giocatore
        OTHER_TURN,
    };
    
    struct Message {
        Action action;

        // Dati aggiuntivi
        uint8_t data[256];
    } typedef Message;

    struct UpdateUserMessage {
        Action action = UPDATE_USER;
        
        uint8_t user_count;
        char usernames[3][64];

        // 63 bytes di dati in più per evitare che il client legga dati non validi
        // Questo perchè tutti i messaggi hanno una lunghezza standard di 256 bytes + 1 byte per l'azione
        uint8_t excess_data[63]; 
    } typedef UpdateUserMessage;

    struct UpdateWordMessage {
        Action action = UPDATE_WORD;

        // Numero di errori fatti fino in quel momento
        uint8_t errors;
        // Parola o frase da indovinare
        char shortphrase [255];
    } typedef UpdateWordMessage;

    struct OtherOneTurn {
        Action action = OTHER_TURN;
        
        // Nome del giocatore che deve svolgere il turno
        char player_name[64];
        // Byte in eccesso siccome la lunghezza massima del nome di un player è 64 bytes
        uint8_t excess_data[192]; 
    }
};


#endif