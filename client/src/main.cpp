#include <Hangman/client.h>


int main(int argc, char *argv[]) {
    Client::HangmanClient* client;
    // Chiede l'immissione dell'indirizzo ip del server, di default è 127.0.0.1
    if (argc == 2) {
        client = new Client::HangmanClient(argv[2], "9090");
    } else if (argc > 2) {
        client = new Client::HangmanClient(argv[2], argv[3]);
    } else {
        std::string ip;
        std::cout << "Inserisci l'indirizzo ip del server: ";
        std::cin >> ip;
        client = new Client::HangmanClient(ip, "9090");
    }

    client->run(true);
}
