#include "../../includes/webserv.hpp"

unsigned short	Sockets::convertStrToUShort(const std::string& s)
{
	unsigned short	nbr;
	unsigned long	temp = std::stoul(s);

	if (temp > std::numeric_limits<unsigned short>::max())
		error_exit("failed to convert std::string to unsigned short nbr", "");

	nbr = static_cast<unsigned short>(temp);

	return nbr;
}

void	error_exit(const std::string& msg, const std::string& server_name)
{
	std::cerr << msg;
    if (!server_name.empty())
        std::cerr << " (server name: " << server_name << ")";
    std::cerr << std::endl;
    std::exit(EXIT_FAILURE);
}

void	Sockets::setNonblockMode(int fd, const std::string& server_name)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		error_exit("failed to SET the O_NONBLOCK flag for a socket", server_name);
}

struct in_addr	Sockets::convertStrIpToBinIP(std::string& ip_str,
	const std::string& server_name, const std::string& port)
{
	struct addrinfo hints, *res = nullptr;
	std::memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;

	int status = getaddrinfo(ip_str.c_str(), port.c_str(), &hints, &res);
	if (status != 0) {
		error_exit(std::string("getaddrinfo error: ") + gai_strerror(status), server_name);
	}

	struct in_addr bin_addr = ((struct sockaddr_in*)(res->ai_addr))->sin_addr;

	freeaddrinfo(res);
	return bin_addr;
}

void	Sockets::bindSocket(int sock_fd, const std::string& server_name,
	serverConfig& server)
{
	if (bind(sock_fd, (struct sockaddr *)&server.bind_addr, sizeof(server.bind_addr)) == -1)
		error_exit("failed to bind a socket", server_name);
}

void	Sockets::initSockets(std::vector<serverConfig>& servers)
{
	std::vector<std::string> to_bind;

	//init server.host and server.bind_addr
	for (auto& server : servers) {
		if (server.host.empty())
			server.host = "0.0.0.0";

		std::string server_name = "";
		if (!server.serverNames.empty())
			server_name = server.serverNames.front();
		std::memset(&server.bind_addr, 0, sizeof(server.bind_addr));
		server.bind_addr.sin_family = AF_INET;
		server.bind_addr.sin_port = htons(convertStrToUShort(server.port));
		server.bind_addr.sin_addr = convertStrIpToBinIP(server.host, server_name, server.port);
	}

	for (auto& server : servers) {

		//skip the same host:port endpoints
		std::string endpoint = server.host + ":" + server.port;
		if (std::find(to_bind.begin(), to_bind.end(), endpoint) != to_bind.end()) continue;
		else to_bind.push_back(endpoint);

		std::string server_name = "";
		if (!server.serverNames.empty())
			server_name = server.serverNames.front();

		/*create a socket*/
		int	fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
			error_exit("failed to create a socket", server_name);

		/**
		 * allow the socket to be reuseable
		 */
		int	on = 1;
		int res = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (res == -1)
			error_exit("failed to set the SO_REUSEADDR option for a socket", server_name);

		/**
		 * set the socket to be nonblocking;
		 * 
		 * all of the sockets for the incoming connections
		 * are also nonblocking because they inherit that state
		 * from the listening socket - is it correct ?
		 */
		setNonblockMode(fd, server_name);

		/**
		 * bind the socket
		 */
		bindSocket(fd, server_name, server);

		/**
		 * set the listen back log
		 */
		if (listen(fd, BACKLOG) == -1)
			error_exit("failed to listen on socket", server_name);

		/**
		 * save the socket
		 */
		server_fds.push_back(fd);
	}
}
