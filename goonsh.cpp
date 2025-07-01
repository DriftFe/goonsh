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
#include <filesystem>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ioctl.h>
#include <iomanip>
#include <signal.h>
#include <readline/rltypedefs.h>
#include <iomanip>
#include <functional>
#include <termios.h>
#include <cassert>
#include <cstdlib>
#include <set>
#include <utility>
#include <cctype>
#include <stdexcept>
#include <locale>
#include <limits>
#include <memory>
#include <cstddef>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <glob.h>
#include <regex>
#include "utils.h"
#include "history.h"
#include "config.h"
#include "completion.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

// Debug mode: set GOONSH_DEBUG=1 in environment to enable
bool GOONSH_DEBUG = (getenv("GOONSH_DEBUG") && std::string(getenv("GOONSH_DEBUG")) == "1");
#define DBG(fmt, ...) do { if (GOONSH_DEBUG) fprintf(stderr, "[DBG] " fmt "\n", ##__VA_ARGS__); } while(0)

std::vector<std::string> builtins = {"cd","ls","pwd","echo","cat","touch","rm","mkdir","rmdir","cp","mv","head","tail","grep","wc","whoami","date","env","export","unset","history","which","clear","alias","unalias","help","exit","quit","man","time","jobs","fg","bg"};
std::map<std::string, std::string> aliases;
std::map<std::string, std::string> shell_vars = {{"GOONSH_THEME", "default"}};
int last_status = 0;
std::vector<pid_t> bg_jobs;

// --- SIGWINCH handler (must be global for signal) ---
void handle_winch(int sig) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    // Optionally store w.ws_row and w.ws_col if needed
    // Do not redraw prompt here
    // Let readline handle SIGWINCH internally
}

// --- Utility Functions ---
std::string get_prompt(const std::string& ps1) {
    std::ostringstream out;
    for (size_t i = 0; i < ps1.size(); ++i) {
        if (ps1[i] == '\\' && i+1 < ps1.size()) {
            char c = ps1[++i];
            if (c == 'u') {
                struct passwd* pw = getpwuid(getuid());
                out << (pw ? pw->pw_name : "?");
            } else if (c == 'h') {
                char hostname[256];
                gethostname(hostname, sizeof(hostname));
                out << hostname;
            } else if (c == 'w') {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd))) out << cwd;
            } else if (c == '$') {
                out << (geteuid() == 0 ? "#" : "$ ");
            } else {
                out << '\\' << c;
            }
        } else {
            out << ps1[i];
        }
    }
    return out.str();
}

struct CmdSegment {
    std::vector<std::string> args;
    std::string input_redir;
    std::string output_redir;
    std::string output_append_redir;
    bool background = false;
};

