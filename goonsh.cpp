#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <map>
#include <ctime>
#include <pwd.h>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <filesystem>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ioctl.h>
#include <iomanip>

std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

const std::string HISTORY_FILE = std::string(getenv("HOME")) + "/.goonsh_history";
void load_history_file() {
    read_history(HISTORY_FILE.c_str());
}
void save_history_file() {
    write_history(HISTORY_FILE.c_str());
}

const std::string RC_FILE = std::string(getenv("HOME")) + "/.goonshrc";
void load_config(std::map<std::string, std::string>& aliases, std::string& prompt, std::vector<std::string>& rc_commands) {
    std::ifstream file(RC_FILE);
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("alias ", 0) == 0) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(6, eq-6);
                std::string v = line.substr(eq+1);
                v.erase(std::remove(v.begin(), v.end(), '"'), v.end());
                v.erase(std::remove(v.begin(), v.end(), '\''), v.end());
                aliases[k] = v;
            }
        } else if (line.rfind("prompt=", 0) == 0) {
            prompt = line.substr(7);
        } else if (!line.empty() && line[0] != '#') {
            rc_commands.push_back(line);
        }
    }
}

void print_help() {
    std::cout << "Available commands:\ncd, ls, pwd, echo, cat, touch, rm, mkdir, rmdir, cp, mv, head, tail, grep, wc, whoami, date, env, export, unset, history, which, clear, alias, unalias, help, exit, quit, [external commands]" << std::endl;
}

