#include <iostream>
#include <Hangman/client.h>


int main(int argc, char *argv[]) {
    Client::HangmanClient client("127.0.0.1", "9090");
    client.run(true);
}
