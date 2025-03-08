#!/usr/bin/python

import os
import sys
import cgitb
cgitb.enable()

request_method = os.environ.get('REQUEST_METHOD', 'GET')
content_type = os.environ.get('CONTENT_TYPE', '')
content_length = os.environ.get('CONTENT_LENGTH', 0)

def fileType(post_data):
	pos = post_data.find(b'filename=') + 10
	type = ""

	for i in range(pos, len(post_data)):
		if post_data[i] == '"':
			return type
		type += post_data[i]
	return ""

def getData(post_data):
	start = post_data.find(b"\r\n\r\n") + 4
	end = post_data.find(b"------WebKitFormBoundary", start)
	return post_data[start:end]

post_data = sys.stdin.read()

file_type = fileType(post_data)
file_data = getData(post_data)

with open("/Users/ashirzad/Desktop/w/website/upload_files/" + file_type, "wb") as file:
	file.write(file_data)

print('file has been successfully created')