std::vector<std::string> get_path_commands() {
    std::vector<std::string> cmds;
    char* path = getenv("PATH");
    if (!path) return cmds;
    std::istringstream iss(path);
    std::string dir;
    while (std::getline(iss, dir, ':')) {
        DIR* d = opendir(dir.c_str());
        if (!d) continue;
        struct dirent* entry;
        while ((entry = readdir(d))) {
            if (entry->d_type == DT_REG || entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
                cmds.push_back(entry->d_name);
        }
        closedir(d);
    }
    std::sort(cmds.begin(), cmds.end());
    cmds.erase(std::unique(cmds.begin(), cmds.end()), cmds.end());
    return cmds;
}

std::vector<std::string> get_files(const std::string& prefix) {
    std::vector<std::string> files;
    std::string dir = ".";
    std::string file_prefix = prefix;
    auto slash = prefix.rfind('/');
    if (slash != std::string::npos) {
        dir = prefix.substr(0, slash);
        file_prefix = prefix.substr(slash+1);
    }
    DIR* d = opendir(dir.c_str());
    if (!d) return files;
    struct dirent* entry;
    while ((entry = readdir(d))) {
        std::string name = entry->d_name;
        if (name.find(file_prefix) == 0)
            files.push_back((dir == "." ? name : dir + "/" + name));
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<std::string> builtins = {"cd","ls","pwd","echo","cat","touch","rm","mkdir","rmdir","cp","mv","head","tail","grep","wc","whoami","date","env","export","unset","history","which","clear","alias","unalias","help","exit","quit"};
std::map<std::string, std::string> aliases;

char* completion_generator(const char* text, int state) {
    static size_t list_index;
    static std::vector<std::string> matches;
    if (state == 0) {
        matches.clear();
        std::string prefix(text);
        // Command completion for first word
        rl_completion_append_character = ' ';
        if (rl_point == 0 || (rl_line_buffer && std::string(rl_line_buffer).find(' ') == std::string::npos)) {
            for (const auto& b : builtins) if (b.find(prefix) == 0) matches.push_back(b);
            for (const auto& a : aliases) if (a.first.find(prefix) == 0) matches.push_back(a.first);
            for (const auto& c : get_path_commands()) if (c.find(prefix) == 0) matches.push_back(c);
        } else {
            // File completion
            for (const auto& f : get_files(prefix)) matches.push_back(f);
        }
        list_index = 0;
    }
    if (list_index < matches.size()) {
        return strdup(matches[list_index++].c_str());
    }
    return nullptr;
}

char** goonsh_completion(const char* text, int start, int end) {
    (void)start; (void)end;
    return rl_completion_matches(text, completion_generator);
}

int main() {
    std::string line;
    std::string prompt = "goonsh$ ";
    std::vector<std::string> rc_commands;
    load_config(aliases, prompt, rc_commands);
    load_history_file();
    rl_attempted_completion_function = goonsh_completion;
    std::cout << "Goonsh. Type 'help' for commands. 'exit' or 'quit' to leave." << std::endl;

    // Execute commands from ~/.goonshrc
    for (const auto& rc_line : rc_commands) {
        if (rc_line.empty()) continue;
        auto args = split(rc_line);
        if (args.empty()) continue;
        // Alias expansion
        if (aliases.count(args[0])) {
            std::vector<std::string> alias_args = split(aliases[args[0]]);
            alias_args.insert(alias_args.end(), args.begin() + 1, args.end());
            args = alias_args;
        }
        std::string cmd = args[0];
        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") { print_help(); continue; }
        if (cmd == "ls") {
            std::string path = (args.size() > 1) ? args[1] : ".";
            DIR* dir = opendir(path.c_str());
            if (!dir) { perror("ls"); continue; }
            std::vector<std::string> files;
            struct dirent* entry;
            size_t maxlen = 0;
            while ((entry = readdir(dir))) {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                    files.push_back(entry->d_name);
                    size_t len = strlen(entry->d_name);
                    if (len > maxlen) maxlen = len;
                }
            }
            closedir(dir);
            // Get terminal width
            int term_width = 80;
            struct winsize w;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
                term_width = w.ws_col;
            }
            size_t col_width = maxlen + 2;
            int cols = term_width / col_width;
            if (cols < 1) cols = 1;
            int rows = (files.size() + cols - 1) / cols;
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    int idx = c * rows + r;
                    if (idx < (int)files.size()) {
                        std::cout << std::left << std::setw(col_width) << files[idx];
                    }
                }
                std::cout << std::endl;
            }
            continue;
        }
        // External command
        pid_t pid = fork();
        if (pid == 0) {
            std::vector<char*> cargs;
            for (auto& arg : args) cargs.push_back(const_cast<char*>(arg.c_str()));
            cargs.push_back(nullptr);
            execvp(cargs[0], cargs.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }

    while (true) {
        char* input = readline(prompt.c_str());
        if (!input) break;
        line = input;
        free(input);
        if (line.empty()) continue;
        add_history(line.c_str());
        auto args = split(line);
        if (args.empty()) continue;
        // Alias expansion
        if (aliases.count(args[0])) {
            std::vector<std::string> alias_args = split(aliases[args[0]]);
            alias_args.insert(alias_args.end(), args.begin() + 1, args.end());
            args = alias_args;
        }
        std::string cmd = args[0];
        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") { print_help(); continue; }
        if (cmd == "ls") {
            std::string path = (args.size() > 1) ? args[1] : ".";
            DIR* dir = opendir(path.c_str());
            if (!dir) { perror("ls"); continue; }
            std::vector<std::string> files;
            struct dirent* entry;
            size_t maxlen = 0;
            while ((entry = readdir(dir))) {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                    files.push_back(entry->d_name);
                    size_t len = strlen(entry->d_name);
                    if (len > maxlen) maxlen = len;
                }
            }
            closedir(dir);
            // Get terminal width
            int term_width = 80;
            struct winsize w;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
                term_width = w.ws_col;
            }
            size_t col_width = maxlen + 2;
            int cols = term_width / col_width;
            if (cols < 1) cols = 1;
            int rows = (files.size() + cols - 1) / cols;
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    int idx = c * rows + r;
                    if (idx < (int)files.size()) {
                        std::cout << std::left << std::setw(col_width) << files[idx];
                    }
                }
                std::cout << std::endl;
            }
            continue;
        }
        // External command
        pid_t pid = fork();
        if (pid == 0) {
            std::vector<char*> cargs;
            for (auto& arg : args) cargs.push_back(const_cast<char*>(arg.c_str()));
            cargs.push_back(nullptr);
            execvp(cargs[0], cargs.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }
    save_history_file();
    return 0;
}
