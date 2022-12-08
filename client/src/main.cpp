#include <Hangman/client.h>


int main() {
    Client::HangmanClient client("127.0.0.1", "9090");

    client.run(true);
}
