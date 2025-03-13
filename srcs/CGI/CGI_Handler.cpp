#include "CGI_Handler.hpp"

CgiHandler::CgiHandler(const HttpRequest &request, const Webserv &webserv, std::string scriptPath)
{
	(void)webserv;
    this->_env["REQUEST_METHOD"] = request.method;
    this->_env["CONTENT_TYPE"] = request.headers.find("Content-Type")->second;
    this->_env["CONTENT_LENGTH"] = request.headers.find("Content-Length")->second;
	this->_env["QUERY_STRING"] = getQueryString(request.path);

	_scriptPath = scriptPath;
	_body = request.body;
}

CgiHandler::~CgiHandler(void)
{

}

std::string CgiHandler::getQueryString(std::string path)
{
	std::string query;

	if (path.find("?") == std::string::npos)
		return ("");
	int i = path.find("?") + 1;
	while (path[i])
	{
		query += path[i];
		i++;
	}
	return (query);
}

char **CgiHandler::getEnvAsCstrArray(void) const
{
	char **env;

	env = new char*[_env.size() + 1];
	std::map<std::string, std::string>::const_iterator it;
	int i = 0;
	for (it = _env.begin(); it != _env.end(); it++)
	{
		std::string value = it->first + "=" + it->second;
		env[i] = new char[value.size() + 1];
		env[i] = strcpy(env[i], (const char *)value.c_str());
		i++;
	}
	env[i] = NULL;
	return (env);
}

std::string CgiHandler::executeCgi()
{
	int stdoutPipe[2];
	int stdinPipe[2];

	if (pipe(stdoutPipe) == -1 || pipe(stdinPipe) == -1)
	{
		perror("pipe");
		return "";
	}

	pid_t pid = fork();
	if (pid == 0)
	{
		close(stdoutPipe[0]);
		dup2(stdoutPipe[1], STDOUT_FILENO);
		close(stdoutPipe[1]);

		close(stdinPipe[1]);
		dup2(stdinPipe[0], STDIN_FILENO);
		close(stdinPipe[0]);

		char **envp = getEnvAsCstrArray();
		std::string python = "/Users/ashirzad/homebrew/bin/python3.10";

		char *argv[] = {(char *)python.c_str(), (char *)_scriptPath.c_str(), NULL};
		execve(python.c_str(), argv, envp);

		perror("execve");
		exit(1);
	}
	else if (pid > 0)
	{
		close(stdoutPipe[1]);
		close(stdinPipe[0]);

		if (!_body.empty())
		{
			write(stdinPipe[1], _body.c_str(), _body.size());
		}
		close(stdinPipe[1]);

		char buffer[1024];
		ssize_t bytesRead;
		std::string output;

		while ((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0';
			output.append(buffer);
		}
		close(stdoutPipe[0]);
		waitpid(pid, nullptr, 0);

		return output;
	}
	return "fork error";
}

