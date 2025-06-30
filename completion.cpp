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

// Autosuggestion logic (explicit key binding, no redisplay handler)
std::string find_history_suggestion(const char* input) {
    HIST_ENTRY** hist = history_list();
    if (!hist) return "";
    std::string prefix = input ? input : "";
    if (prefix.empty()) return "";
    const size_t MAX_SUGGEST_LEN = 256;
    for (int i = history_length - 1; i >= 0; --i) {
        std::string h = hist[i]->line;
        if (h.find(prefix) == 0 && h != prefix) {
            // Only suggest if not excessively longer than input
            if (h.length() - prefix.length() > MAX_SUGGEST_LEN) continue;
            // Only suggest if not an exact repeat of the last command
            if (i < history_length - 1 && h == hist[i+1]->line) continue;
            return h.substr(prefix.size());
        }
    }
    return "";
}

std::string find_file_suggestion(const char* input) {
    std::string prefix = input ? input : "";
    if (prefix.empty()) return "";
    // Only suggest for the last word
    size_t last_space = prefix.find_last_of(" ");
    std::string last_word = (last_space == std::string::npos) ? prefix : prefix.substr(last_space + 1);
    std::string before = (last_space == std::string::npos) ? "" : prefix.substr(0, last_space + 1);
    std::vector<std::string> files = get_files(last_word);
    // If the last word is already a full match for a file, do not suggest further
    for (const auto& f : files) {
        if (f == last_word) return "";
    }
    for (const auto& f : files) {
        if (f.find(last_word) == 0 && f != last_word && prefix.find(f) == std::string::npos) {
            return before + f.substr(last_word.size());
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

// (Removed duplicate goonsh_completion definition; see below for wrapper version)

// Ghost suggestion redisplay (fish-like)
std::string last_suggestion;
extern "C" void clear_last_suggestion() {
    if (!last_suggestion.empty()) {
        printf("\033[s");
        for (size_t i = 0; i < last_suggestion.length(); ++i) putchar(' ');
        printf("\033[u");
        fflush(stdout);
        last_suggestion.clear();
    }
}
#include <readline/readline.h>
// Static flag to track if completion is in progress
static bool goonsh_completion_active = false;

char** goonsh_completion(const char* text, int start, int end);

void goonsh_redisplay() {
    rl_redisplay();
    // Only show ghost suggestion if not in completion mode
    if (goonsh_completion_active) {
        // Clear any previous suggestion
        if (!last_suggestion.empty()) {
            printf("\033[s");
            for (size_t i = 0; i < last_suggestion.length(); ++i) putchar(' ');
            printf("\033[u");
            fflush(stdout);
            last_suggestion.clear();
        }
        return;
    }
    std::string input = rl_line_buffer ? rl_line_buffer : "";
    // Only show file suggestion if cursor is after a space (not in command position)
    size_t first_space = input.find(' ');
    // Only show file suggestion if:
    // - There is at least one space
    // - The cursor is after the first space
    // - The last word is not empty
    bool after_command = (
        first_space != std::string::npos &&
        rl_point > (int)first_space &&
        input.substr(first_space + 1).find_first_not_of(' ') != std::string::npos
    );
    std::string suggestion;
    if (after_command) {
        suggestion = find_file_suggestion(input.c_str());
    } else {
        suggestion = "";
    }
    // Always clear the max of previous and current suggestion area
    size_t clear_len = std::max(last_suggestion.length(), suggestion.length());
    if (clear_len > 0) {
        printf("\033[s");
        for (size_t i = 0; i < clear_len; ++i) putchar(' ');
        printf("\033[u");
        fflush(stdout);
    }
    if (!suggestion.empty()) {
        printf("\033[s");
        printf("\033[90m%s\033[0m", suggestion.c_str());
        printf("\033[u");
        fflush(stdout);
    }
    last_suggestion = suggestion;
}

// Wrap the completion function to set/clear the flag
char** goonsh_completion(const char* text, int start, int end) {
    goonsh_completion_active = true;
    char** result = rl_completion_matches(text, completion_generator);
    goonsh_completion_active = false;
    return result;
}

// Accept suggestion with right arrow (explicit, robust)
int accept_suggestion(int, int) {
    // Only suggest if cursor is at end of line
    if (rl_point != rl_end) return 0;
    std::string input = rl_line_buffer ? rl_line_buffer : "";
    std::string suggestion = find_history_suggestion(input.c_str());
    if (suggestion.empty()) suggestion = find_file_suggestion(input.c_str());
    if (!suggestion.empty()) {
        rl_insert_text(suggestion.c_str());
        rl_point = rl_end;
        rl_redisplay();
        return 0;
    }
    return 0;
}
