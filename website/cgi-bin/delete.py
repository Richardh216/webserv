import os
import urllib.parse

print("Content-Type: text/plain\n")

dir_path = os.environ.get("UPLOAD_PATH")

query_string = os.environ.get('QUERY_STRING', '')


try :
	obj = {}
	for file in query_string.split("&"):
		key, value = file.split("=")
		obj[key] = value

	value = obj.get("file", "")
	if ".." in value or value == "":
		print("invalid query string\n")
	file_path = f"{dir_path}/{value}"
	if os.path.exists(file_path):
		os.remove(file_path)
		print(f"File '{value}' successfully got deleted\n")
	else:
		print(f"no such file '{value}' exists\n")
except:
	print("invalid request\n")







