#! /usr/bin/python3
# -*- mode: python; python-indent: 2 -*-
#
# Copyright 2012 Canonical Ltd.  2013 Jean-Francois Dockes
#
# Contact: Jean-Francois Dockes <jfd@recoll.org>
#
# GPLv3
#

import os
import gettext
import locale
import sys
import time
import urllib.parse
import hashlib
import subprocess

from gi.repository import GLib, GObject, Gio
from gi.repository import Accounts, Signon
from gi.repository import GData
from gi.repository import Unity

try:
  from recoll import rclconfig
  hasrclconfig = True
except:
  hasrclconfig = False
# As a temporary measure, we also look for rclconfig as a bare
# module. This is so that the intermediate releases of the lens can
# ship and use rclconfig.py with the lens code
if not hasrclconfig:
  try:
    import rclconfig
    hasrclconfig = True
  except:
    pass
    
try:
  from recoll import recoll
  from recoll import rclextract
  hasextract = True
except:
  import recoll
  hasextract = False

APP_NAME = "unity-scope-recoll"
LOCAL_PATH = "/usr/share/locale/"
gettext.bindtextdomain(APP_NAME, LOCAL_PATH)
gettext.textdomain(APP_NAME)
_ = gettext.gettext

GROUP_NAME = 'org.recoll.Unity.Scope.File.Recoll'
UNIQUE_PATH = '/org/recoll/unity/scope/file/recoll'
SEARCH_URI = ''
SEARCH_HINT = _('Search Recoll index')
NO_RESULTS_HINT = _('Sorry, there are no documents in the Recoll index that match your search.')
PROVIDER_CREDITS = _('Powered by Recoll')
SVG_DIR = '/usr/share/icons/unity-icon-theme/places/svg/'
PROVIDER_ICON = SVG_DIR+'service-recoll.svg'
DEFAULT_RESULT_ICON = 'recoll'
DEFAULT_RESULT_TYPE = Unity.ResultType.PERSONAL

c0 = {'id': 'global',
      'name': _('File & Folders'),
      'icon': SVG_DIR + 'group-installed.svg',
      'renderer': Unity.CategoryRenderer.VERTICAL_TILE}
c1 = {'id': 'recent',
      'name': _('Recent'),
      'icon': SVG_DIR + 'group-installed.svg',
      'renderer': Unity.CategoryRenderer.VERTICAL_TILE}
c2 = {'id': 'download',
      'name': _('Download'),
      'icon': SVG_DIR + 'group-folders.svg',
      'renderer': Unity.CategoryRenderer.VERTICAL_TILE}
c3 = {'id': 'folders',
      'name': _('Folders'),
      'icon': SVG_DIR + 'group-folders.svg',
      'renderer': Unity.CategoryRenderer.VERTICAL_TILE}
CATEGORIES = [c0, c1, c2, c3]

FILTERS = []

EXTRA_METADATA = []

UNITY_TYPE_TO_RECOLL_CLAUSE = {
    "documents" : "rclcat:message rclcat:spreadsheet rclcat:text",
    "folders" : "mime:inode/directory", 
    "images" : "rclcat:media", 
    "audio":"rclcat:media", 
    "videos":"rclcat:media",
    "presentations" : "rclcat:presentation",
    "other":"rclcat:other",
    }

# Icon names for some recoll mime types which don't have standard icon
# by the normal method. Some keys are actually icon names, not mime types
SPEC_MIME_ICONS = {'application/x-fsdirectory' : 'gnome-fs-directory',
                   'inode/directory' : 'gnome-fs-directory',
                   'message/rfc822' : 'emblem-mail',
                   'message-rfc822' : 'emblem-mail',
                   'application/x-recoll' : 'recoll'}

# Truncate results here:
MAX_RESULTS = 30

# Where the thumbnails live:
XDGCACHE = os.getenv('XDG_CACHE_DIR', os.path.expanduser("~/.cache"))
THUMBDIRS = [os.path.join(XDGCACHE, "thumbnails"),
             os.path.expanduser("~/.thumbnails")]

