#include "CGI_Handler.hpp"

CgiHandler::CgiHandler(std::string scirpt_path) : _scriptPath(scirpt_path), _body("")
{
    this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
    this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    this->_env["REQUEST_METHOD"] = "POST";
    this->_env["CONTENT_LENGTH"] = std::to_string(20);
    this->_env["QUERY_STRING"] = "name=Ali&message=hello";
    this->_env["SERVER_PORT"] = "8000";
    this->_env["REMOTE_PORT"] = "8000";
    this->_env["REMOTE_ADDR"] = "127.0.0.1";
    this->_env["HTTP_HOST"] =  "example.com";
    this->_env["HTTP_USER_AGENT"] =  "Mozilla/5.0";
    this->_env["HTTP_CONTENT_TYPE"] = "application/json";

}

CgiHandler::~CgiHandler(void)
{

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
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return "";
    }


    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char **envp = getEnvAsCstrArray();

        execve(_scriptPath.c_str(), nullptr, envp);

        perror("execve");
        exit(1);
    }
    else {
        close(pipefd[1]);

        char buffer[1024];
        ssize_t bytesRead;
        std::string output;

        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output.append(buffer);
        }
        close(pipefd[0]);
        waitpid(pid, nullptr, 0);

        _body = output;
        return _body;
    }
}
