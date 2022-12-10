#include <iostream>
#include <Hangman/server.h>


int main(int argc, char *argv[]) {
    Server::HangmanServer *server;

    std::cout << "Starting up server..." << std::endl;
    srand(time(nullptr)); // NOLINT(cert-msc51-cpp)

    if (argc == 3)
        server = new Server::HangmanServer(argv[1], strtol(argv[2], nullptr, 10));
    else if (argc == 2)
        server = new Server::HangmanServer(argv[1]);
    else
        server = new Server::HangmanServer();

    server->run(true);
}