def url_encode_for_thumb(in_s, offs):
  h = b"0123456789ABCDEF"
  out = in_s[:offs]
  for i in range(offs, len(in_s)):
    c = in_s[i]
    if c <= 0x20 or c >= 0x7f or in_s[i] in b'"#%;<>?[\\]^`{|}':
      out += bytes('%', 'ascii');
      out += bytes(chr(h[(c >> 4) & 0xf]), 'ascii')
      out += bytes(chr(h[c & 0xf]), 'ascii')
    else:
      out += bytes(chr(c), 'ascii')
      pass
  return out

def _get_thumbnail_path(url):
    """Look for a thumbnail for the input url, according to the
    freedesktop thumbnail storage standard. The input 'url' always
    begins with file:// and is binary. We encode it properly
    and compute the path inside the thumbnail storage
    directory. We return the path only if the thumbnail does exist
    (no generation performed)"""
    global THUMBDIRS

    # Compute the thumbnail file name by encoding and hashing the url string
    try:
      url = url_encode_for_thumb(url, 7)
    except Exception as msg:
        print("_get_thumbnail_path: url encode failed: %s" % msg, 
              file=sys.stderr)
        return ""
    #print("_get_thumbnail: encoded path: [%s]" % url, file=sys.stderr)
    thumbname = hashlib.md5(url).hexdigest() + ".png"

    # If the "new style" directory exists, we should stop looking in
    # the "old style" one (there might be interesting files in there,
    # but they may be stale, so it's best to not touch them). We do
    # this semi-dynamically so that we catch things up if the
    # directory gets created while we are running.
    if os.path.exists(THUMBDIRS[0]):
        THUMBDIRS = THUMBDIRS[0:1]

    # Check in appropriate directories to see if the thumbnail file exists
    #print("_get_thumbnail: thumbname: [%s]" % thumbname, file=sys.stderr)
    for topdir in THUMBDIRS:
        for dir in ("large", "normal"): 
            tpath = os.path.join(topdir, dir, thumbname)
            #print("Testing [%s]" % (tpath,), file=sys.stderr)
            if os.path.exists(tpath):
                return tpath

    return ""


class RecollScopePreviewer(Unity.ResultPreviewer):
  def do_run(self):
    icon = Gio.ThemedIcon.new(self.result.icon_hint)
    preview = Unity.GenericPreview.new(self.result.title, 
                                       self.result.comment.strip(), icon)
    view_action = Unity.PreviewAction.new("open", _("Open"), None)
    preview.add_action(view_action)
    show_action = Unity.PreviewAction.new("show", _("Show in Folder"), None)
    preview.add_action(show_action)
    return preview

