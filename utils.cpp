#include "utils.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <regex>
#include <sstream>
#include <dirent.h>
#include <algorithm>

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_single = false, in_double = false, escape = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (escape) {
            token += c;
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (isspace(c) && !in_single && !in_double) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

std::string expand_envvars(const std::string& input) {
    std::string result;
    std::regex env_re(R"(\$([A-Za-z_][A-Za-z0-9_]*))");
    std::sregex_iterator it(input.begin(), input.end(), env_re), end;
    size_t last = 0;
    for (; it != end; ++it) {
        result += input.substr(last, it->position() - last);
        const char* val = getenv((*it)[1].str().c_str());
        if (val) result += val;
        last = it->position() + it->length();
    }
    result += input.substr(last);
    return result;
}

std::string expand_path(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p[0] == '~') {
        const char* home = getenv("HOME");
        if (home) p = std::string(home) + p.substr(1);
    }
    p = expand_envvars(p);
    return p;
}

std::vector<std::string> get_files(const std::string& prefix) {
    std::vector<std::string> files;
    std::string expanded_prefix = expand_path(prefix);
    std::string dir = ".";
    std::string file_prefix = expanded_prefix;
    auto slash = expanded_prefix.rfind('/');
    std::string user_prefix = prefix;
    if (slash != std::string::npos) {
        dir = expanded_prefix.substr(0, slash);
        file_prefix = expanded_prefix.substr(slash+1);
        user_prefix = prefix.substr(0, prefix.rfind('/')+1);
    } else {
        user_prefix = "";
    }
    DIR* d = opendir(dir.c_str());
    if (!d) return files;
    struct dirent* entry;
    while ((entry = readdir(d))) {
        std::string name = entry->d_name;
        if (name.find(file_prefix) == 0) {
            std::string fullpath = dir + "/" + name;
            struct stat st;
            if (stat(fullpath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                files.push_back(user_prefix + name + "/");
            } else {
                files.push_back(user_prefix + name);
            }
        }
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    return files;
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
