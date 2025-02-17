#include "CGI_Handler.hpp"

int main(void)
{
    std::string script_path = "script.py";
    CgiHandler handler(script_path);

    std::string response = handler.executeCgi();

    std::cout << response << std::endl;
    return(0);
}