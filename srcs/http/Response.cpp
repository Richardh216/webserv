#include "../../includes/webserv.hpp"

void	Response::testInitRequest(HttpRequest& request)
{
	request.method = "GET";
	request.path = "/";
	request.httpVersion = "HTTP/1.1";

	request.headers.insert(std::pair("Host", "localhost"));
	request.headers.insert(std::pair("From", "someuser@jmarshall.com"));
	request.headers.insert(std::pair("User-Agent", "HTTPTool/1.1"));

	request.body = "";

	request.isValid = true;
}

std::string	Response::findHeaderValue(const std::string& name,
	const std::map<std::string, std::string>& headers) const
{
	const auto& it = headers.find(name);

	std::string value = "";
	if (it != headers.end())
		value = it->second;

	return value;
}

void	Response::chooseServer(int fd, const HttpRequest& request,
	std::vector<serverConfig>& servers)
{
	struct sockaddr_in client_address;
	socklen_t		add_len = sizeof(client_address);
	int res = getsockname(fd, (struct sockaddr*)&client_address, &add_len);
	if (res == -1)
		return;

	serverConfig* first_match = nullptr;
	std::string value = findHeaderValue("Host", request.headers);
	for (auto& server : servers) {
		if (client_address.sin_port == server.bind_addr.sin_port
			&& (client_address.sin_addr.s_addr == server.bind_addr.sin_addr.s_addr
				|| server.bind_addr.sin_addr.s_addr == 0))
		{
			if (!first_match)
				first_match = &server;
			for (const auto& name : server.serverNames ) {
				if (name == value) {
					this->choosed_server = &server;
					return;
				}
			}
		}
	}
	this->choosed_server = first_match;
}

void	Response::addHeader(const std::string& name, const std::string& value)
{
	headers.insert(std::pair<std::string, std::string>(name, value));
}

void	Response::formResponse(const HttpRequest& request, const Webserv& webserv)
{
	if (this->choosed_server == nullptr) return;

	this->http_version = "HTTP/1.1";
	addHeader("Date", takeGMTTime());
	addHeader("Server", "webserv/0.01");
	addHeader("Connection", "close");

	findRouteInConfig(request.path);
	std::string full_path = findFullPath(request.path);
	if (full_path.empty() || !std::filesystem::exists(full_path)) {
		formError(404, webserv.status_code_info.at(404));
	}
	else {
		if (request.method == "GET") {
			handleGET(full_path, webserv);
		}
		else if (request.method == "POST") {
		}
		else if (request.method == "DELETE") {
		}
		else {
			formError(405, webserv.status_code_info.at(405));
		}
	}
}

void	Response::findRouteInConfig(const std::string& request_path)
{
	for (auto& route: choosed_server->routes) {
		if (request_path.compare(0, route.path.size(), route.path) == 0) {
			if (choosed_route == nullptr
				|| (route.path.size() > choosed_route->path.size()))
				choosed_route = &route;
		}
	}
}

std::string Response::findFullPath(const std::string& request_path)
{
	//join the request path with the root or alias path
	std::string full_path = "";
	if (!choosed_server->root.empty())
		full_path = choosed_server->root + request_path;
	if (choosed_route && !choosed_route->root.empty())
		full_path = choosed_route->root + request_path;
	else if (choosed_route && !choosed_route->alias.empty())
		full_path = choosed_route->alias + request_path.substr(choosed_route->path.size()); // size+1 ?

	return full_path;
}

void	Response::handleGET(std::string& full_path, const Webserv& webserv)
{
	if (std::filesystem::is_directory(full_path))
		handleDirRequest(full_path, webserv);
	else if (std::filesystem::is_regular_file(full_path))
		serveFile(full_path, webserv);
	else
		formError(403, webserv.status_code_info.at(403));
}

void	Response::handleDirRequest(std::string& full_path, const Webserv& webserv)
{
	std::string index_file = "index.html";
	if (choosed_route && !choosed_route->indexFile.empty())
		index_file = choosed_route->indexFile;

	std::string tmp = full_path + "/" + index_file;
	if (std::filesystem::exists(tmp)
		&& std::filesystem::is_regular_file(tmp)) {
		serveFile(tmp, webserv);
	}
	else {
		if (choosed_route && choosed_route->autoindex) {
			//this->body = generateAutoindexHTML(full_path);
		}
		else {
			formError(403, webserv.status_code_info.at(403));
		}
	}
}

void	Response::serveFile(const std::string& full_path, const Webserv& webserv)
{
	if (addBody(full_path, true)) {
		this->status_code = "200";
		this->reason_phrase = webserv.status_code_info.at(200);
		addHeader("Content-Length", std::to_string(this->body.size()));
		//choose file extension ?
	}
	else {
		formError(404, webserv.status_code_info.at(404));
	}
}

void	Response::formError(int code, const std::string& error_message)
{
	std::string error_file_path = "./website/errors/" + std::to_string(code) + ".html";

	if (choosed_server) {
		const auto& it = choosed_server->errorPages.find(code);
		if (it != choosed_server->errorPages.end()) {
			if (!choosed_server->root.empty()) {
				std::string tmp = choosed_server->root + it->second;
				if (std::filesystem::exists(tmp)
					&& std::filesystem::is_regular_file(tmp))
					error_file_path = tmp;
			}
		}
	}

	this->status_code = std::to_string(code);
	this->reason_phrase = error_message;
	if (!addBody(error_file_path, true))
		this->body = "<html><body><h1>" + status_code + " "
			+ reason_phrase + "</h1></body></html>";
	addHeader("Content-Type", "text/html");
	//or search an extension of a file and add
	addHeader("Content-Length", std::to_string(this->body.size()));
}

bool	Response::addBody(const std::string& full_path, bool is_bin)
{
	std::ifstream file(full_path, (is_bin ? std::ios::binary : std::ios::in));

	if (file) {
		std::stringstream buffer;
		buffer << file.rdbuf();
		this->body = buffer.str();
		//this->body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return true;
	}

	return false;
}

void	Response::sendResponse(int socket_fd)
{
	std::string response;

	if (this->choosed_server == nullptr) return;

	response.reserve(1024);
	response = http_version + " " + status_code + " "
		+ reason_phrase + "\r\n";

	for (const auto&[key, value] : headers) {
		response += key + ": " + value + "\r\n";
	}

	response += "\r\n";
	response += body;

	ssize_t bytes_sent = send(socket_fd, response.c_str(), response.size(), 0);
	if (bytes_sent == -1) {
		std::cerr << "failed to send a http request" << std::endl;
	}
}

std::string Response::takeGMTTime()
{
	std::time_t t = std::time(nullptr);
	const char format[] = "%a, %d %b %Y %H:%M:%S GMT";
	std::vector<char> buff(100);

	std::size_t size = std::strftime(buff.data(), buff.size(), format, std::gmtime(&t));

	return (size > 0) ? std::string(buff.data()) : "failed to check";
}

std::string	Response::checkContentType(std::string file, const Webserv& webserv)
{
	/**
	 * file : ./website/inages/favicon.png
	 */
	(void)file;
	(void)webserv;

	return "";
}
