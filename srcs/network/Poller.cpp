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
		error_exit("failed to invoke poll function", "");
}
