#ifndef hangman_client_h
#define hangman_client_h

#include <sys/types.h>
#include <sys/socket.h>
#include <iterator>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cctype>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"


namespace Client {
    class HangmanClient {
        private:
            // Descrittore del socket del server
            int sockfd;

            char attempts[26];
            int attempts_count;

            char short_phrase[SHORTPHRASE_LENGTH];
            char blocked_letters[6] = "AEIOU";

            template<typename TypeMessage> void _send(TypeMessage &message);
            template<typename TypeMessage> void _receive(TypeMessage &message);
            
        public:
            HangmanClient(const char address[], const char port[]);

            ~HangmanClient();

            void join(const char username[]);

            void loop();

            void run();

            void close();

            void sendLetter(char letter);

            void sendShortPhrase(const char phrase[]);
        
            char* getAttempts() { return attempts; };

            char* getShortPhrase() { return short_phrase; };
    };
}


#endif