class RecollScope(Unity.AbstractScope):
  __g_type_name__ = "RecollScope"

  def __init__(self):
    super(RecollScope, self).__init__()
    self.search_in_global = True;
    lng, self.localecharset = locale.getdefaultlocale()

  def do_get_search_hint (self):
    return SEARCH_HINT

  def do_get_schema (self):
    #print("RecollScope: do_get_schema", file=sys.stderr)
    schema = Unity.Schema.new ()
    if EXTRA_METADATA:
      for m in EXTRA_METADATA:
        schema.add_field(m['id'], m['type'], m['field'])
    #FIXME should be REQUIRED for credits
    schema.add_field('provider_credits', 's', 
                     Unity.SchemaFieldType.OPTIONAL)
    return schema

  def do_get_categories(self):
    #print("RecollScope: do_get_categories", file=sys.stderr)
    cs = Unity.CategorySet.new ()
    if CATEGORIES:
      for c in CATEGORIES:
        cat = Unity.Category.new (c['id'], c['name'],
                                  Gio.ThemedIcon.new(c['icon']),
                                  c['renderer'])
        cs.add (cat)
    return cs

  def do_get_filters(self):
    #print("RecollScope: do_get_filters", file=sys.stderr)
    filters = Unity.FilterSet.new()
    f = Unity.RadioOptionFilter.new(
      "modified", _("Last modified"),     
      Gio.ThemedIcon.new("input-keyboard-symbolic"), False)
    f.add_option ("last-7-days", _("Last 7 days"), None)
    f.add_option ("last-30-days", _("Last 30 days"), None)
    f.add_option ("last-year", _("Last year"), None);
    filters.add(f)

    f2 = Unity.CheckOptionFilter.new (
      "type", _("Type"), Gio.ThemedIcon.new("input-keyboard-symbolic"), False)
    f2.add_option ("documents", _("Documents"), None)
    f2.add_option ("folders", _("Folders"), None)
    f2.add_option ("images", _("Images"), None)
    f2.add_option ("audio", _("Audio"), None)
    f2.add_option ("videos", _("Videos"), None)
    f2.add_option ("presentations", _("Presentations"), None)
    f2.add_option ("other", _("Other"), None)
    filters.add (f2)

    f3 = Unity.MultiRangeFilter.new (
      "size", _("Size"), Gio.ThemedIcon.new("input-keyboard-symbolic"), False)
    f3.add_option ("1kb", _("1KB"), None)
    f3.add_option ("100kb", _("100KB"), None)
    f3.add_option ("1mb", _("1MB"), None)
    f3.add_option ("10mb", _("10MB"), None)
    f3.add_option ("100mb", _("100MB"), None)
    f3.add_option ("1gb", _("1GB"), None)
    f3.add_option (">1gb", _(">1GB"), None)
    filters.add (f3)

    return filters

  def do_get_group_name(self):
    return GROUP_NAME

  def do_get_unique_name(self):
    return UNIQUE_PATH

  def do_create_search_for_query(self, search_context):
    #print("RecollScope: do_create_search_for query", file=sys.stderr)
    return RecollScopeSearch(search_context)

  def do_activate(self, result, metadata, id):
    print("RecollScope: do_activate. id [%s] uri [%s]" % (id, result.uri), 
          file=sys.stderr)
    if id == 'show':
      os.system("nautilus '%s'" % str(result.uri))
      return Unity.ActivationResponse(handled=Unity.HandledType.HIDE_DASH,
                                      goto_uri=None)
    else:
      uri = result.uri
      # Pass all uri without fragments to the desktop handler
      if uri.find("#") == -1:
        return Unity.ActivationResponse(handled=Unity.HandledType.NOT_HANDLED,
                                         goto_uri=uri)
      # Pass all others to recoll
      proc = subprocess.Popen(["recoll", uri])
      ret = Unity.ActivationResponse(handled=Unity.HandledType.HIDE_DASH,
                                     goto_uri=None)
      return ret

  def do_create_previewer(self, result, metadata):
    #print("RecollScope: do_create_previewer", file=sys.stderr)
    previewer = RecollScopePreviewer()
    previewer.set_scope_result(result)
    previewer.set_search_metadata(metadata)
    return previewer


