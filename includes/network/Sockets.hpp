#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <poll.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

class Sockets
{
	private:
		static const int	BACKLOG = 100;

		unsigned short	convertStrToUShort(const std::string& s);
		struct in_addr	convertStrIpToBinIP(std::string& ip_str,
							const std::string& server_name,
							const std::string& port);

		void	setNonblockMode(int fd, const std::string& server_name);
		void	bindSocket(int sock_fd, const std::string& server_name,
					serverConfig& server);
	public:
		std::vector<int>	server_fds;
		void	initSockets(std::vector<serverConfig>& servers);
};

void	error_exit(const std::string& msg, const std::string& server_name);

#endif
