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
		CgiHandler(const HttpRequest &request, std::string scriptPath, std::string uploadPath);
		~CgiHandler(void);

		std::string getUploadPath(std::string scriptPath, std::string uploadPath);
		std::string getQueryString(std::string path);
		char *ft_strcpy(char *s1, const char *s2);
		char **getEnvAsCstrArray();
		std::string executeCgi();
};

#endif
