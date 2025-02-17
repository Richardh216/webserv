#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <string>

std::string getConfigPath(int argc, char **argv);
void		initStatusCodeInfo(std::map<int, std::string>& inf);
void		initContentTypes(std::map<std::string, std::string>& inf);

#endif