std::vector<CmdSegment> parse_pipeline(const std::string& line) {
    std::vector<CmdSegment> segments;
    CmdSegment seg;
    std::string token;
    bool in_single = false, in_double = false, escape = false;
    for (size_t i = 0; i <= line.size(); ++i) {
        char c = (i < line.size()) ? line[i] : ' ';
        if (escape) {
            token += c;
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if (!in_single && !in_double && (isspace(c) || c == '|' || c == '>' || c == '<' || c == '&' || i == line.size())) {
            if (!token.empty()) {
                seg.args.push_back(token);
                token.clear();
            }
            // Always split << and >> as separate arguments
            if (c == '<' && i+1 < line.size() && line[i+1] == '<') {
                seg.args.push_back("<<");
                i++;
            } else if (c == '>' && i+1 < line.size() && line[i+1] == '>') {
                seg.args.push_back(">>");
                i++;
            } else if (c == '>' || c == '<' || c == '|' || c == '&') {
                std::string op(1, c);
                seg.args.push_back(op);
            }
            if (c == '|') {
                segments.push_back(seg);
                seg = CmdSegment();
            } else if (c == '>') {
                if (i+1 < line.size() && line[i+1] == '>') {
                    i++;
                    while (i+1 < line.size() && isspace(line[i+1])) i++;
                    size_t start = ++i;
                    while (i < line.size() && !isspace(line[i]) && line[i] != '|' && line[i] != '<' && line[i] != '&') i++;
                    seg.output_append_redir = line.substr(start, i-start);
                    i--;
                } else {
                    while (i+1 < line.size() && isspace(line[i+1])) i++;
                    size_t start = ++i;
                    while (i < line.size() && !isspace(line[i]) && line[i] != '|' && line[i] != '<' && line[i] != '&') i++;
                    seg.output_redir = line.substr(start, i-start);
                    i--;
                }
            } else if (c == '<') {
                while (i+1 < line.size() && isspace(line[i+1])) i++;
                size_t start = ++i;
                while (i < line.size() && !isspace(line[i]) && line[i] != '|' && line[i] != '>' && line[i] != '&') i++;
                seg.input_redir = line.substr(start, i-start);
                i--;
            } else if (c == '&') {
                seg.background = true;
            }
        } else {
            token += c;
        }
    }
    segments.push_back(seg);
    return segments;
}

int run_pipeline(std::vector<CmdSegment>& segments) {
    int n = segments.size();
    int prev_fd = -1;
    std::vector<pid_t> pids;
    pid_t pgid = 0;
    int shell_terminal = STDIN_FILENO;
    int shell_pgid = getpgrp();
    struct termios shell_tmodes;
    tcgetattr(shell_terminal, &shell_tmodes);

    for (int i = 0; i < n; ++i) {
        int pipefd[2];
        if (i < n-1) pipe(pipefd);
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(0, pgid ? pgid : getpid());
            if (!segments.back().background)
                tcsetpgrp(shell_terminal, getpid());
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            if (!segments[i].input_redir.empty()) {
                int fd = open(segments[i].input_redir.c_str(), O_RDONLY);
                if (fd >= 0) { dup2(fd, 0); close(fd); }
            } else if (prev_fd != -1) {
                dup2(prev_fd, 0); close(prev_fd);
            }
            if (!segments[i].output_redir.empty()) {
                int fd = open(segments[i].output_redir.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666);
                if (fd >= 0) { dup2(fd, 1); close(fd); }
            } else if (!segments[i].output_append_redir.empty()) {
                int fd = open(segments[i].output_append_redir.c_str(), O_WRONLY|O_CREAT|O_APPEND, 0666);
                if (fd >= 0) { dup2(fd, 1); close(fd); }
            } else if (i < n-1) {
                dup2(pipefd[1], 1); close(pipefd[1]);
            }
            if (i < n-1) close(pipefd[0]);
            std::vector<std::string> final_args;
            for (auto& arg : segments[i].args) {
                // You may want to add command_substitute, math_expand, expand_shell_vars, etc. here
                final_args.push_back(arg);
            }
            if (final_args.empty()) _exit(0);
            char** argv = new char*[final_args.size()+1];
            for (size_t j = 0; j < final_args.size(); ++j) argv[j] = strdup(final_args[j].c_str());
            argv[final_args.size()] = nullptr;
            // Debug print for execvp
            std::cerr << "[DEBUG] execvp: ";
            for (size_t j = 0; j < final_args.size(); ++j) std::cerr << argv[j] << " ";
            std::cerr << std::endl;
            execvp(argv[0], argv);
            perror("execvp");
            _exit(127);
        } else if (pid > 0) {
            if (prev_fd != -1) close(prev_fd);
            if (i < n-1) { close(pipefd[1]); prev_fd = pipefd[0]; }
            setpgid(pid, pgid ? pgid : pid);
            if (!pgid) pgid = pid;
            if (i == 0 && !segments.back().background) {
                tcsetpgrp(shell_terminal, pid);
            }
            pids.push_back(pid);
        } else {
            perror("fork");
            return 1;
        }
    }
    int status = 0;
    if (!segments.back().background) {
        for (auto pid : pids) waitpid(pid, &status, WUNTRACED);
        tcsetpgrp(shell_terminal, shell_pgid);
        tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
    } else {
        for (auto pid : pids) bg_jobs.push_back(pid);
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

// --- Main Loop ---
int main(int argc, char* argv[]) {
    // Job control setup
    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    // signal(SIGWINCH, handle_winch); // Let readline handle SIGWINCH

    std::string line;
    std::string ps1 = "[\\u@\\h \\w]$ ";
    std::string prompt;
    std::vector<std::string> rc_commands;
    load_config(aliases, prompt, rc_commands);
    if (!prompt.empty()) ps1 = prompt;
    load_history_file();
    rl_attempted_completion_function = goonsh_completion;
    rl_redisplay_function = goonsh_redisplay; // Enable fish-like ghost suggestions
    rl_bind_keyseq("\033[C", accept_suggestion); // Right arrow
    // Custom SIGINT handler for main shell
    extern void clear_last_suggestion();
    auto sigint_handler = [](int) {
        clear_last_suggestion();
        rl_replace_line("", 0);
        rl_crlf();
        rl_on_new_line();
        rl_redisplay();
    };
    signal(SIGINT, sigint_handler);
    std::cout << COLOR_MAGENTA << "Welcome To Goonsh >~< (Type 'help' for commands. 'exit' or 'quit' to leave)" << COLOR_RESET << std::endl;

    // Scripting mode: goonsh file.sh
    if (argc == 2) {
        std::ifstream script(argv[1]);
        if (!script) { std::cerr << "Cannot open script: " << argv[1] << std::endl; return 1; }
        while (std::getline(script, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto segments = parse_pipeline(line);
            last_status = run_pipeline(segments);
        }
        return last_status;
    }

    // Execute commands from ~/.goonshrc
    for (const auto& rc_line : rc_commands) {
        if (rc_line.empty()) continue;
        auto segments = parse_pipeline(rc_line);
        run_pipeline(segments);
    }

    while (true) {
        std::cerr << "[DEBUG] Top of main loop" << std::endl;
        prompt = get_prompt(ps1);
        char* input = readline(prompt.c_str());
        if (!input) {
            std::cout << "exit" << std::endl;
            break;
        }
        line = input;
        free(input);
        // Limit input line length
        const size_t MAX_INPUT_LEN = 4096;
        if (line.length() > MAX_INPUT_LEN) {
            std::cerr << "Error: input line too long (max " << MAX_INPUT_LEN << " chars)" << std::endl;
            continue;
        }
        // Remove comments (not inside quotes)
        bool in_single = false, in_double = false;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"' && !in_single) in_double = !in_double;
            else if (line[i] == '\'' && !in_double) in_single = !in_single;
            else if (line[i] == '#' && !in_single && !in_double) { line = line.substr(0, i); break; }
        }
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        add_history(line.c_str());
        auto segments = parse_pipeline(line);
        // If single command, not background, and is a builtin, run in parent
        if (segments.size() == 1 && !segments[0].background && !segments[0].args.empty()) {
            std::string cmd = segments[0].args[0];
            std::cerr << "[DEBUG] Parsed command: '" << cmd << "'\n";
            static const std::map<std::string, std::string> builtin_help = {
                {"cd",    "Usage: cd [DIR]\nChange the current directory to DIR."},
                {"help",  "Usage: help\nShow this help message."},
                {"exit",  "Usage: exit\nExit the shell."},
                {"cat",   "Usage: cat [FILE]...\nConcatenate FILE(s) to standard output."},
                {"rm",    "Usage: rm [FILE]...\nRemove (unlink) the FILE(s)."},
                {"touch", "Usage: touch [FILE]...\nChange file timestamps."},
                {"mkdir", "Usage: mkdir [DIRECTORY]...\nCreate the DIRECTORY(ies), if they do not already exist."}
            };
            // Only handle true builtins
            if (cmd == "exit") {
                std::cout << "Bye!" << std::endl;
                break;
            } else if (cmd == "help") {
                std::cout << "Available commands:\n";
                for (const auto& b : builtins) std::cout << "  " << b << std::endl;
                continue;
            } else if (cmd == "cd") {
                const char* target = (segments[0].args.size() > 1) ? segments[0].args[1].c_str() : getenv("HOME");
                if (chdir(target) != 0) perror("cd");
                continue;
            }
            // Special case: cat with no arguments, print help and do not run
            if (cmd == "cat" && segments[0].args.size() == 1) {
                auto it = builtin_help.find(cmd);
                if (it != builtin_help.end()) {
                    std::cout << it->second << std::endl;
                } else {
                    std::cerr << "Error: missing arguments for command '" << cmd << "'" << std::endl;
                }
                continue;
            }
        }
        // Here-document (<< delimiter) support for first segment
        for (auto& seg : segments) {
            for (size_t i = 0; i + 1 < seg.args.size(); ++i) {
                if (seg.args[i] == "<<") {
                    std::string delimiter = seg.args[i + 1];
                    std::string heredoc;
                    std::string inputline;
                    std::cout << "> " << std::flush;
                    while (std::getline(std::cin, inputline)) {
                        if (inputline == delimiter) break;
                        heredoc += inputline + "\n";
                        std::cout << "> " << std::flush;
                    }
                    int pipefd[2];
                    pipe(pipefd);
                    write(pipefd[1], heredoc.c_str(), heredoc.size());
                    close(pipefd[1]);
                    seg.input_redir = "/dev/fd/" + std::to_string(pipefd[0]);
                    seg.args.erase(seg.args.begin() + i, seg.args.begin() + i + 2);
                    break;
                }
            }
        }
        // Ignore SIGINT and set SIGWINCH to default while running external commands
        struct sigaction old_int, old_winch, ign, def;
        ign.sa_handler = SIG_IGN;
        sigemptyset(&ign.sa_mask);
        ign.sa_flags = 0;
        def.sa_handler = SIG_DFL;
        sigemptyset(&def.sa_mask);
        def.sa_flags = 0;
        sigaction(SIGINT, &ign, &old_int);
        sigaction(SIGWINCH, &def, &old_winch);
        last_status = run_pipeline(segments);
        // Restore custom handlers after command execution
        sigaction(SIGINT, &old_int, nullptr);
        sigaction(SIGWINCH, &old_winch, nullptr);
        save_history_file();
    }
    return 0;
}
