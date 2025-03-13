#!/Users/ashirzad/homebrew/bin/python3.10

import os
import urllib.parse

server_upload_path = "/cgi-bin/database/"
filesystem_upload_path = "/Users/ashirzad/Desktop/webserv/website/cgi-bin/database/"

links = ""
for file in os.listdir(filesystem_upload_path):
	file_path = os.path.join(filesystem_upload_path, file)
	if os.path.isfile(file_path) and not file.startswith("."):
		encoded_filename = urllib.parse.quote(file)
		links += f'<a href="{server_upload_path}{encoded_filename}" download><div class="link">{file}</div></a>\n'

if links == "":
	links += "<p>no file</p>"

html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Download Files</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@100..900&display=swap');
        body {{
            font-family: 'Inter', sans-serif;
            background-color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            color: #333;
        }}
        .container {{
            text-align: center;
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }}
        .links a {{
            color: white;
            text-decoration: none;
            background-color: #121212;
        }}
        .link {{
            padding: 8px 12px;
            background-color: #121212;
            margin: 5px;
            border-radius: 5px;
            display: inline-block;
        }}
        .link:hover {{
            cursor: pointer;
            opacity: 0.9;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h2>Download the file you want</h2>
        <div class="links">
            {links}
        </div>
    </div>
</body>
</html>
"""

print(html_content)
