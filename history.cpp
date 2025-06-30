#include "history.h"
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <string>

const std::string HISTORY_FILE = std::string(getenv("HOME")) + "/.goonsh_history";

void load_history_file() {
    read_history(HISTORY_FILE.c_str());
}

void save_history_file() {
    write_history(HISTORY_FILE.c_str());
}
