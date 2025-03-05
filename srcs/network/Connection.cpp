#include "../../includes/webserv.hpp"

bool	Connection::isServerFd(int fd, const std::vector<int>& server_fds)
{
	const auto& it = std::find(server_fds.begin(), server_fds.end(), fd);

	return it != server_fds.end();
}

void	Connection::handleServerFd(int fd, Poller& poller)
{
	for (;;) {

		if (poller.nfds >= Poller::MAX_FDS_POLL)
			break;

		int client_fd = accept(fd, nullptr, nullptr);

		if (client_fd == -1) {
			// EAGAIN or EWOULDBLOCK -> no pending connections are present
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				std::cerr << "accept() failed: " << strerror(errno) << std::endl;
			break;
		}

		poller.poll_fds[poller.nfds].fd = client_fd;
		poller.poll_fds[poller.nfds].events = POLLIN;
		poller.nfds++;
	}
}
