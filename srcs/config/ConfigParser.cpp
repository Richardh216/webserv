#include "../../includes/webserv.hpp"

/*
TODO: 

1. double check error handling and in general handling of client_max_body_size

*/

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

int	ConfigParser::parseSize(const std::string &sizeStr) { //discuss error handling
	long	tmpVal;
	if (!isdigit(sizeStr.back()) && (sizeStr.back() != 'M' && sizeStr.back() != 'm' && sizeStr.back() != 'k' && sizeStr.back() != 'K'))
		return -1; //use default value if it's incoherent
	if (sizeStr.size() == 1) {
		tmpVal = std::stol(sizeStr.substr(0, sizeStr.size()));
	} else {
		tmpVal = std::stol(sizeStr.substr(0, sizeStr.size() - 1));
	}
	int		val = 0;
	if (tmpVal < INT_MIN) {
		return 0; //negative out of bounds, will handle it as no body allowed
	} else if (tmpVal > INT_MAX) {
		return -1; //positive out of bounds, will handle it as default value
	} else {
		val = static_cast<int>(tmpVal);
	}
	if (val == -1) {
		return -1; // default value
	} else if (val < -1) {
		return 0; //no body allowed
	}
	if (sizeStr.back() == 'M' || sizeStr.back() == 'm') return val * 1024 * 1024;
	if (sizeStr.back() == 'K' || sizeStr.back() == 'k') return val * 1024;
	return std::stoi(sizeStr);
}

void	ConfigParser::getPortHost(const std::string &line, serverConfig &config) {
	size_t	pos = line.find(':');
	if (pos != std::string::npos) {
		config.port = line.substr(pos + 1); //old
		config.host = line.substr(0, pos); //old
		// config.host.push_back(line.substr(0, pos)); //new for vector
		// config.port.push_back(line.substr(pos + 1));
	}
	else {
		config.port = line; //if only one, it makes it the port, could be error prone //old
		config.host = "0.0.0.0";
		// config.host.push_back("0.0.0.0");  // Default host if none is provided
		// config.port.push_back(line); //if no colon treat line as port
	}
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
			} else if (line.find("root") == 0 && !insideRoute) {
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
			} else if (!insideRoute) {
				throw std::runtime_error("Incoherent values in Server in Config File: " + line);
			} else if (insideRoute) {
				if (line.find("root") == 0) { //dir serving files
					if (!currentRoute.root.empty()) {
						throw std::runtime_error("Location path " + currentRoute.path + " has multiple root directives.");
					}
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
						currentRoute.redirection.clear();
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
					if (!currentRoute.alias.empty()) {
						throw std::runtime_error("Location path " + currentRoute.path + " has multiple alias directives.");
					}
					currentRoute.alias = trim(getValue(line));
				} else { // check inside route
					throw std::runtime_error("Incoherent values in Route in Config File: " + line);
				}
			}
		} else { //check for outside of server & root values
			throw std::runtime_error("Incoherent values outside of Server in Config File: " + line);
		}
	}
	file.close();
}


void	ConfigParser::checkDuplicateLocationPath(void) { //check for duplicate location paths
	for (auto &server : servers) {
		std::set<std::string>	paths;
		for (auto &route : server.routes) {
			if (paths.find(route.path) != paths.end()) {
				throw std::runtime_error("Duplicate Location Path Found: " + route.path);
			}
			paths.insert(route.path);
		}
	}
}

//new, just cheks name
void	ConfigParser::checkDuplicateServer(void) {
	std::set<std::string>	seenNames;

	for (size_t i = 0; i < servers.size(); i++) {
		for (const std::string &name : servers[i].serverNames) {
			if (seenNames.find(name) != seenNames.end()) {
				throw std::runtime_error("Duplicate Server Configuration Found: Same server name on multiple Servers!");
			}
			seenNames.insert(name);
		}
	}
}

void	ConfigParser::checkRootAlias(void) { //some checks happen in parsing before
	for (auto &server : servers) {
		for (auto & route : server.routes) {
			int	rootCount = (route.root.empty() ? 0 : 1); //1 if root exists, 0 if not
			int	aliasCount = (route.alias.empty() ? 0 : 1); //1 if alias exists, 0 if not

			//check for both root and alias in same location
			if (rootCount > 0 && aliasCount > 0) {
				throw std::runtime_error("Location path " + route.path + " cannot have both root and alias directives.");
			}

			//check for duplicate Server root
			if (rootCount > 1) {
				throw std::runtime_error("Location path " + route.path + " contains multiple root directives.");
			}

			//check for duplicate Server alias
			if (aliasCount > 1) {
				throw std::runtime_error("Location path " + route.path + " contains multiple alias directives.");
			}
		}
	}
}

void	ConfigParser::checkErrorPagesPath(void) { //ensures error_page paths start with '/'
	for (auto & server : servers) {
		for (auto & errorPage : server.errorPages) {
			if (errorPage.second[0] != '/') {
				throw std::runtime_error("Error page path must start with '/': " + errorPage.second);
			}
		}
	}
}

void	ConfigParser::removeInvalidLocationPath() { //creates a new list of routes and only adds valid ones
	for (auto &server : servers) {
		auto &routes = server.routes;

		std::vector<Route> validRoutes;

		//get valid routes
		for (const auto &route : routes) {
			if (!route.path.empty() && route.path[0] == '/') {
				validRoutes.push_back(route);
			}
		}

		//replace old routes with just the valid ones
		routes = std::move(validRoutes); //more efficient than routes = validRoutes
	}
}

//play around with config file to test!
void	ConfigParser::checkingFunction(void) {
	checkDuplicateServer(); //name check
	checkDuplicateLocationPath();
	checkRootAlias();
	checkErrorPagesPath();
	removeInvalidLocationPath();
}

void	ConfigParser::tester(const std::string &inFile) {
	ConfigParser parser;
	parser.parseConfigFile(inFile);
	parser.checkingFunction();

	for (const auto &server : parser.servers) {
		std::cout << "\nServer on " << server.host << ":" << server.port << "\n"; //single print
		// for (size_t i = 0; i < server.host.size(); ++i) { //only looping list of hosts now
		// 	std::cout << "  - " << server.host[i] << ":" << server.port[i] << "\n";
		// }

		if (!server.serverNames.empty()) {
			std::cout << " Server Name: ";
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
			if (!route.cgiExtension.empty()) std::cout << "    CGI Extension: " << route.cgiExtension << "\n";
			if (!route.cgiPath.empty()) std::cout << "    CGI Path: " << route.cgiPath << "\n";
			// std::cout << "    Autoindex: " << (route.autoindex ? "On" : "Off") << "\n"; //print even if not set
			std::cout << "    Autoindex: " << (route.autoindex ? "On" : "Off") << "\n";
			std::cout << "    Upload Enable: " << (route.uploadEnabled ? "On" : "Off") << "\n";
			if (!route.uploadPath.empty()) std::cout << "    Upload Path: " << route.uploadPath << "\n";
			if (!route.redirection.empty()) {
				std::cout << " Redirection:\n";
				for (const auto &redir : route.redirection) {
					std::cout << "  " << redir.first << " -> " << redir.second << "\n\n";
				}
			}
		}
	}
}
