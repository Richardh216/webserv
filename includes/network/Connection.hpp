#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <unistd.h>

class Poller;

class Connection
{
	private:
	public:
		bool	isServerFd(int fd, const std::vector<int>& server_fds);
		void	handleServerFd(int fd, Poller& poller);
};

#endif
