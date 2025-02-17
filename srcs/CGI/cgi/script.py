#!/usr/bin/env python3

import os
import cgi
import cgitb
cgitb.enable()

form = cgi.FieldStorage()

name = form.getvalue('name', '(no name provided)')
message = form.getvalue('message', '(no message provided)')

name = name[0] if isinstance(name, list) else name
message = message[0] if isinstance(message, list) else message

print("Content-Type: text/html\r\n")
print()

print("<html>")
print("<head><title>CGI Response</title></head>")
print("<body>")
print("<h1>Hello, {}</h1>".format(name if name else "Stranger"))
print("</body>")
print("</html>")
