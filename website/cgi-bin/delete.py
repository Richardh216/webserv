#!/Users/ashirzad/homebrew/bin/python3.10

import os
import cgi
import cgitb
import urllib.parse

path = "/Users/ashirzad/Desktop/webserv/website/cgi-bin/database"


# # Parse form data
query_string = os.environ.get('QUERY_STRING', '')

resource = query_string[query_string.find("=") + 1:]

if ".." in resource:
	print("invalid filename\n")

file_path = f"{path}/{resource}"

if os.path.exists(file_path):
	os.remove(file_path)
	print(f"File '{resource}' successfully got deleted\n")
else:
	print(f"no suche file '{resource}' exists\n")





