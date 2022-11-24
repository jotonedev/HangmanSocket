from libcpp.string cimport string
from libcpp.vector cimport vector

SHORTPHRASE_LENGTH = 123
USERNAME_LENGTH = 32

cdef extern from "protocol.h" namespace "Server":
    ctypedef enum Action:
        UPDATE_SHORTPHRASE,
        UPDATE_USER,
        UPDATE_ATTEMPTS,
        WIN,
        LOSE,
        YOUR_TURN,
        OTHER_TURN,

    cdef cppclass Message:
        Action action
        uint8_t data[124]

    cdef cppclass UpdateUserMessage:
        Action action
        uint8_t user_count
        char usernames[3][USERNAME_LENGTH]
        uint8_t pad[124 - 1 - 3 * USERNAME_LENGTH]

    cdef cppclass UpdateShortPhraseMessage:
        Action action
        uint8_t errors
        char short_phrase[SHORTPHRASE_LENGTH]

    cdef cppclass UpdateAttemptsMessage:
        Action action
        uint8_t attempts
        uint8_t errors
        uint8_t max_errors
        char attempts_list[26]
        uint8_t pad[124 - 1 - 1 - 1 - 26]

    cdef cppclass OtherOneTurnMessage:
        Action action
        char player_name[USERNAME_LENGTH]
        uint8_t pad[124 - USERNAME_LENGTH]

cdef extern from "server.cpp":
    pass

cdef extern from "server.h" namespace "Server":
    cdef cppclass Player:
        int sockfd
        char username[USERNAME_LENGTH]

    cdef cppclass HangmanServer:
        HangmanServer(const string, uint16_t) except +
        void start(uint8_t, const string, uint8_t)
        void loop()
        void run(const bool)
        void new_round()

        vector[Player] getPlayers()
        Player *get_current_player()
        int get_players_count()
        bool is_full()
        bool is_empty()
        int get_current_errors()
        int get_current_attempt()
        const char *get_short_phrase()
        char get_last_attempt()
