# Recoll

This is a fork from of `recoll` from [https://framagit.org/medoc92/recoll](https://framagit.org/medoc92/recoll).

- Implements real time indexing for macOS systems. Before, real-time indexing was reserved to Linux and Windows. The implementation for mac works very efficiently and makes use of native `fsevents` framework.
- Improve several scripts for parsing documents including `rclpdf` for `pdf` documents, which used to crash due to some wrong handling of threads
- Include `rclmd.py` handler for parsing MarkDown files; it parses the YAML frontmatter used in Obsidian.

---

Recoll is a desktop full-text search tool. It finds keywords inside
documents as well as file names. 

* Versions are available for Linux and MS Windows.
* A WEB front-end with preview and download features can replace or
  supplement the GUI for remote use. 
* It can search most document formats. You may need external applications
  for text extraction. 
* It can reach any storage place: files, archive members, email
  attachments, transparently handling decompression. 
* One click will open the document inside a native editor or display an
  even quicker text preview. 
* The software is free, open source, and licensed under the GPL.

For more detail, see the [features page on the web site](https://www.recoll.org/features.html) or
the [online documentation](https://www.recoll.org/pages/documentation.html). 

Most distributions feature prebuilt packages for Recoll, but if you need them, [the instructions for
building and installing are here](https://www.recoll.org/usermanual/usermanual.html#RCL.INSTALL.BUILDING).
