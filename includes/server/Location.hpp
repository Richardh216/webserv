#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

/**
 * routing rules based on the requested URL (see NGINX)
 */

class Location
{
	/**
	 * location /htmls/errors {}
	 * 
	 * /htmls/errors -> route
	 * 
	 */
	private:
		bool		is_internal; // (?)
		std::string	path;
		std::string	root;
		std::string alias;

		/**
		 * "Define a list of accepted HTTP methods for the route"
		 * 
		 * limit_except GET POST DELETE;
		 * 
		 * (?)
		 * bool	can_get;	|
		 * bool	can_post;	|
		 * bool	can_delete;	|-> by default all false
		 * 
		 */

		/**
		 * "Define a HTTP redirection"
		 * 
		 * return 301 /new_path;
		 */
		bool		is_redir;
		int			status_code;
		std::string	new_path;

		/**
		 * Turn on or off directory listing
		 * 
		 * autoindex on;
		 * autoindex off;
		 */
		bool		autoindex;

		/**
		 * Set a default file to answer if the request is a directory
		 * 
		 * index welcome.html;
		 * 
		 */
		std::string	index;

		/**
		 * 
		 * ◦ Execute CGI based on certain file extension (for example .php).
		 * ◦ Make it work with POST and GET methods.
		 * ◦ Make the route able to accept uploaded files and configure where they should be saved.
		 */

	public:
};

#endif
