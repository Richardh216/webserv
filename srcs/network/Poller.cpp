#include "../../includes/webserv.hpp"

/**
 * int poll(struct pollfd *fds, nfds_t nfds, int timeout);
 * 
 * struct pollfd {
 *		int   fd;         * file descriptor *
 *		short events;     * requested events *
 *		short revents;    * returned events *
 * };
 */

void	Poller::initPoll(std::vector<int> &server_fds)
{
	for (int socket: server_fds) {

		if (nfds >= MAX_FDS_POLL)
			break;

		std::memset(&poll_fds[nfds], 0, sizeof(poll_fds[nfds]));
		poll_fds[nfds].fd = socket;
		poll_fds[nfds].events = POLLIN;

		nfds++;
	}
}

void	Poller::processPoll()
{
	if (poll(poll_fds, nfds, TIMEOUT) == -1)
		std::cerr << "poll() failed: " << strerror(errno) << std::endl;
}

void	Poller::removeFd(int& fd_index)
{
	if (fd_index < 0 || fd_index >= nfds) return;

	close(poll_fds[fd_index].fd);

	for (int i = fd_index; i < nfds - 1; ++i) {
		poll_fds[i] = poll_fds[i + 1];
	}
	nfds--;
	fd_index--;
}

bool	Poller::skipFd(bool is_server, int& fd_index)
{
	(void)is_server;
	if (poll_fds[fd_index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
		//if (!is_server) removeFd(fd_index); ???
		return true;
	}
	if (!(poll_fds[fd_index].revents & POLLIN)) {
		//if (!is_server) removeFd(fd_index); ???
		return true;
	}

	return false;
}
