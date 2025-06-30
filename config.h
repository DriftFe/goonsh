#ifndef GOONSH_CONFIG_H
#define GOONSH_CONFIG_H

#include <string>
#include <vector>
#include <map>

void load_config(std::map<std::string, std::string>& aliases, std::string& prompt, std::vector<std::string>& rc_commands);

#endif // GOONSH_CONFIG_H
