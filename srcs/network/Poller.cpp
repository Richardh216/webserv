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

void	Poller::processPoll(int curr_nfds)
{
	if (poll(poll_fds, curr_nfds, TIMEOUT) == -1)
		std::cerr << "poll() failed: " << strerror(errno) << std::endl;
}

void	Poller::removeFd(int fd_index, int curr_nfds)
{
	if (fd_index < 0 || fd_index >= curr_nfds) return;
	close(poll_fds[fd_index].fd);
	poll_fds[fd_index].fd = -1;
}

bool	Poller::isFdBad(int fd_index)
{
	return poll_fds[fd_index].revents & (POLLERR | POLLHUP);
}

bool	Poller::isFdReadable(int fd_index)
{
	return poll_fds[fd_index].revents & POLLIN;
}

bool	Poller::isFdWriteable(int fd_index)
{
	return poll_fds[fd_index].revents & POLLOUT;
}

void	Poller::addWriteEvent(int fd_index) {
	poll_fds[fd_index].events |= POLLOUT;
}

void	Poller::removeWriteEvent(int fd_index) {
	poll_fds[fd_index].events &= ~POLLOUT;
}

void	Poller::compressFdArr()
{
	int new_nfds = 0;

	for (int i = 0; i < nfds; ++i) {
		if (poll_fds[i].fd != -1) {
			poll_fds[new_nfds] = poll_fds[i];
			new_nfds++;
		}
	}
	nfds = new_nfds;
}
