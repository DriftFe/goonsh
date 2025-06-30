#ifndef GOONSH_UTILS_H
#define GOONSH_UTILS_H

#include <string>
#include <vector>

std::vector<std::string> split(const std::string& line);
std::string expand_envvars(const std::string& input);
std::string expand_path(const std::string& path);
std::vector<std::string> get_files(const std::string& prefix);
std::vector<std::string> get_path_commands();

#endif // GOONSH_UTILS_H
