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
				error_exit("failed to call accept", "");
			break;
		}

		poller.poll_fds[poller.nfds].fd = client_fd;
		poller.poll_fds[poller.nfds].events = POLLIN;
		poller.nfds++;
	}
}

void	Connection::handleClientFd(struct pollfd& pollfd)
{
	constexpr size_t BUFFER_SIZE = 4096;
    std::vector<char> buff(BUFFER_SIZE);
    ssize_t bytesRead;

	for (;;) {
		bytesRead = recv(pollfd.fd, buff.data(), sizeof(buff), 0);
		if (bytesRead < 0)
			break;
		else if (bytesRead == 0) {
			//delete 'pollfd' from the 'poll_fds' array
			close(pollfd.fd);
			pollfd.fd = -1;
			break;
		}
		else
			std::cout.write(buff.data(), bytesRead);
		break;
	}
}
