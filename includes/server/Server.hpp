#ifndef	SERVER_HPP
#define	SERVER_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

class Location;

class Server
{
	private:
		// If a characteristic is not defined, the std::string will be an empty string

		int			port;	//	|
		std::string	IP;		// 	|-> listen 127.0.0.1:80;

		std::string	server_name; // server_name example.com;

		/**
		 * Global root directory for the server
		 * 
		 * root /path/to/your/root;
		 */
		std::string	root;

		/**
		 * The first server for a host:port will be the default
		 * for this host:port (subject pdf)
		 */
		bool		is_default;

		/**
		 * map:
		 * 404 -> "./htmls/404.html"
		 * 500 -> "./htmls/500.html"
		 * ...
		 * 
		 * error_page 404 /errors/404.html;
		 * error_page 500 502 503 504 /errors/500.html; (nginx)
		 * 
		 */
		std::map<int, std::string>	error_pages;

		// routing rules based on the requested URL (see NGINX)
		// each location should be unique so maybe we should use std:set
		std::vector<Location>		locations;

		/**
		 * The size can be specified in bytes, kilobytes (2K), or megabytes (2M). (?)
		 * 
		 * If the client tries to send a body larger than this limit,
		 * the server should respond with a 413 Payload Too Large status.
		 */
		int	client_max_body_size;

	public:
};

#endif