class RecollScopeSearch(Unity.ScopeSearchBase):
  __g_type_name__ = "RecollScopeSearch"

  def __init__(self, search_context):
    super(RecollScopeSearch, self).__init__()
    self.set_search_context(search_context)
    self.max_results = MAX_RESULTS
    if hasrclconfig:
      self.config = rclconfig.RclConfig()
      try:
        self.max_results = int(self.config.getConfParam("unityscopemaxresults"))
      except:
        pass

  def connect_db(self):
    #print("RecollScopeSearch: connect_db", file=sys.stderr)
    self.db = None
    dblist = []
    if hasrclconfig:
      extradbs = rclconfig.RclExtraDbs(self.config)
      dblist = extradbs.getActDbs()
    try:
      self.db = recoll.connect(extra_dbs=dblist)
      self.db.setAbstractParams(maxchars=200, contextwords=4)
    except Exception as s:
      print("RecollScope: Error connecting to db: %s" % s, file=sys.stderr)
      return

  def do_run(self):
    #print("RecollScopeSearch: do_run", file=sys.stderr)
    context = self.search_context
    filters = context.filter_state
    search_string = context.search_query.strip()
    if not search_string or search_string is None:
      return
    result_set = context.result_set
    
    # Get the list of documents
    is_global = context.search_type == Unity.SearchType.GLOBAL
    self.connect_db()

    # We do not want filters to effect global results
    catgf = ""
    datef = ""
    if not is_global:
      catgf = self.catg_filter(filters)
      datef = self.date_filter(filters)
      sizef = self.size_filter(filters)
      search_string = " ".join((search_string, catgf, datef, sizef))
    else:
      print("RecollScopeSearch: GLOBAL", file=sys.stderr)

    # Do the recoll thing
    try:
      query = self.db.query()
      nres = query.execute(search_string)
    except Exception as msg:
      print("recoll query execute error: %s" % msg, file=sys.stderr)
      return

    print("RecollScopeSearch::do_run: [%s] -> %d results" % 
          (search_string, nres), file=sys.stderr)

    actual_results = 0
    for i in range(nres):
      try:
        doc = query.fetchone()
      except:
        break

      titleorfilename = doc.title
      if titleorfilename is None or titleorfilename == "":
        titleorfilename = doc.filename
      if titleorfilename is None:
        titleorfilename = "doc.title and doc.filename are none !"

      url, mimetype, iconname = self.icon_for_type (doc)

      try:
        abstract = self.db.makeDocAbstract(doc, query)
      except:
        pass

      # Ok, I don't understand this category thing for now...
      category = 0
      #print({"uri":url,"icon":iconname,"category":category,
       #      "mimetype":mimetype, "title":titleorfilename,
        #     "comment":abstract,
         #    "dnd_uri":doc.url}, file=sys.stderr)

      result_set.add_result(
        uri=url,
        icon=iconname,
        category=category,
        result_type=Unity.ResultType.PERSONAL,
        mimetype=mimetype,
        title=titleorfilename,
        comment=abstract,
        dnd_uri=doc.url)

      actual_results += 1
      if actual_results >= self.max_results:
        break

  def date_filter (self, filters):
    print("RecollScopeSearch: date_filter", file=sys.stderr)
    dateopt = ""
    f = filters.get_filter_by_id("modified")
    if f != None:
      o = f.get_active_option()
      if o != None:
        if o.props.id == "last-year":
          dateopt="date:P1Y/"
        elif o.props.id == "last-30-days":
          dateopt = "date:P1M/"
        elif o.props.id == "last-7-days":
          dateopt = "date:P7D/"
    #print("RecollScopeSearch::date_filter:[%s]" % dateopt, file=sys.stderr)
    return dateopt

  def catg_filter(self, filters):
    print("RecollScopeSearch::catg_filter", file=sys.stderr)
    f = filters.get_filter_by_id("type")
    if not f: return ""
    if not f.props.filtering:
      return ""
    ss = ""
    for fopt in f.options:
      if fopt.props.active:
        if fopt.props.id in UNITY_TYPE_TO_RECOLL_CLAUSE:
          ss += " " + UNITY_TYPE_TO_RECOLL_CLAUSE[fopt.props.id]
    #print("RecollScopSearch::catg_filter:[%s]" % ss, file=sys.stderr)
    return ss

  def size_filter(self, filters):
    print("RecollScopeSearch::size_filter", file=sys.stderr)
    f = filters.get_filter_by_id("size")
    if not f: return ""
    if not f.props.filtering:
      return ""
    min = f.get_first_active()
    max = f.get_last_active()
    if min.props.id == max.props.id:
      # Take it as < except if it's >1gb
      if max.props.id == ">1gb":
        ss = " size>1g"
      else:
        ss = " size<" + min.props.id
    else:
      if max.props.id == ">1gb":
        ss = "size>" + min.props.id
      else:
        ss = " size>" + min.props.id + " size<" + max.props.id
    #print("RecollScopeSearch::size_filter: [%s]" % ss, file=sys.stderr)
    return ss

  # Send back a useful icon depending on the document type
  def icon_for_type (self, doc):
    iconname = "text-x-preview"
    # Results with an ipath get a special mime type so that they
    # get opened by starting a recoll instance.
    thumbnail = ""
    if doc.ipath != "":
      mimetype = "application/x-recoll"
      url = doc.url + "#" + doc.ipath
    else:
      mimetype = doc.mimetype
      url = doc.url
      # doc.url is a unicode string which is badly wrong. 
      # Retrieve the binary path for thumbnail access.
      thumbnail = _get_thumbnail_path(doc.getbinurl())

    iconname = ""
    if thumbnail:
      iconname = thumbnail
    else:
      content_type = Gio.content_type_from_mime_type(doc.mimetype)
      icon = Gio.content_type_get_icon(content_type)
      if icon:
        # At least on Quantal, get_names() sometimes returns
        # a list with '(null)' in the first position...
        for iname in icon.get_names():
          if iname != '(null)':
            iconname = iname
            break
      if iconname in SPEC_MIME_ICONS:
          iconname = SPEC_MIME_ICONS[iconname]
      if iconname == "":
        if doc.mimetype in SPEC_MIME_ICONS:
          iconname = SPEC_MIME_ICONS[doc.mimetype]

    return (url, mimetype, iconname);


def load_scope():
  return RecollScope()
