#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <climits>
#include <algorithm>
#include <stdexcept>

class	ConfigParser {
	private:
		std::string	trim(const std::string &str);
		std::string	getValue(const std::string &line);
		std::vector<std::string>	split(const std::string &str, char delim);
		std::string	getLocationPath(const std::string &line);
		int			parseSize(const std::string &sizeStr);
		void		getPortHost(const std::string &line, serverConfig &config);
	public:
		std::vector<serverConfig>	servers;
		void	parseConfigFile(const std::string &filename);
		void	tester(const std::string &inFile);
		void	checkDuplicateServer(void);
		void	checkDuplicateLocationPath(void);
		void	checkRootAlias(void);
		void	checkErrorPagesPath(void);
		void	removeInvalidLocationPath(void);
		void	checkingFunction();
};
