#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <fcntl.h>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <unistd.h>
#endif

#include "protocol.h"
#include "string_utils.h"


#define MAX_CLIENTS 3


using std::string;


namespace Server {
    /**
     * Rappresenta il giocatore
     *
     * Contiene il nome del giocatore e il descrittore del suo socket
     *
     * @author John Toniutti
     */
    struct Player {
        /// Socket del client
        int sockfd;
        /// Nome del client
        char username[USERNAME_LENGTH]{};
    } typedef Player;


    /**
     * Questa classe rappresenta l'intero server del gioco dell'impiccato
     *
     * Provvede a dare una funzione per eseguire il gioco direttamente e a dare le funzioni per crearsi il proprio loop
     * di gioco nel caso fosse necessario.
     * @note Questa classe non è thread-safe
     * @warning Se un client non rispetta il protocollo, questo viene disconnesso
     *
     * @author John Toniutti
     */
    class HangmanServer {
    private:
        /// Descrittore del socket del server
        int sockfd;

        /// Contiene l'indirizzo IP e la porta del server
        struct sockaddr_in address{};

        /// Rappresenta il numero di errori massimo che i giocatori possono commettere
        unsigned int max_errors{};
        /// Rappresenta il numero di errori commessi dai giocatori
        unsigned int current_errors{};
        /// Rappresenta i tentativi fatti fin'ora
        std::vector<char> attempts;
        /// Rappresenta quanti tentativi sono stati fatti
        unsigned int current_attempt{};
        /// Rappresenta la parola o frase da indovinare
        char short_phrase[SHORTPHRASE_LENGTH]{};
        /// Rappresenta la parola o frase da indovinare con i caratteri non ancora indovinati sostituiti da _
        char short_phrase_masked[SHORTPHRASE_LENGTH]{};
        /// Rappresenta le lettere che non si possono indovinare all'inizio
        const char *start_blocked_letters{};
        /// Rappresenta il numero di tentativi che devono essere fatti prima di poter usare le lettere bloccate
        unsigned int blocked_attempts{};
        /// Lista dei client connessi
        std::vector<Player> players;
        /// Rappresenta il numero di giocatori connessi
        unsigned int players_connected{};
        /// Rappresenta il giocatore corrente
        Player *current_player{};
        /// Contiene tutte le possibili frasi da indovinare
        std::vector<string> all_phrases;

        /**
         * Permette di inviare un messaggio ad un certo giocatore
         * @tparam TypeMessage Un tipo di messaggio di 128 bytes
         * @param player Il giocatore a cui inviare il messaggio
         * @param message Il messaggio da inviare
         * @return true se l'invio è andato a buon fine, false altrimenti
         * @retval true L'invio è andato a buon fine
         * @retval false L'invio non è andato a buon fine
         *
         * @note Se l'invio fallisce, il giocatore viene disconnesso
         */
        template<typename TypeMessage>
        bool _send(Player *player, TypeMessage &message);

        /**
         * Permette di leggere un messaggio da un certo giocatore
         * @tparam TypeMessage Un tipo di messaggio di 128 bytes
         * @tparam TypeAction L'azione che ci si aspetta di ricevere
         *
         * @param player Il giocatore da cui aspettarsi il messaggio
         * @param message Dove salvare il messaggio
         * @param timeout Il tempo massimo di attesa per il messaggio (in secondi)
         *
         * @return Se la lettura è andata a buon fine
         */
        template<class TypeMessage>
        bool
        _read(Player *player, TypeMessage &message, Client::Action action = Client::Action::GENERIC, int timeout = 5);

    protected:
        /**
         * Carica le frasi da un file
         *
         * @param filename il nome del file da cui caricare le frasi
         * @throws std::runtime_error se il file non esiste
         */
        void _load_short_phrases(const string &filename = "data/data.txt");

        /**
         * Permette di generare una nuova frase da indovinare
         */
        void _generate_short_phrase();

        /**
         * Permette di passare il turno al giocatore successivo
         */
        void _next_turn();

        /**
         * Permette di inviare a tutti i giocatori un'azione
         * @param action L'azione da inviare
         */
        inline void _broadcast_action(Server::Action action);

        /**
         * Permette di inviare a un giocatore un aggiornamento sullo stato del gioco
         *
         * @param player Il giocatore a cui inviare l'aggiornamento
         */
        inline void _send_update_short_phrase(Player &player);

