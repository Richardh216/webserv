#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

#include "../../includes/webserv.hpp"

class CgiHandler
{
	private:
		std::map<std::string, std::string> _env;
		std::string _scriptPath;
		std::string _body;

	public:
		CgiHandler(const HttpRequest &request, const Webserv &webserv, std::string scriptPath);
		~CgiHandler(void);

		std::string getQueryString(std::string path);
		char **getEnvAsCstrArray() const;
		std::string executeCgi();
};

#endif
