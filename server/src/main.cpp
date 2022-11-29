#include <iostream>
#include <Hangman/server.h>


int main(int argc, char *argv[]) {
    std::cout << "Starting up server..." << std::endl;
    srand(time(nullptr)); // NOLINT(cert-msc51-cpp)

    Server::HangmanServer server;
    server.run(true);
}
