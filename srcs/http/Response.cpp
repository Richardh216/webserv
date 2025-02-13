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

const serverConfig&	Response::chooseServer(const HttpRequest& request,
	const std::vector<serverConfig>& servers) const
{
	std::string value = findHeaderValue("Host", request.headers);

	for (const auto& server : servers) {
		for (const auto& name : server.serverNames ) {
			if (name == value)
				return server;
		}
	}

	return servers.front();
}

void	Response::addHeader(const std::string& name,
	const std::string& value)
{
	headers.insert(std::pair<std::string, std::string>(name, value));
}

void	Response::formResponse(const HttpRequest& request,
	const serverConfig& server, const std::map<int, std::string>& status_code_info)
{
	this->http_version = "HTTP/1.1";
	//addHeader("Date", "");
	addHeader("Server", "webserv/0.01");
	addHeader("Content-Type", "text/html");
	addHeader("Connection", "close");

	struct Route correct_route = findRouteInConfig(request.path, server);
	//what if there will be '//' in the path ? 
	std::string full_path = findFullPath(request.path, server, correct_route);
	if (full_path.empty() || !std::filesystem::exists(full_path)) {
		formError(404, server, status_code_info.at(404));
	}
	else {
		if (request.method == "GET") {
			prepareBody(full_path, correct_route, server, status_code_info);
		}
		else if (request.method == "POST") {
		}
		else if (request.method == "DELETE") {
		}
		else {
			formError(405, server, status_code_info.at(405));
		}
	}
}

struct Route	Response::findRouteInConfig(const std::string& request_path,
	const serverConfig& server)
{
	struct Route correct_route;
	for (const auto& route: server.routes) {
		if (request_path.compare(0, route.path.size(), route.path) == 0) {
			if (route.path.size() > correct_route.path.size())
				correct_route = route;
		}
	}

	return correct_route;
}

std::string Response::findFullPath(const std::string& request_path,
	const serverConfig& server, const struct Route& correct_route)
{
	//join the request path with the root or alias path
	std::string full_path = "";
	if (!server.root.empty())
		full_path = server.root + request_path;
	if (!correct_route.root.empty())
		full_path = correct_route.root + request_path;
	else if (!correct_route.alias.empty())
		full_path = correct_route.alias + request_path.substr(correct_route.path.size()); // size+1 ?

	return full_path;
}

void	Response::prepareBody(std::string& full_path, const struct Route& correct_route,
	const serverConfig& server, const std::map<int, std::string>& status_code_info)
{
	if (std::filesystem::is_directory(full_path)) {

		std::string index_file = "index.html";
		if (!correct_route.path.empty() 
			&& !correct_route.indexFile.empty())
			index_file = correct_route.indexFile;

		std::string tmp = full_path + index_file;
		if (std::filesystem::exists(tmp)
			&& std::filesystem::is_regular_file(tmp)) {
			full_path = tmp;
			//addBody()
			//return
		}
		else {
			if (!correct_route.path.empty() && correct_route.autoindex) {
				//generate html file
				//addBody()
				//return
			}
			else {
				formError(403, server, status_code_info.at(403));
				return;
			}
		}

	}
	else if (std::filesystem::is_regular_file(full_path)) {
		addBody(full_path);
		this->status_code = "200";
		this->reason_phrase = status_code_info.at(200);
		addHeader("Content-Length", std::to_string(this->body.size()));
	}
	else
		formError(403, server, status_code_info.at(403));
}

void	Response::formError(int code, const serverConfig& server,
	const std::string& error_message)
{
	std::string error_file_path = "./website/errors/" + std::to_string(code) + ".html";

	const auto& it = server.errorPages.find(code);
	if (it != server.errorPages.end()) {
		if (!server.root.empty()) {
			error_file_path = server.root + it->second;
			//check if it exists and if it is a file
		}
	}

	this->status_code = std::to_string(code);
	this->reason_phrase = error_message;
	addBody(error_file_path);
	addHeader("Content-Length", std::to_string(this->body.size()));
}

void	Response::addBody(const std::string& full_path)
{
	std::ifstream file(full_path);

	if (file) {
		std::stringstream buffer;
		buffer << file.rdbuf();
		this->body = buffer.str();
		file.close();
	}

}

void	Response::sendResponse(int socket_fd)
{
	std::string response;

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
