#include "../../includes/webserv.hpp"

// check for error codes in nginx
// check if / is needed at the end of paths

//1. redir code and redir path should be 2 vars
//2 check autoindex if it's working correctly // should work
//3 doesn't print root var from struct Route // has nothing to print
//4 add alias feature for struct Route only // done
//5 nginx behavior for duplicate directives in the same location or server section in config file


std::string	ConfigParser::trim(const std::string &str) {
	size_t	first = str.find_first_not_of(" \t"); //first not whitespace
	if (first == std::string::npos) return "";
	size_t	last = str.find_last_not_of(" \t"); //last not whitespace
	return str.substr(first, last - first + 1); //actual content
}

std::string	ConfigParser::getValue(const std::string &line) {
	size_t	pos = line.find(" ");
	if (pos == std::string::npos) return "";

	std::string	value = trim(line.substr(pos + 1)); //extract val

	size_t	commentPos = value.find('#'); //ignore everything after #
	if (commentPos != std::string::npos) {
		value = trim(value.substr(0, commentPos));
	}

	if (!value.empty() && value.back() == ';') { //remove the ; at the end
		value.pop_back();
	}
	return value;
}

std::vector<std::string>	ConfigParser::split(const std::string &str, char delim) {
	std::vector<std::string>	tokens; //store split parts
	std::stringstream			ss(str); // convert input str into stram
	std::string					token; // tmp var for each token

	while (std::getline(ss, token, delim)) { //read up to each delim
		if (!token.empty())
			tokens.push_back(trim(token)); //trim spaces and add to tokens
	}
	return tokens;
}

std::string	ConfigParser::getLocationPath(const std::string &line) {
	size_t	start = line.find(" ");
	size_t	end = line.find("{");
	if (start == std::string::npos || end == std::string::npos || start >= end)
		return "";
	std::string	path = line.substr(start, end - start);
	return trim(path);
}

int	ConfigParser::parseSize(const std::string &sizeStr) {
	if (sizeStr.back() == 'M') return std::stoi(sizeStr.substr(0, sizeStr.size() - 1)) * 1024 * 1024;
	if (sizeStr.back() == 'K') return std::stoi(sizeStr.substr(0, sizeStr.size() - 1)) * 1024;
	return std::stoi(sizeStr);
}

void	ConfigParser::getPortHost(const std::string &line, serverConfig &config) {
	size_t	pos = line.find(':');
	if (pos != std::string::npos) {
		config.port = line.substr(pos + 1);
		config.host = line.substr(0, pos);
	}
	else {
		config.port = line; //if only one, it makes it the port, could be error prone
	}
	//else default case?
}