        /**
         * Permette di inviare a un giocatore un aggiornamento sul numero di errori commessi
         *
         * @param player Il giocatore a cui inviare l'aggiornamento
         */
        inline void _send_update_attempts(Player &player);

        /**
         * Permette di inviare a un giocatore un aggiornamento sulla lista dei giocatori
         *
         * @param player Il giocatore a cui inviare l'aggiornamento
         */
        inline void _send_update_players(Player &player);

        /**
         * Permette di inviare un'azione ad un certo giocatore
         * @param player Il giocatore a cui inviare l'azione
         * @param action L'azione da inviare
         * @return true se l'invio è andato a buon fine, false altrimenti
         * @retval true L'invio è andato a buon fine
         * @retval false L'invio non è andato a buon fine
         */
        inline bool _send_action(Player *player, Server::Action action);

        /**
         * Permette di eliminare un giocatore dalla lista dei giocatori
         * @param player Il Player da eliminare
         */
        void _remove_player(Player *player);

        /**
         * Permette di verificare se la parola o frase è stata indovinata
         * @return se la parola o frase è stata indovinata
         * @retval true se la parola o la frase è stata indovinata
         * @retval false se la parola o la frase non è stata indovinata
         */
        inline bool _is_short_phrase_guessed();

        /**
         * Permette di ricevere da un player un tentativo contenente una lettera
         * @param player Il player che deve fare il tentativo
         * @param timeout Il tempo massimo da aspettare in secondi
         * @return Un numero che rappresenta il risultato della funzione
         * @retval 1 Se il tentativo è andato a buon fine e ha indovinato
         * @retval 0 Se il tentativo è andato a buon fine e non ha indovinato
         * @retval -1 Se la lettera è bloccata o se è già stata usata
         * @retval -2 Se ha mandato un pacchetto sbagliato (in questo caso il client viene disconnesso)
         */
        int _get_letter_from_player(Player *player, int timeout = 5);

        /**
         * Permette di ricevere da un player un tentativo sulla frase da indovinare
         * @param player Il player che deve fare il tentativo
         * @param timeout Il tempo massimo da aspettare in secondi
         * @return Un numero che rappresenta il risultato della funzione
         * @retval 1 Se il tentativo è andato a buon fine e ha indovinato
         * @retval 0 Se il tentativo è andato a buon fine e non ha indovinato
         * @retval -2 Se ha mandato un pacchetto sbagliato (in questo caso il client viene disconnesso)
         */
        int _get_short_phrase_from_player(Player *player, int timeout = 10);

        /**
         * Permette di verificare se uno dei giocatori connessi si è disconnesso
         */
        void _check_disconnected_players();

        /**
         * Permette di verificare se ci sono nuovi client che vogliono connettersi e di accettarli
         */
        void accept();

        /**
         * Loop del server
         * @brief Si occupa di gestire le connessioni e le richieste dei client, quindi di eseguire il gioco
         * @note Deve trovarsi all'interno di un while loop
         */
        void loop();

        /**
         * Permette di avviare un nuovo round
         */
        void new_round();

    public:
        /**
         * Costruttore della classe HangmanServer
         * @param _ip L'indirizzo IP del server (se lasciato come default usa tutte le interfacce disponibili)
         * @param _port La porta del server
         * @throws std::runtime_error Se non è possibile creare il socket
         */
        HangmanServer(const string &_ip = "0.0.0.0", uint16_t _port = 9090);

        /**
         * Distruttore della classe HangmanServer
         * @brief Chiude il socket del server e con i giocatori connessi
         */
        ~HangmanServer();

        /**
         * Avvia il server
         * @param _max_errors Il numero massimo di errori prima che la partita sia persa
         * @param _start_blocked_letters Le lettere che non si possono indovinare all'inizio
         * @param _blocked_attempts Il numero di tentativi che devono essere fatti prima di poter usare le lettere bloccate
         * @param _filename Il nome del file da cui caricare le frasi
         * @throws std::runtime_error Se il server non è stato avviato
         */
        void start(uint8_t _max_errors = 10, const string &_start_blocked_letters = "AEIOU",
                   uint8_t _blocked_attempts = 3, const string &_filename = "data/data.txt");

        /**
         * Esegue tutte le funzioni del server
         * @brief Permette di lasciare la gestione del server alla classe stessa, che si occuperà di avviare il server e gestire il loop di gioco
         * @param verbose Se deve stampare un resoconto dello stato del server ad ogni loop
        */
        void run(const bool verbose = true);
    };

}


#endif