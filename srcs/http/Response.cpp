#include "../../includes/webserv.hpp"

std::string	Response::findHeaderValue(const std::string& name,
	const std::map<std::string, std::string>& headers) const
{
	const auto& it = headers.find(name);

	std::string value = "";
	if (it != headers.end())
		value = it->second;

	return value;
}

void	Response::chooseServer(const HttpRequest& request,
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
	if (this->choosed_server == nullptr) {
		formError(404, webserv);
		is_formed = true;
		return;
	}

	this->http_version = "HTTP/1.1";

	std::string	time = takeGMTTime();
	if (!time.empty()) addHeader("Date", time);

	addHeader("Server", "webserv/0.01");
	addHeader("Connection", "close");

	this->request_path = request.path;
	findRouteInConfig(request.path);
	if (choosed_route && !choosed_route->redirection.empty()) {
		redirect(webserv);
		is_formed = true;
		return;
	}

	std::string full_path = findFullPath(request.path);

	if (full_path.empty() || !std::filesystem::exists(full_path)) {
		formError(404, webserv);
	}
	if (1 == 1) {
		if (request.path.find("cgi-bin") != std::string::npos){
			handleCGI(request, webserv);
		}
		else if (request.method == "GET" && isMethodAllowed(request.method)) {
			handleGET(full_path, webserv);
		}
		else {
			formError(405, webserv);
		}
	}
	is_formed = true;
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
		formError(403, webserv);
}

//handle CGI

void	Response::handleCGI(const HttpRequest &request, const Webserv &webserv)
{
	if (request.path.find("/cgi-bin/upload.py") != std::string::npos)
	{
		CgiHandler handler(request, webserv, "/Users/ashirzad/Desktop/webserv/website/cgi-bin/upload.py");

		this->body = handler.executeCgi();
		addHeader("Content-Type", "text/plain");
		addHeader("Content-Length", std::to_string(body.size()));
	}
	else if (request.path.find("/cgi-bin/download.py") != std::string::npos)
	{
		CgiHandler handler(request, webserv, "/Users/ashirzad/Desktop/webserv/website/cgi-bin/download.py");

		this->body = handler.executeCgi();
		addHeader("Content-Type", "text/html");
		addHeader("Content-Length", std::to_string(body.size()));
	}
	else if (request.path.find("/cgi-bin/delete.py") != std::string::npos)
	{
		CgiHandler handler(request, webserv, "/Users/ashirzad/Desktop/webserv/website/cgi-bin/delete.py");

		this->body = handler.executeCgi();
		addHeader("Content-Type", "text/plain");
		addHeader("Content-Length", std::to_string(body.size()));
	}
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
			generateAutoindexHTML(full_path, webserv);
		}
		else {
			formError(403, webserv);
		}
	}
}

void	Response::serveFile(const std::string& full_path, const Webserv& webserv)
{
	if (addBody(full_path, true)) {
		this->status_code = "200";
		this->reason_phrase = webserv.status_code_info.at(200);
		addHeader("Content-Length", std::to_string(this->body.size()));

		std::string type = checkContentType(full_path, webserv);
		if (!type.empty()) addHeader("Content-Type", type); // + "; charset=utf-8" ?
	}
	else {
		formError(404, webserv);
	}
}

void	Response::formError(int code, const Webserv& webserv)
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
	this->reason_phrase = webserv.status_code_info.at(code);
	if (!addBody(error_file_path, true)) {
		this->body = "<html><body><h1>" + status_code + " "
			+ reason_phrase + "</h1></body></html>";
		addHeader("Content-Type", "text/html");
	}
	else {
		std::string type = checkContentType(error_file_path, webserv);
		if (!type.empty()) addHeader("Content-Type", type); // + "; charset=utf-8" ?
	}

	addHeader("Content-Length", std::to_string(this->body.size()));
}

bool	Response::addBody(const std::string& full_path, bool is_bin)
{
	std::ifstream file(full_path,
		(is_bin ? (std::ios::binary | std::ios::in) : std::ios::in));

	if (!file)
		return false;

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	this->body.resize(size);
	file.read(&this->body[0], size);

	return file.good();
}

int	Response::sendResponse()
{
	std::string initline_and_headers;

	if (!headers_sent) {
		initline_and_headers.reserve(1024);
		initline_and_headers = http_version + " " + status_code + " "
			+ reason_phrase + "\r\n";

		for (const auto&[key, value] : headers) {
			initline_and_headers += key + ": " + value + "\r\n";
		}
		initline_and_headers += "\r\n";

		if (sendChunk(initline_and_headers)) {
			headers_sent = true;
			total_bytes_sent = 0;
		}
		else return 0;
	}
	return sendChunk(this->body);
}