void	ConfigParser::parseConfigFile(const std::string &filename) { //builds list of serverConfig each contains a list of Route
	std::ifstream	file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening config file: " << filename << std::endl;
		return;
	}

	std::string		line;
	serverConfig	currentServer;
	Route			currentRoute;
	bool	insideServer = false;
	bool	insideRoute = false;

	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#') continue; //skip comments and empty lines

		if (line == "server {") {
			insideServer = true;
			currentServer = serverConfig();
			continue;
		}

		if (line == "}") {
			if (insideRoute) {
				currentServer.routes.push_back(currentRoute); //adds current route to list
				insideRoute = false;
			} else if (insideServer) {
				servers.push_back(currentServer); //adds current server to the list
				insideServer = false;
			}
			continue;
		}

		if (insideServer) {
			if (line.find("listen") == 0) {
				getPortHost(getValue(line), currentServer);
			} else if (line.find("server_name") == 0) {
				currentServer.serverNames = split(getValue(line), ' ');
			} else if (line.find("root") == 0) {
				currentServer.root = trim(getValue(line));
			} else if (line.find("error_page") == 0) {
				std::vector<std::string>	parts = split(getValue(line), ' ');
				if (parts.size() == 2) {
					currentServer.errorPages[std::stoi(parts[0])] = parts[1]; //0 is error code like 404, 1 is the path to the html
				}
			} else if (line.find("client_max_body_size") == 0) {
				currentServer.client_max_body_size = parseSize(getValue(line));
			} else if (line.find("location") == 0) {
				if (insideRoute) {
					currentServer.routes.push_back(currentRoute);
				}
				insideRoute = true;
				currentRoute = Route();
				currentRoute.path = getLocationPath(line);
			} else if (insideRoute) {
				if (line.find("root") == 0) { //dir serving files
					currentRoute.root = trim(getValue(line));
				} else if (line.find("index") == 0) { //default file
					currentRoute.indexFile = trim(getValue(line));
				} else if (line.find("limit_except") == 0) { //list of allowed http methods (limit_except)
					currentRoute.allowedMethods = split(getValue(line), ' ');
				} else if (line.find("autoindex") == 0) {  //dir listing
					currentRoute.autoindex = getValue(line) == "on";
				} else if (line.find("return") == 0) { //redir
					std::vector<std::string>	partsR = split(getValue(line), ' ');
					if (partsR.size() == 2) {
						currentRoute.redirection[std::stoi(partsR[0])] = partsR[1];
					}
				} else if (line.find("cgi_extension") == 0) {
					currentRoute.cgiExtension = trim(getValue(line));
				} else if (line.find("cgi_path") == 0) {
					currentRoute.cgiPath = trim(getValue(line));
				} else if (line.find("upload_enable") == 0) {
					currentRoute.uploadEnabled = getValue(line) == "on"; //If value == "on", then currentRoute.autoindex = true, vice versa
				} else if (line.find("upload_path") == 0) { //dir for file uploads
					currentRoute.uploadPath = trim(getValue(line));
				} else if (line.find("alias") == 0) {
					currentRoute.alias = trim(getValue(line));
				}
			}
		}
	}
	file.close();
}

void	ConfigParser::tester(const std::string &inFile) {
	ConfigParser parser;
	parser.parseConfigFile(inFile);

	for (const auto &server : parser.servers) {
		std::cout << "\nServer on " << server.host << ":" << server.port << "\n";

		if (!server.serverNames.empty()) {
			std::cout << " Server Names: ";
			for (const auto &name : server.serverNames) {
				std::cout << name << " ";
			}
			std::cout << "\n";
		}

		std::cout << " Server Root: " << server.root << "\n";

		if (!server.errorPages.empty()) {
			std::cout << " Error Pages:\n";
			for (const auto &errorPage : server.errorPages) {
				std::cout << "  " << errorPage.first << " -> " << errorPage.second << "\n";
			}
		}

		std::cout << " Client Max Body Size: " << server.client_max_body_size << " bytes\n";

		std::cout << " Routes:\n";
		for (const auto &route : server.routes) {
			std::cout << "  - Path: " << route.path << "\n";
			if (!route.root.empty()) std::cout << "    Root: " << route.root << "\n";
			if (!route.indexFile.empty()) std::cout << "    Index File: " << route.indexFile << "\n";
			if (!route.allowedMethods.empty()) {
				std::cout << "    Allowed Methods (limit_except): ";
				for (const auto &method : route.allowedMethods) std::cout << method << " ";
				std::cout << "\n";
			}
			if (!route.alias.empty()) std::cout << "    alias: " << route.alias << "\n";
			if (!route.redirection.empty()) {
			std::cout << " Redirection:\n";
			for (const auto &redir : route.redirection) {
				std::cout << "  " << redir.first << " -> " << redir.second << "\n";
			}
		}
			if (!route.cgiExtension.empty()) std::cout << "    CGI Extension: " << route.cgiExtension << "\n";
			if (!route.cgiPath.empty()) std::cout << "    CGI Path: " << route.cgiPath << "\n";
			std::cout << "    Autoindex: " << (route.autoindex ? "On" : "Off") << "\n";
			if (route.uploadEnabled) std::cout << "    Upload Enabled: On\n";
			if (!route.uploadPath.empty()) std::cout << "    Upload Path: " << route.uploadPath << "\n";
		}
	}

}