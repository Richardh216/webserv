#include "../../includes/webserv.hpp"

std::string getConfigPath(int argc, char **argv)
{
	std::string	path = "./srcs/main/default.conf"; // define a default path

	if (argc > 1)
		path = argv[1];

	return path;
}

void	initStatusCodeInfo(std::map<int, std::string>& inf)
{
	inf[200] = "OK";

	inf[301] = "Moved Permanently";
	inf[302] = "Found";
	inf[307] = "Temporary Redirect";
	inf[308] = "Permanent Redirect";

	inf[400] = "Bad Request";
	inf[401] = "Unauthorized";
	inf[403] = "Forbidden";
	inf[404] = "Not Found";
	inf[405] = "Method Not Allowed";
	inf[408] = "Request Timeout";
	inf[413] = "Content Too Large";

	inf[500] = "Internal Server Error";
	inf[503] = "Service Unavailable";
	inf[505] = "HTTP Version Not Supported";
}

void	initContentTypes(std::map<std::string, std::string>& inf)
{
	inf[".html"] = "text/html";
	inf[".htm"] = "text/html";
	inf[".css"] = "text/css";
	inf[".xml"] = "text/xml";
	inf[".txt"] = "text/plain";
	inf[".jpg"] = "image/jpeg";
	inf[".jpeg"] = "image/jpeg";
	inf[".png"] = "image/png";
	inf[".gif"] = "image/gif";
	inf[".svg"] = "image/svg+xml";
	inf[".pdf"] = "application/pdf";
	inf[".mp3"] = "audio/mpeg";
	inf[".mp4"] = "video/mp4";
	inf[".webm"] = "video/webm";
	inf[".wav"] = "audio/wav";
	inf[".ogg"] = "audio/ogg";
	inf[".avi"] = "video/x-msvideo";
	inf[".mov"] = "video/quicktime";
	inf[".ico"] = "image/x-icon";
	inf[".tiff"] = "image/tiff";
	inf[".bmp"] = "image/bmp";
}
