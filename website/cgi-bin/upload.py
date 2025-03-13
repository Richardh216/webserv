#!/Users/ashirzad/homebrew/bin/python3.10

import os
import cgi
import cgitb

dir_path = "./website/cgi-bin/database"

form = cgi.FieldStorage()

filename = form["file"].filename
data = form["file"].value

if filename == "" or data == "":
	print("upload request failed\n")

file_path = f"{dir_path}/{filename}"

with open(file_path, "wb") as file:
	file.write(data)

print(f"The {filename} got uploaded successfully ok\n")
