/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_Handler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ashirzad <ashirzad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/11 12:34:29 by ashirzad          #+#    #+#             */
/*   Updated: 2025/02/12 13:03:52 by ashirzad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI_Handler.hpp"

CGIHandler::CGIHandler(void)
{
	this->_env["CONTENY_TYPE"] = "text/html";
	this->_env["REQUEST_METHOD"] = "GET";
	this->_env["QUERY_STRING"] = "name=Ali?message=Hello";
}

CGIHandler::~CGIHandler(void)
{

}

char **CGIHandler::getEnvAsCstrArray(void)
{
	char **env = new char*[_env.size() + 1];
	int i = 0;

	for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); it++)
	{
		std::string value = it->first + "=" + it->second;
		env[i] = new char[value.size() + 1];
		env[i] = strcpy(env[i], (const char *)value.c_str());
		i++;
	}
	env[i] = NULL;
	return (env);
}

void CGIHandler::executeCGI(void)
{
	int pipefd[2];

	if (pipe(pipefd) == -1)
	{
		perror("pipe error\n");
		return ;
	}
	pid_t pid = fork();
	if (pid == 0)
	{
		close(pipefd[0]);
		setenv("QUERY_STRING", _env["QUERY_STRING"].c_str(), 1);

		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);

		execve("script.py", NULL, getEnvAsCstrArray());
		perror("execve failed");
		exit(0);
	} else {
		close(pipefd[1]);
		char buffer[1024];

		ssize_t bytesRead;
		while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0';
			std::cout << buffer;
		}
		close(pipefd[0]);
		waitpid(pid, NULL, 0);
	}
}
