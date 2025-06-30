#include "config.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <algorithm>

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
