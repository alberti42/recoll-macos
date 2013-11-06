#! /usr/bin/python3
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

locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP_NAME, LOCAL_PATH)
gettext.textdomain(APP_NAME)
_ = gettext.gettext

THEME = "/usr/share/icons/unity-icon-theme/places/svg/"


# Icon names for some recoll mime types which don't have standard icon by the
# normal method
SPEC_MIME_ICONS = {'application/x-fsdirectory' : 'gnome-fs-directory.svg',
                   'inode/directory' : 'gnome-fs-directory.svg',
                   'message/rfc822' : 'mail-read',
                   'application/x-recoll' : 'recoll'}

# Truncate results here:
MAX_RESULTS = 30

# Where the thumbnails live:
XDGCACHE = os.getenv('XDG_CACHE_DIR', os.path.expanduser("~/.cache"))
THUMBDIRS = [os.path.join(XDGCACHE, "thumbnails"),
             os.path.expanduser("~/.thumbnails")]

def _get_thumbnail_path(url):
    """Look for a thumbnail for the input url, according to the
    freedesktop thumbnail storage standard. The input 'url' always
    begins with file:// and is unencoded. We encode it properly
    and compute the path inside the thumbnail storage
    directory. We return the path only if the thumbnail does exist
    (no generation performed)"""
    global THUMBDIRS

    # Compute the thumbnail file name by encoding and hashing the url string
    path = url.replace("file://", "", 1)
    try:
        path = "file://" + urllib.quote(path)
    except:
        #print "_get_thumbnail_path: urllib.quote failed"
        return None
    #print "_get_thumbnail: encoded path: [%s]" % (path,)
    thumbname = hashlib.md5(path).hexdigest() + ".png"

    # If the "new style" directory exists, we should stop looking in
    # the "old style" one (there might be interesting files in there,
    # but they may be stale, so it's best to not touch them). We do
    # this semi-dynamically so that we catch things up if the
    # directory gets created while we are running.
    if os.path.exists(THUMBDIRS[0]):
        THUMBDIRS = THUMBDIRS[0:1]

    # Check in appropriate directories to see if the thumbnail file exists
    #print "_get_thumbnail: thumbname: [%s]" % (thumbname,)
    for topdir in THUMBDIRS:
        for dir in ("large", "normal"): 
            tpath = os.path.join(topdir, dir, thumbname)
            # print "Testing [%s]" % (tpath,)
            if os.path.exists(tpath):
                return tpath

    return None


class RecollScope(Unity.AbstractScope):
  __g_type_name__ = "RecollScope"

  def __init__(self):
    super(RecollScope, self).__init__()
    self.search_in_global = True;

    lng, self.localecharset = locale.getdefaultlocale()
    if hasrclconfig:
      self.config = rclconfig.RclConfig()
    self.last_connect_time = 0

  def _connect_db(self):
    #print "Connecting to db"
    self.db = None
    dblist = []
    if hasrclconfig:
      extradbs = rclconfig.RclExtraDbs(self.config)
      dblist = extradbs.getActDbs()
      try:
        self.db = recoll.connect(extra_dbs=dblist)
        self.db.setAbstractParams(maxchars=200, contextwords=4)
      except Exception, s:
        print >> sys.stderr, "recoll-lens: Error connecting to db:", s
        return

  def do_get_group_name(self):
    # The primary bus name we grab *must* match what we specify in our
    # .scope file
    return "org.recoll.Unity.Scope.File.Recoll"

  def do_get_unique_name(self):
    return "/org/recoll/unity/scope/file/recoll"

  def do_get_filters(self):
    filters = Unity.FilterSet.new()
    f = Unity.RadioOptionFilter.new ("modified", _("Last modified"), Gio.ThemedIcon.new("input-keyboard-symbolic"), False)
    f.add_option ("last-7-days", _("Last 7 days"), None)
    f.add_option ("last-30-days", _("Last 30 days"), None)
    f.add_option ("last-year", _("Last year"), None);
    filters.add(f)
    f2 = Unity.CheckOptionFilter.new ("type", _("Type"), Gio.ThemedIcon.new("input-keyboard-symbolic"), False)
    f2.add_option ("media", _("Media"), None)
    f2.add_option ("message", _("Message"), None)
    f2.add_option ("presentation", _("Presentation"), None)
    f2.add_option ("spreadsheet", _("Spreadsheet"), None)
    f2.add_option ("text", _("Text"), None)
    f2.add_option ("other", _("Other"), None)
    filters.add (f2)
    return filters

  def do_get_categories(self):
    cats = Unity.CategorySet.new()
    cats.add (Unity.Category.new ('global',
                                  _("Files & Folders"),
                                  Gio.ThemedIcon.new(THEME + "group-folders.svg"),
                                  Unity.CategoryRenderer.VERTICAL_TILE))
    cats.add (Unity.Category.new ('recent',
                                  _("Recent"),
                                  Gio.ThemedIcon.new(THEME + "group-recent.svg"),
                                  Unity.CategoryRenderer.VERTICAL_TILE))
    cats.add (Unity.Category.new ('downloads',
                                  _("Downloads"),
                                  Gio.ThemedIcon.new(THEME + "group-downloads.svg"),
                                  Unity.CategoryRenderer.VERTICAL_TILE))
    cats.add (Unity.Category.new ('folders',
                                  _("Folders"),
                                  Gio.ThemedIcon.new(THEME + "group-folders.svg"),
                                  Unity.CategoryRenderer.VERTICAL_TILE))
    return cats

  def do_create_search_for_query(self, search_context):
    return RecollScopeSearch(search_context, self.db)


