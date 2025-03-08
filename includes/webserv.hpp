#ifndef	WEBSERV_HPP
#define	WEBSERV_HPP

#include "./server/Server.hpp"

#include "./http/Request.hpp"
#include "./http/Response.hpp"

#include "./config/ConfigParser.hpp"

#include "./network/Connection.hpp"
#include "./network/Poller.hpp"
#include "./network/Sockets.hpp"

#include "./main/utils.hpp"

#include "../srcs/CGI/CGI_Handler.hpp"

struct Webserv
{
	std::string	config_path;
	std::map<int, std::string> status_code_info;
	std::map<std::string, std::string> content_types;

	std::vector<Response*> responses;
	~Webserv() {
		for (Response* response : responses) {
			delete response;
		}
		responses.clear();
	}
};

#endif
