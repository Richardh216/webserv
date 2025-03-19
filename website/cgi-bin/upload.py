import sys
import os
import cgi
import cgitb

print("Content-Type: text/plain\n")

dir_path = os.environ.get("UPLOAD_PATH")

form = cgi.FieldStorage()

filename = form["file"].filename
data = form["file"].value

if filename == "" or data == "":
	print("upload request failed\n")

file_path = f"{dir_path}/{filename}"

try:
	with open(file_path, "wb") as file:
		file.write(data)
except:
	print("No such file or directory:", file_path)
	sys.exit(1)

print(f"The {filename} got uploaded successfully\n")
