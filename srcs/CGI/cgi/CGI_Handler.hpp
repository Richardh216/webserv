#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <iostream>
#include <map>
#include <string>
#include <unistd.h>

class CgiHandler
{
    private:
        std::map<std::string, std::string> _env;
        std::string _scriptPath;
        std::string _body;
        
    public:
        CgiHandler(std::string script_path); 
        ~CgiHandler(void);

        void _initEnv();
        char **getEnvAsCstrArray() const;
        std::string executeCgi();
};

#endif