int	Response::sendChunk(const std::string& chunk)
{
	int	bytes_sent = 0;
	int response_size = chunk.size() - total_bytes_sent;
	const char* to_send = chunk.c_str() + total_bytes_sent;

	bytes_sent = send(fd, to_send, response_size, 0);
	if (bytes_sent == -1) {
		std::cerr << "failed to send a http request" << std::endl;
		return 0;
	}
	total_bytes_sent += bytes_sent;
	return (size_t)total_bytes_sent == chunk.size();
}

std::string Response::takeGMTTime()
{
	std::time_t t = std::time(nullptr);
	const char format[] = "%a, %d %b %Y %H:%M:%S GMT";
	std::vector<char> buff(100);

	std::size_t size = std::strftime(buff.data(), buff.size(), format, std::gmtime(&t));
	return (size > 0) ? std::string(buff.data()) : "";
}

std::string	Response::checkContentType(std::string file, const Webserv& webserv)
{
	std::string extension = std::filesystem::path(file).extension().string();

	const auto& it = webserv.content_types.find(extension);
	return (it != webserv.content_types.end() ? it->second : "");
}

bool	Response::isMethodAllowed(const std::string& method)
{
	/**
	 * GET & POST are enabled by default
	 */
	if (!choosed_route || choosed_route->allowedMethods.empty()) {
		return (method == "GET" || method == "POST");
	}

	const auto& it = std::find(choosed_route->allowedMethods.begin(),
		choosed_route->allowedMethods.end(), method);
	return (it != choosed_route->allowedMethods.end());
}

void	Response::redirect(const Webserv& webserv)
{
	const auto& redir_inf = choosed_route->redirection.begin();

	const auto& code_info = webserv.status_code_info.find(redir_inf->first);
	if (code_info == webserv.status_code_info.end()) {
		formError(500, webserv);
		return;
	}

	this->status_code = code_info->first;
	this->reason_phrase = code_info->second;
	addHeader("Location", redir_inf->second);
}

void	Response::generateAutoindexHTML(const std::string& full_path, const Webserv& webserv)
{
	std::string tmp_path = full_path;
	if (!tmp_path.empty() && tmp_path.back() == '/') tmp_path.pop_back();

	std::string main_dir = std::filesystem::path(tmp_path).filename().string();
	if (!main_dir.empty()) main_dir += "/";

	body = R"(
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Index of )" + main_dir + R"(</title>
		<style>
			body {font-family: Arial, sans-serif;
				background: linear-gradient(120deg,#141E30,#243B55);
				color: white;height: 100vh;display: flex;
				flex-direction: column;align-items: center; }
			a{color:#007BFF;text-decoration: none;} a:hover {color:#0056b3;}
			.container {min-width: 400px;}
			.entry {display: flex;padding: 5px 0;
				border-bottom: 1px solid rgba(255, 255, 255, 0.2);}
			.name {width: 250px;text-align: left;}
			.date {width: 150px;text-align: left;}
			.size {width: 100px;text-align: right;}
		</style>
	</head><body><h2>Index of )" + main_dir + R"(</h2><div class="container">)";

	std::filesystem::path dir_path(full_path);
	if (dir_path.has_parent_path()) // exclude going out of the root (correct ?)
		body += R"(<a href="../">../</a>)";

	if (!request_path.empty() && request_path.back() != '/')
		request_path += "/";
	try {

		for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {

			std::string filename = entry.path().filename().string();
			if (entry.is_directory()) filename += "/";

			std::string file_size = (entry.is_directory() ? "-"
				: std::to_string(entry.file_size()));

			std::string date = checkLastWriteTime(entry.path().c_str());

			body += "<div class=\"entry\"><a class=\"name\" href=\"" + request_path
				+ filename + "\">" + filename + "</a><span class=\"date\">"
				 + date + "</span><span class=\"size\">" + file_size + "</span></div>";
		}
	}
	catch(...) {
		formError(500, webserv);
		return;
	}

	body += "</div></body></html>";

	this->status_code = "200";
	this->reason_phrase = webserv.status_code_info.at(200);
	addHeader("Content-Type", "text/html");
	addHeader("Content-Length", std::to_string(this->body.size()));
}

std::string Response::checkLastWriteTime(const char *path)
{
	std::string mod_time = "";
	struct stat fileStat;

	if (stat(path, &fileStat) == 0) {
		time_t t = fileStat.st_mtime;
		const char format[] = "%d-%b-%Y %H:%M";
		char buff[50];

		std::size_t size = std::strftime(buff, sizeof(buff),
			format, std::gmtime(&t));
		if (size > 0) mod_time = std::string(buff);
	}
	return mod_time;
}

bool	Response::getIsFormed() const {
	return is_formed;
}

int	Response::getFd() const {
	return fd;
}

void	Response::setFd(int fd) {
	this->fd = fd;
}
