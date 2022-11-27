#include <iostream>
#include <Hangman/server.h>


int main(int argc, char *argv[]) {
    Server::HangmanServer server;

    srand(time(nullptr)); // NOLINT(cert-msc51-cpp)

    server.run(true);
}
