#!/usr/bin/env python3

import os
from urllib.parse import parse_qs

# Read environment variables
query_string = os.getenv("QUERY_STRING", "")
request_method = os.getenv("REQUEST_METHOD", "")

# Parse the query string
params = {}
for value in query_string.split("?"):
	params[value.split("=")[0]] = value.split("=")[1]

# Generate a response
print("Content-Type: text/html\r\n\r\n", end="")
print("<html><body>")
print("<h1>CGI Script Output</h1>")
print(f"<p>Request Method: {request_method}</p>")
print("<ul>")
for key, value in params.items():
	print(f"<li> {key}: {value} </li>")
print("</ul>")
print("</body></html>")
