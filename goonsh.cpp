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
#include <signal.h>
#include <regex>
#include <readline/rltypedefs.h>
#include "utils.h"
#include "history.h"
#include "config.h"
#include "completion.h"

void print_help() {
    std::cout << "Available commands:\ncd, ls, pwd, echo, cat, touch, rm, mkdir, rmdir, cp, mv, head, tail, grep, wc, whoami, date, env, export, unset, history, which, clear, alias, unalias, help, exit, quit, [external commands]" << std::endl;
}

std::vector<std::string> builtins = {"cd","ls","pwd","echo","cat","touch","rm","mkdir","rmdir","cp","mv","head","tail","grep","wc","whoami","date","env","export","unset","history","which","clear","alias","unalias","help","exit","quit"};
std::map<std::string, std::string> aliases;

void sigint_handler(int) {
    std::cout << std::endl;
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

// Expand envvars in all args
std::vector<std::string> expand_args(const std::vector<std::string>& args) {
    std::vector<std::string> out;
    for (const auto& arg : args) out.push_back(expand_envvars(arg));
    return out;
}

int main() {
    std::string line;
    std::string prompt = "goonsh$ ";
    std::vector<std::string> rc_commands;
    load_config(aliases, prompt, rc_commands);
    load_history_file();
    rl_attempted_completion_function = goonsh_completion;
    rl_redisplay_function = goonsh_redisplay;
    rl_bind_keyseq("\033[C", accept_suggestion); // Right arrow
    signal(SIGINT, sigint_handler);
    std::cout << "Welcome To Goonsh >~< (Type 'help' for commands. 'exit' or 'quit' to leave)" << std::endl;

    // Execute commands from ~/.goonshrc
    int last_status = 0;
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
        // Expand envvars in all args
        args = expand_args(args);
        // Inline assignment: VAR=val
        if (args[0].find('=') != std::string::npos && args[0].find('=') != 0 && args[0].find('=') < args[0].size()-1 && args[0].find_first_of(" ") == std::string::npos) {
            auto eq = args[0].find('=');
            std::string var = args[0].substr(0, eq);
            std::string val = args[0].substr(eq+1);
            setenv(var.c_str(), val.c_str(), 1);
            continue;
        }
        std::string cmd = args[0];
        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") { print_help(); continue; }
        if (cmd == "cd") {
            const char* target = (args.size() > 1) ? args[1].c_str() : getenv("HOME");
            if (chdir(target) != 0) {
                perror("cd");
            }
            continue;
        }
        if (cmd == "export") {
            if (args.size() == 2 && args[1].find('=') != std::string::npos) {
                auto eq = args[1].find('=');
                std::string var = args[1].substr(0, eq);
                std::string val = args[1].substr(eq+1);
                setenv(var.c_str(), val.c_str(), 1);
            } else if (args.size() == 2) {
                setenv(args[1].c_str(), getenv(args[1].c_str()) ? getenv(args[1].c_str()) : "", 1);
            } else {
                std::cerr << "Usage: export VAR or export VAR=val" << std::endl;
            }
            continue;
        }
        if (cmd == "unset") {
            if (args.size() == 2) {
                unsetenv(args[1].c_str());
            } else {
                std::cerr << "Usage: unset VAR" << std::endl;
            }
            continue;
        }
        if (cmd == "env") {
            extern char **environ;
            for (char **env = environ; *env; ++env) {
                std::cout << *env << std::endl;
            }
            continue;
        }
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
            std::cerr << "goonsh: command not found: " << args[0] << std::endl;
            exit(127);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        } else {
            perror("fork");
            last_status = 1;
        }
    }

    while (true) {
        bool should_exit = false;
                char* input = readline(prompt.c_str());
        if (!input) {
            break;
        }
        line = input;
        free(input);
        // Remove comments (not inside quotes)
        bool in_single = false, in_double = false;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"' && !in_single) in_double = !in_double;
            else if (line[i] == '\'' && !in_double) in_single = !in_single;
            else if (line[i] == '#' && !in_single && !in_double) { line = line.substr(0, i); break; }
        }
        // Ignore empty/whitespace-only input
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        add_history(line.c_str());

        // Advanced command chaining: split by ;, &&, || (not in quotes)
        std::vector<std::pair<std::string, std::string>> segments; // {cmd, op}
        size_t pos = 0, last = 0;
        in_single = in_double = false;
        std::string last_op;
        while (pos < line.size()) {
            if (line[pos] == '"' && !in_single) in_double = !in_double;
            else if (line[pos] == '\'' && !in_double) in_single = !in_single;
            else if (!in_single && !in_double) {
                if (line.compare(pos, 2, "&&") == 0) {
                    segments.emplace_back(line.substr(last, pos-last), last_op);
                    last_op = "&&";
                    pos += 2; last = pos; continue;
                } else if (line.compare(pos, 2, "||") == 0) {
                    segments.emplace_back(line.substr(last, pos-last), last_op);
                    last_op = "||";
                    pos += 2; last = pos; continue;
                } else if (line[pos] == ';') {
                    segments.emplace_back(line.substr(last, pos-last), last_op);
                    last_op = ";";
                    ++pos; last = pos; continue;
                }
            }
            ++pos;
        }
        segments.emplace_back(line.substr(last), last_op);
        int last_status = 0;
        for (const auto& seg : segments) {
        std::string segline = seg.first;
        std::string op = seg.second;
                if (segline.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        if (op == "&&" && last_status != 0) continue;
        if (op == "||" && last_status == 0) continue;
        auto args = split(segline);
        if (args.empty()) continue;
                // Alias expansion
        if (aliases.count(args[0])) {
        std::vector<std::string> alias_args = split(aliases[args[0]]);
        alias_args.insert(alias_args.end(), args.begin() + 1, args.end());
        args = alias_args;
        }
        // Expand envvars in all args
        args = expand_args(args);
        // Inline assignment: VAR=val
        if (args[0].find('=') != std::string::npos && args[0].find('=') != 0 && args[0].find('=') < args[0].size()-1 && args[0].find_first_of(" ") == std::string::npos) {
        auto eq = args[0].find('=');
        std::string var = args[0].substr(0, eq);
        std::string val = args[0].substr(eq+1);
        setenv(var.c_str(), val.c_str(), 1);
                continue;
        }
        std::string cmd = args[0];
        if (cmd == "exit" || cmd == "quit") { should_exit = true; break; }
        if (cmd == "help") { print_help(); continue; }
        if (cmd == "cd") {
            const char* target = (args.size() > 1) ? args[1].c_str() : getenv("HOME");
                        if (chdir(target) != 0) {
                perror("cd");
            }
            continue;
        }
        if (cmd == "export") {
            if (args.size() == 2 && args[1].find('=') != std::string::npos) {
                auto eq = args[1].find('=');
                std::string var = args[1].substr(0, eq);
                std::string val = args[1].substr(eq+1);
                setenv(var.c_str(), val.c_str(), 1);
                            } else if (args.size() == 2) {
                setenv(args[1].c_str(), getenv(args[1].c_str()) ? getenv(args[1].c_str()) : "", 1);
                            } else {
                std::cerr << "Usage: export VAR or export VAR=val" << std::endl;
            }
            continue;
        }
        if (cmd == "unset") {
            if (args.size() == 2) {
                unsetenv(args[1].c_str());
                            } else {
                std::cerr << "Usage: unset VAR" << std::endl;
            }
            continue;
        }
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
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
                    } else {
            perror("fork");
            last_status = 1;
        }
    }
    save_history_file();
    if (should_exit) {
                break;
    }
}
return 0;
}
