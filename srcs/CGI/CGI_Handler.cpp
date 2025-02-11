/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_Handler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ashirzad <ashirzad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/11 12:34:29 by ashirzad          #+#    #+#             */
/*   Updated: 2025/02/11 15:21:36 by ashirzad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <map>

class CgiHandler
{
public:
    void executeCgi(const std::string& scriptPath, const std::map<std::string, std::string>& envVars)
	{
        pid_t pid = fork();
        if (pid == 0) {
            for (const auto& envVar : envVars) {
                setenv(envVar.first.c_str(), envVar.second.c_str(), 1);
            }
            // Redirect stdout to a pipe
            dup2(_stdoutPipe[1], STDOUT_FILENO);
            close(_stdoutPipe[0]);
            // Execute the script
            execl(scriptPath.c_str(), scriptPath.c_str(), NULL);
            exit(1); // If execl fails
        } else if (pid > 0) {
            // Parent process: Read the output of the script
            close(_stdoutPipe[1]);
            char buffer[4096];
            ssize_t bytesRead = read(_stdoutPipe[0], buffer, sizeof(buffer));
            if (bytesRead > 0) {
                _response.assign(buffer, bytesRead);
            }
            close(_stdoutPipe[0]);
            waitpid(pid, NULL, 0); // Wait for the child process to finish
        } else {
            std::cerr << "Failed to fork process" << std::endl;
        }
    }

    std::string getResponse() const {
        return _response;
    }

private:
    int _stdoutPipe[2];
    std::string _response;
};

int main() {
    CgiHandler handler;
    std::map<std::string, std::string> envVars = {
        {"REQUEST_METHOD", "GET"},
        {"QUERY_STRING", "name=John&age=30"},
        {"SCRIPT_NAME", "/cgi-bin/script.py"}
    };
    handler.executeCgi("/path/to/script.py", envVars);
    std::cout << "CGI Response:\n" << handler.getResponse() << std::endl;
    return 0;
}
