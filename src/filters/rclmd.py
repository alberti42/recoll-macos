#!/usr/bin/env python3
#
# Script name: rclmd.py
# Author: Andrea Alberti

import os
import re
import rclexecm  # For communicating with recoll
import yaml  # Import PyYAML for parsing YAML front matter
from rclbasehandler import RclBaseHandler

####
_htmlprefix =b'''<html><head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
'''
####
_htmlmidfix = b'''</head><body><pre>
'''
####
_htmlsuffix = b'''
</pre></body></html>'''
####

class MDhandler(RclBaseHandler):
    def __init__(self, em):
        super(MDhandler, self).__init__(em)

    def parse_md_file(self, text_content):
        """Extract YAML front matter and return it along with the body content."""
        frontmatter = {}
        body_content = text_content

        if text_content.startswith('---'):
            end = text_content.find('---', 3)
            if end != -1:
                yaml_content = text_content[3:end].strip()
                frontmatter = yaml.safe_load(yaml_content)
                body_content = text_content[end + 3:].strip()  # Get the content after the front matter

        return frontmatter, body_content

    def extract_tags(self, frontmatter):
        """Extract tags from the YAML front matter."""
        stripped_tags = []
        if 'tags' in frontmatter:
            tags=frontmatter['tags']
            if tags is not None:        
                stripped_tags = [tag.lstrip('#') for tag in frontmatter['tags']]  # Remove leading '#' if present
        return stripped_tags

    def html_text(self, filename):
        # self.em.log(f"Filename: {filename.decode('utf-8')}")
        # Read the file content
        with open(filename, 'rb') as file:
            content = file.read()
        # self.em.log(f"Content:\n{content}")
        # Convert content to string (assuming UTF-8 encoding)
        text_content = content.decode('utf-8')

        # Parse the front matter and get the body content
        frontmatter, body_content = self.parse_md_file(text_content)

        # Extract tags from the front matter
        tags = self.extract_tags(frontmatter)

        # Log the extracted tags (optional for debugging)
        # self.em.log(f"Extracted tags: {tags}")

        # Start generating the HTML output
        output = _htmlprefix

        # Add meta tags for each extracted tag (using MDT as the prefix)
        # self.em.log(f"TAGS: {tags}")
        for tag in tags:
            output += b'<meta name="tags" content="' + \
                      rclexecm.htmlescape(rclexecm.makebytes(tag)) + b'">\n'

        output += _htmlmidfix

        # Include the Markdown content without the front matter
        if frontmatter:
            content_start = text_content.find('---', 3) + 4
            output += rclexecm.htmlescape(text_content[content_start:].encode('utf-8'))
        else:
            output += rclexecm.htmlescape(content)

        output += _htmlsuffix

        self.outputmimetype = "text/html"

        return output

if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = MDhandler(proto)
    rclexecm.main(proto, extract)
