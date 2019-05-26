#!/usr/bin/python3
#################################
# Copyright (C) 2019 J.F.Dockes
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the
#	Free Software Foundation, Inc.,
#	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
########################################################

#
# Process the stream produced by a modified pffexport:
# https://github.com/libyal/libpff
# The tool has been modified to produce a data stream instead of a file tree

import sys
import os
import tempfile
import shutil
import getopt
import traceback
import email.parser
import email.policy
import mailbox

import rclconfig
import conftree

def _deb(s):
	print("%s"%s, file=sys.stderr)


# The pffexport stream yields the email in several pieces, with some
# data missing (e.g. attachment MIME types). We rebuild a complete
# message for parsing by the Recoll email handler
class EmailBuilder(object):
	def __init__(self):
		self.reset()
		self.parser = email.parser.Parser(policy = email.policy.default)
	def reset(self):
		self.headers = ''
		self.body = ''
		self.bodymime = ''
		self.attachments = []
	def setheaders(self, h):
		self.headers = h
	def setbody(self, body, main, sub):
		self.body = body
		self.bodymimemain = main
		self.bodymimesub = sub
	def addattachment(self, att, filename):
		_deb("Adding attachment")
		self.attachments.append((att, filename))
	def flush(self):
		if not self.headers:
			_deb("Not flushing because no headers")
		if self.headers and (self.body or self.attachments):
			newmsg = email.message.EmailMessage(policy =
													email.policy.default)
			
			headerstr = self.headers.decode('utf-8')
			# print("%s" % headerstr)
			headers = self.parser.parsestr(headerstr, headersonly=True)
			_deb("EmailBuilder: content-type %s" % headers['content-type'])
			for nm in ('from', 'subject'):
				if nm in headers:
					newmsg.add_header(nm, headers[nm])

			tolist = headers.get_all('to')
			alldests = ""
			for toheader in tolist:
				for dest in toheader.addresses:
					sd = str(dest).replace('\n', '').replace('\r','')
					_deb("EmailBuilder: dest %s" % sd)
					alldests += sd + ", "
				alldests = alldests.rstrip(", ")
				newmsg.add_header('to', alldests)

			# Also: CC
			
			if self.body:
				newmsg.set_content(self.body, maintype = self.bodymimemain,
									   subtype = self.bodymimesub)
				
			for att in self.attachments:
				#if self.body:
				#	newmsg.make_mixed()
				ext = os.path.splitext(att[1])[1]
				_deb("Querying mimemap with %s" % ext)
				mime = mimemap.get(ext)
				if not mime:
					mime = 'application/octet-stream'
				_deb("Attachment: filename %s MIME %s" % (att[1], mime))
				mt,st = mime.split('/')
				newmsg.add_attachment(att[0], maintype=mt, subtype=st,
									  filename=att[1])

			newmsg.set_unixfrom("From some@place.org Sun Jan 01 00:00:00 2000")
			print("%s\n" % newmsg.as_string(unixfrom=True, maxheaderlen=80))

		self.reset()
	

class PFFReader(object):
	def __init__(self, infile=sys.stdin):
		try:
			self.myname = os.path.basename(sys.argv[0])
		except:
			self.myname = "???"

		self.infile = infile
		self.fields = {}
		self.msg = EmailBuilder()
		
		if sys.platform == "win32":
			import msvcrt
			msvcrt.setmode(self.outfile.fileno(), os.O_BINARY)
			msvcrt.setmode(self.infile.fileno(), os.O_BINARY)
		self.debugfile = None
		if self.debugfile:
			self.errfout = open(self.debugfile, "a")
		else:
			self.errfout = sys.stderr
		
	def log(self, s):
		print("PFFReader: %s: %s" % (self.myname, s), file=self.errfout)

	# Read single parameter from process input: line with param name and size
	# followed by data. The param name is returned as str/unicode, the data
	# as bytes
	def readparam(self):
		inf = self.infile.buffer
		s = inf.readline()
		if s == b'':
			return ('', b'')
		s = s.rstrip(b'\n')
		if s == b'':
			return ('', b'')
		l = s.split()
		if len(l) != 2:
			self.log(b'bad line: [' + s + b']', 1, 1)
			return ('', b'')
		paramname = l[0].decode('ASCII').rstrip(':')
		paramsize = int(l[1])
		if paramsize > 0:
			paramdata = inf.read(paramsize)
			if len(paramdata) != paramsize:
				self.log("Bad read: wanted %d, got %d" %
					  (paramsize, len(paramdata)), 1, 1)
				return('', b'')
		else:
			paramdata = b''
		return (paramname, paramdata)

	def mainloop(self):
		basename = ''
		while 1:
			name, data = self.readparam()
			if name == "":
				break
			try:
				paramstr = data.decode('utf-8')
			except:
				paramstr = ''

			if name == 'filename':
				basename = os.path.basename(paramstr)
				self.log("name: [%s] data: %s" %
						 (name, paramstr))
				parentdir = os.path.basename(os.path.dirname(paramstr))
			elif name == 'data':
				if parentdir == 'Attachments':
					#self.log("Attachment: %s" % basename)
					self.msg.addattachment(data, basename)
				else:
					if basename == 'OutlookHeaders.txt':
						self.msg.flush()
						pass
					if basename == 'ConversationIndex.txt':
						pass
					elif basename == 'Recipients.txt':
						pass
					elif basename == 'InternetHeaders.txt':
						#self.log("name: [%s] data: %s" % (name, paramstr))
						self.msg.setheaders(data)
					elif os.path.splitext(basename)[0] == 'Message':
						ext = os.path.splitext(basename)[1]
						if ext == '.txt':
							self.msg.setbody(data, 'text', 'plain')
						elif ext == '.html':
							self.msg.setbody(data, 'text', 'html')
						elif ext == '.rtf':
							self.msg.setbody(data, 'text', 'rtf')
						else:
							raise Exception("PST: Unknown body type %s"%ext)
						self.log("Message")
						pass
				basename = ''
				parentdir = ''
		self.log("Out of loop")
		self.msg.flush()


config = rclconfig.RclConfig()
dir1 = os.path.join(config.getConfDir(), "examples")
dir2 = os.path.join(config.datadir, "examples")
mimemap = conftree.ConfStack('mimemap', [dir1, dir2])

proto = PFFReader()
proto.mainloop()
