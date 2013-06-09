
import sys
import subprocess
import time
import urllib
import hashlib
import os
import locale
from gi.repository import Unity, GObject, Gio

try:
    from recoll import rclconfig
    hasrclconfig = True
except:
    hasrclconfig = False

try:
    from recoll import recoll
    from recoll import rclextract
    hasextract = True
except:
    import recoll
    hasextract = False

BUS_PATH = "/org/recoll/unitylensrecoll/scope/main"

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

    
# Icon names for some recoll mime types which don't have standard icon by the
# normal method
SPEC_MIME_ICONS = {'application/x-fsdirectory' : 'gnome-fs-directory.svg',
                   'inode/directory' : 'gnome-fs-directory.svg',
                   'message/rfc822' : 'mail-read',
                   'application/x-recoll' : 'recoll'}

# Truncate results here:
MAX_RESULTS = 30

# These category ids must match the order in which we add them to the lens
CATEGORY_ALL = 0

# typing timeout: we don't want to start a search for every
# char? Unity does batch on its side, but we may want more control ?
# Or not ? I'm not sure this does any good on a moderate size index.
# Set to 0 to not use it (default). Kept around because this still might be
# useful with a very big index ?
TYPING_TIMEOUT = 0

class Scope (Unity.Scope):

    def __init__ (self):
        Unity.Scope.__init__ (self, dbus_path=BUS_PATH)
        
        # Listen for changes and requests
        self.connect ("activate-uri", self.activate_uri)
        if Unity._version == "4.0":
            #print "Setting up for Unity 4.0"
            self.connect("notify::active-search",
                     self._on_search_changed)
            self.connect("notify::active-global-search",
                     self._on_global_search_changed)
            self.connect("filters-changed",
                     self._on_search_changed);
        else:
            #print "Setting up for Unity 5.0+"
            self.connect ("search-changed", self._on_search_changed)
            self.connect ("filters-changed",
                      self._on_filters_changed)

        self.last_connect_time = 0
        self.timeout_id = None
        language, self.localecharset = locale.getdefaultlocale()
        if hasrclconfig:
            self.config = rclconfig.RclConfig()

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

    def get_search_string (self):
        search = self.props.active_search
        return search.props.search_string if search else None
    
    def get_global_search_string (self):
        search = self.props.active_global_search
        return search.props.search_string if search else None
    
    def search_finished (self):
        search = self.props.active_search
        if search:
            search.emit("finished")
    
    def global_search_finished (self):
        search = self.props.active_global_search
        if search:
            search.emit("finished")

    def reset (self):
        self._do_browse (self.props.results_model)
        self._do_browse (self.props.global_results_model)
    
    def _on_filters_changed (self, scope):
        #print "_on_filters_changed()"
        self.queue_search_changed(Unity.SearchType.DEFAULT)
    
    if Unity._version == "4.0":
        def _on_search_changed (self, scope, param_spec=None):
            search_string = self.get_search_string()
            results = scope.props.results_model
            #print "Search 4.0 changed to: '%s'" % search_string
            self._update_results_model (search_string, results)
    else:
        def _on_search_changed (self, scope, search, search_type, cancellable):
            search_string = search.props.search_string
            results = search.props.results_model
            #print "Search 5.0 changed to: '%s'" % search_string
            if search_string:
                self._update_results_model (search_string, results)
            else:
                search.props.results_model.clear()
    
    def _on_global_search_changed (self, scope, param_spec):
        search = self.get_global_search_string()
        results = scope.props.global_results_model
        #print "Global search changed to: '%s'" % search
        self._update_results_model (search, results)
    
    def _update_results_model (self, search_string, model):
        if search_string:
            self._do_search (search_string, model)
        else:
            self._do_browse (model)

    def _do_browse (self, model):
        if self.timeout_id is not None:
            GObject.source_remove(self.timeout_id)
        model.clear ()

        if model is self.props.results_model:
            self.search_finished()
        else:
            self.global_search_finished()

    def _on_timeout(self, search_string, model):
        if self.timeout_id is not None:
            GObject.source_remove(self.timeout_id)
        self.timeout_id = None
        self._really_do_search(search_string, model)
        if model is self.props.results_model:
            self.search_finished()
        else:
            self.global_search_finished()

    def _do_search (self, search_string, model):
        if TYPING_TIMEOUT == 0:
            self._really_do_search(search_string, model)
            return True

        if self.timeout_id is not None:
            GObject.source_remove(self.timeout_id)
        self.timeout_id = \
            GObject.timeout_add(TYPING_TIMEOUT, self._on_timeout, 
                    search_string, model)

    def _really_do_search(self, search_string, model):
        #print "really_do_search:", "[" + search_string + "]"

        model.clear ()
        if search_string == "":
            return True

        current_time = time.time()
        if current_time - self.last_connect_time > 10:
            self._connect_db()
            self.last_connect_time = current_time

        if not self.db:
            model.append ("",
                      "error",
                      CATEGORY_ALL,
                      "text/plain",
                  "You need to use the recoll GUI to create the index first !",
                      "",
                      "")
            return
        fcat = self.get_filter("rclcat")
        for option in fcat.options:
            if option.props.active:
                search_string += " rclcat:" + option.props.id 

        # Do the recoll thing
        try:
            query = self.db.query()
            nres = query.execute(search_string.decode(self.localecharset))
        except:
            return

        actual_results = 0
        for i in range(nres):
            try:
                doc = query.fetchone()
            except:
                break

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

            titleorfilename = doc.title
            if titleorfilename == "":
                titleorfilename = doc.filename

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

            #print "iconname(%s) = %s" % (doc.mimetype, iconname)

            try:
                abstract = self.db.makeDocAbstract(doc, query).encode('utf-8')
            except:
                break

            model.append (url,
                          iconname,
                          CATEGORY_ALL,
                          mimetype,
                          titleorfilename,
                          abstract,
                          doc.url)

            actual_results += 1
            if actual_results >= MAX_RESULTS:
                break
        

    # If we return from here, the caller gets an error:
    #  Warning: g_object_get_qdata: assertion `G_IS_OBJECT (object)' failed
    # Known bug, see:
    #   https://bugs.launchpad.net/unity/+bug/893688
    # Then, the default url activation takes place
    # which is not at all what we want.
    # First workaround:
    #    In the recoll case, we just exit, the lens will be restarted.
    #    In the regular case, we return, and activation works exactly once for
    #    2 calls on oneiric and mostly for precise...
    # New workaround, suggested somewhere on the net and kept: other
    # construction method
    def activate_uri (self, scope, uri):
        """Activation handler for uri"""
        
        #print "Activate: %s" % uri
        
        # Pass all uri without fragments to the desktop handler
        if uri.find("#") == -1:
            # Reset browsing state when an app is launched
            if Unity._version == "4.0":
                self.reset ()
            ret = Unity.ActivationResponse(handled=Unity.HandledType.NOT_HANDLED,
                                                goto_uri=uri)
            return ret
        
        # Pass all others to recoll
        proc = subprocess.Popen(["recoll", uri])
        #print "Subprocess returned, going back to unity"

        scope.props.results_model.clear();
        scope.props.global_results_model.clear();
        # Old workaround:
        #sys.exit(0)

        # New and better:
        # The goto_uri thing is a workaround suggested somewhere instead of
        # passing the string. Does fix the issue
        #return Unity.ActivationResponse.new(Unity.HandledType.HIDE_DASH,''
        ret = Unity.ActivationResponse(handled=Unity.HandledType.HIDE_DASH,
                                       goto_uri='')
        return ret
