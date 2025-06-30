#include <cstdio>
#include "completion.h"
#include "utils.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstring>

extern std::vector<std::string> builtins;
extern std::map<std::string, std::string> aliases;

//Autosuggestion logic
std::string current_suggestion;

std::string find_suggestion(const char* input) {
    HIST_ENTRY** hist = history_list();
    if (!hist) return "";
    std::string prefix = input ? input : "";
    if (prefix.empty()) return "";
    for (int i = history_length - 1; i >= 0; --i) {
        std::string h = hist[i]->line;
        if (h.find(prefix) == 0 && h != prefix) {
            return h.substr(prefix.size());
        }
    }
    return "";
}

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

// Custom redisplay to show autosuggestion
void goonsh_redisplay() {
    rl_redisplay();
    std::string suggestion = find_suggestion(rl_line_buffer);
    if (!suggestion.empty()) {
        // Save cursor position
        printf("\033[s");
        // Print suggestion in gray
        printf("\033[90m%s\033[0m", suggestion.c_str());
        // Restore cursor
        printf("\033[u");
        fflush(stdout);
    } else if (current_suggestion.length() > 0) {
        // Clear previous suggestion if input is now empty or no suggestion
        printf("\033[s");
        for (size_t i = 0; i < current_suggestion.length(); ++i) putchar(' ');
        printf("\033[u");
        fflush(stdout);
    }
    current_suggestion = suggestion;
}

// Accept suggestion with right arrow
int accept_suggestion(int, int) {
    if (!current_suggestion.empty()) {
        rl_insert_text(current_suggestion.c_str());
        rl_point = rl_end;
        current_suggestion.clear();
        rl_redisplay();
        return 0;
    }
    return 0;
}