class RecollScopeSearch(Unity.ScopeSearchBase):
  __g_type_name__ = "RecollScopeSearch"

  def __init__(self, search_context, db):
    super(RecollScopeSearch, self).__init__()
    self.set_search_context(search_context)
    self._gdocs_accounts = accounts

  def do_run(self):
    context = self.search_context
    filters = context.filter_state
    search_string = context.search_query
    result_set = context.result_set
    print("Search changed to: '%s'" % search_string)
    
    # Get the list of documents
    is_global = context.search_type == Unity.SearchType.GLOBAL
    current_time = time.time()
    if current_time - self.last_connect_time > 10:
      self._connect_db()
      self.last_connect_time = current_time

    # We do not want filters to effect global results
    catgf = ""
    datef = ""
    if not is_global:
      catgf = self.catg_filter(filters)
      datef = self.date_filter(filters)
      " ".join((search_string, catgf, datef))
    
    # Do the recoll thing
    try:
      query = self.db.query()
      nres = query.execute(search_string.decode(self.localecharset))
    except:
      print("recoll query execute error")
      return

    actual_results = 0
    for i in range(nres):
      try:
        doc = query.fetchone()
      except:
        break
      titleorfilename = doc.title
      if titleorfilename == "":
        titleorfilename = doc.filename

      # Results with an ipath get a special mime type so that they
      # get opened by starting a recoll instance.
      mimetype, iconname = self.icon_for_type (doc):

      try:
        abstract = self.db.makeDocAbstract(doc, query).encode('utf-8')
      except:
        break

      # Ok, I don't understand this category thing for now...
      if is_global:
        category = 0
      else:
        if doc.mimetype == "inode/directory" or \
               doc.mimetype = "application/x-fsdirectory":
          category = 3
        else:
          category = 1

      result_set.add_result(
        url,
        icon=iconname,
        category=category,
        result_type=Unity.ResultType.PERSONAL,
        mimetype=mimetype,
        title=titleorfilename,
        comment=abstract,
        dnd_uri=doc.url)

      actual_results += 1
      if actual_results >= MAX_RESULTS:
        break


  def date_filter (self, filters):
    f = filters.get_filter_by_id("modified")
    if f != None:
      o = f.get_active_option()
      if o != None:
        dateopt = ""
        if o.props.id == "last-year":
          dateopt="P365D/"
        elif o.props.id == "last-30-days":
          dateopt = "P30D/"
        elif o.props.id == "last-7-days":
          dateopt = "P7D/"
    return dateopt

  def catg_filter(self, filters):
    f = filters.get_filter_by_id("type")
    if not f: return ""
    if not f.props.filtering:
      return ""
    ss = ""
    # ("media", "message", "presentation", "spreadsheet", "text", "other")
    for ftype in f.options:
        if f.get_option(ftype).props.active:
          ss += " rclcat:" + option.props.id 

    return ss

  # Send back a useful icon depending on the document type
  def icon_for_type (self, doc):
    iconname = "text-x-preview"
    # Results with an ipath get a special mime type so that they
    # get opened by starting a recoll instance.
    thumbnail = None
    if doc.ipath != "":
      mimetype = "application/x-recoll"
      url = doc.url + "#" + doc.ipath
    else:
      mimetype = doc.mimetype
      url = doc.url
      # doc.url is a unicode string which is badly wrong. 
      # Retrieve the binary path for thumbnail access.
      thumbnail = _get_thumbnail_path(doc.getbinurl())

    iconname = None
    if thumbnail:
      iconname = thumbnail
    else:
      if SPEC_MIME_ICONS.has_key(doc.mimetype):
        iconname = SPEC_MIME_ICONS[doc.mimetype]
      else:
        icon = Gio.content_type_get_icon(doc.mimetype)
        if icon:
          # At least on Quantal, get_names() sometimes returns
          # a list with '(null)' in the first position...
          for iname in icon.get_names():
            if iname != '(null)':
              iconname = iname
              break

    return (mimetype, iconname);


def load_scope():
  return RecollScope()
