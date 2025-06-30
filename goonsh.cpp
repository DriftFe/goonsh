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

std::vector<std::string> builtins = {"cd","ls","pwd","echo","cat","touch","rm","mkdir","rmdir","cp","mv","head","tail","grep","wc","whoami","date","env","export","unset","history","which","clear","alias","unalias","help","exit","quit","man","time","jobs","fg","bg"};
std::map<std::string, std::string> aliases;
std::map<std::string, std::string> shell_vars = {{"GOONSH_THEME", "default"}};
int last_status = 0;
std::vector<pid_t> bg_jobs;

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

// --- Globbing ---
std::vector<std::string> expand_glob(const std::string& pattern) {
    glob_t globbuf;
    std::vector<std::string> results;
    if (glob(pattern.c_str(), 0, nullptr, &globbuf) == 0) {
        for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
            results.push_back(globbuf.gl_pathv[i]);
        }
    }
    globfree(&globbuf);
    if (results.empty()) results.push_back(pattern); // fallback
    return results;
}

// --- Brace Expansion ---
std::vector<std::string> expand_braces(const std::string& arg) {
    std::regex brace_re(R"(\{(\d+)\.\.(\d+)\})");
    std::smatch m;
    if (std::regex_search(arg, m, brace_re)) {
        int start = std::stoi(m[1]);
        int end = std::stoi(m[2]);
        std::vector<std::string> expanded;
        for (int i = start; i <= end; ++i) {
            std::string s = arg;
            s.replace(m.position(0), m.length(0), std::to_string(i));
            expanded.push_back(s);
        }
        return expanded;
    }
    return {arg};
}

// --- Command Substitution ---
std::string command_substitute(const std::string& arg) {
    std::string out = arg;
    std::regex sub_re(R"(\$\(([^)]+)\))");
    std::smatch m;
    while (std::regex_search(out, m, sub_re)) {
        std::string cmd = m[1];
        FILE* fp = popen(cmd.c_str(), "r");
        std::string result;
        if (fp) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp)) result += buf;
            pclose(fp);
        }
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') result.pop_back();
        out.replace(m.position(0), m.length(0), result);
    }
    return out;
}

// --- Math Expansion ---
std::string math_expand(const std::string& arg) {
    std::regex math_re(R"(\$\(\(([^)]+)\)\))");
    std::smatch m;
    if (std::regex_search(arg, m, math_re)) {
        std::string expr = m[1];
        // Only support +, -, *, /, % and integers for now
        int result = 0;
        std::istringstream iss(expr);
        int lhs; char op; int rhs;
        iss >> lhs >> op >> rhs;
        switch (op) {
            case '+': result = lhs + rhs; break;
            case '-': result = lhs - rhs; break;
            case '*': result = lhs * rhs; break;
            case '/': result = rhs ? lhs / rhs : 0; break;
            case '%': result = rhs ? lhs % rhs : 0; break;
            default: result = 0;
        }
        std::string s = arg;
        s.replace(m.position(0), m.length(0), std::to_string(result));
        return s;
    }
    return arg;
}

// --- Shell Variable Expansion ---
std::string expand_shell_vars(const std::string& arg) {
    std::regex var_re(R"(\$([A-Za-z_][A-Za-z0-9_]*)\b)");
    std::smatch m;
    std::string out = arg;
    while (std::regex_search(out, m, var_re)) {
        std::string var = m[1];
        std::string val = shell_vars.count(var) ? shell_vars[var] : (getenv(var.c_str()) ? getenv(var.c_str()) : "");
        out.replace(m.position(0), m.length(0), val);
    }
    return out;
}

// --- Parsing and Execution ---

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
    for (int i = 0; i < n; ++i) {
        int pipefd[2];
        if (i < n-1) pipe(pipefd);
        pid_t pid = fork();
        if (pid == 0) {
            // Restore default SIGINT in child
            signal(SIGINT, SIG_DFL);
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
                arg = command_substitute(arg);
                arg = math_expand(arg);
                arg = expand_shell_vars(arg);
                std::vector<std::string> braces = expand_braces(arg);
                for (auto& b : braces) {
                    std::vector<std::string> globs = expand_glob(b);
                    final_args.insert(final_args.end(), globs.begin(), globs.end());
                }
            }
            if (final_args.empty()) _exit(0);
            char** argv = new char*[final_args.size()+1];
            for (size_t j = 0; j < final_args.size(); ++j) argv[j] = strdup(final_args[j].c_str());
            argv[final_args.size()] = nullptr;
                        execvp(argv[0], argv);
            perror("execvp");
            _exit(127);
        } else if (pid > 0) {
            if (prev_fd != -1) close(prev_fd);
            if (i < n-1) { close(pipefd[1]); prev_fd = pipefd[0]; }
            pids.push_back(pid);
        } else {
            perror("fork");
            return 1;
        }
    }
    int status = 0;
    for (auto pid : pids) {
        if (!segments.back().background) waitpid(pid, &status, 0);
        else bg_jobs.push_back(pid);
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

// --- Main Loop ---

int main(int argc, char* argv[]) {
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
    // SIGWINCH handler for terminal resize
    auto sigwinch_handler = [](int) {
        rl_resize_terminal();
        rl_redisplay();
    };
    signal(SIGWINCH, sigwinch_handler);
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
        }
        last_status = run_pipeline(segments);
        save_history_file();
    }
    return 0;
}
