#include "../../includes/webserv.hpp"

//void	printServers(std::vector<serverConfig>& servers)
//{

//	for (const auto& i : servers) {
//		std::cout << "\n\n\n\n";
//		std::cout << "port: " << i.port << std::endl;
//		std::cout << "host: " << i.host << std::endl;
//		std::cout << "root: " << i.root << std::endl;
//		for (const auto& y : i.serverNames ) {
//			std::cout << "server_name: "<< y << std::endl;
//		}
//		for (const auto& [key, value] : i.errorPages) {
//			std::cout << "error: " << key << "; " << "path: " << value << std::endl;
//		}
//		std::cout << "client_max_body_size: " << i.client_max_body_size << std::endl;
//		std::cout << "\n\nroutes: \n";
//		for (const auto& z : i.routes) {
//			std::cout << "\n";
//			std::cout << "path: " << z.path << std::endl;
//			std::cout << "root: " << z.root << std::endl;
//			for (const auto& p : z.allowedMethods ) {
//				std::cout << "allowedmethod: "<< p << std::endl;
//			}
//			std::cout << "autoindex: " << z.autoindex << std::endl;
//			std::cout << "indexFile: " << z.indexFile << std::endl;
//			//std::cout << "redirection: " << z.redirection << std::endl;
//			std::cout << "cgiExtension: " << z.cgiExtension << std::endl;
//			std::cout << "cgiPath: " << z.cgiPath << std::endl;
//			std::cout << "uploadEnabled: " << z.uploadEnabled << std::endl;
//			std::cout << "uploadPath: " << z.uploadPath << std::endl;
//			std::cout << "\n";
//		}
//	}
//}

//void	setupSockets(std::vector<serverConfig>& servers)
//{
//	printServers(servers);
//	std::vector<int>	socket_fds;

//	for (const auto& server : servers) {
//		int	fd = socket(AF_INET, SOCK_STREAM, 0);
//		if (fd == -1) {
//			std::cerr << "failed to create a socket for the ";
//		}
//	}

//	//int	server_socket = socket(AF_INET, SOCK_STREAM, 0);
//}
