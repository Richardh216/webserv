#ifndef RESPONSE_HPP
#define RESPONSE_HPP

struct HttpRequest;

class Response
{
	private:
		std::string http_version;
		std::string status_code;
		std::string reason_phrase;

		std::map<std::string, std::string>  headers;
		std::string body = "";

		void	addHeader(const std::string& name, const std::string& value);

		void	addBody(const std::string& file_path);

		struct Route	findRouteInConfig(const std::string& request_path,
				const serverConfig& server);

		std::string	findFullPath(const std::string& request_path,
				const serverConfig& server,
				const struct Route& correct_route);

		void	prepareBody(std::string& full_path,
				const struct Route& correct_route,
				const serverConfig& server,
				const std::map<int, std::string>& status_code_info);

		void	formError(int code, const serverConfig& server,
			const std::string& error_message);
	public:
		void	testInitRequest(HttpRequest& request);

		std::string findHeaderValue(const std::string& name,
			const std::map<std::string, std::string>& headers) const;

		const serverConfig&	chooseServer(const HttpRequest& request,
			const std::vector<serverConfig>& servers) const;

		void	formResponse(const HttpRequest& request, const serverConfig& server,
			const std::map<int, std::string>& status_code_info);

		void	sendResponse(int socket_fd);
};

#endif
