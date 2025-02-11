#!/usr/bin/env python3

import os
from urllib.parse import parse_qs

# Read environment variables
query_string = os.getenv("QUERY_STRING", "")
request_method = os.getenv("REQUEST_METHOD", "")

# Parse the query string
params = parse_qs(query_string)

# Generate a response
print("Content-Type: text/html\r\n\r\n", end="")
print("<html><body>")
print("<h1>CGI Script Output</h1>")
print(f"<p>Request Method: {request_method}</p>")
print(f"<p>Query String: {query_string}</p>")
print("<ul>")
for key, values in params.items():
    print(f"<li>{key}: {', '.join(values)}</li>")
print("</ul>")
print("</body></html>")
