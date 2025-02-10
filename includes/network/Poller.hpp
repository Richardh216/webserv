#ifndef POLLER_HPP
#define POLLER_HPP

#include <poll.h>

class Poller
{
	public:

		static constexpr int MAX_FDS_POLL = 500;
		static constexpr int TIMEOUT = -1;
		int nfds = 0;
		struct pollfd poll_fds[MAX_FDS_POLL];

		void	initPoll(std::vector<int> &server_fds);
		void	processPoll();
};

#endif
