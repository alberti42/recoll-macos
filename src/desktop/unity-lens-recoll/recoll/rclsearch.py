#
# IDEAS AND COMMENTS
#
# - We should probably cache all App instances on startup to avoid recreating
#   themon each search. Listen to the "changed" signal on the GMenu tree to
#   update
#
# - Matching and scoring algorithm is really lame. Could be more efficient,
#   and calculate the scores better
#
# - Bear in mind when looking at this code that this is *meant* to be simple :-)
#

import sys
from gi.repository import GLib, GObject, Gio
from gi.repository import Dee
from gi.repository import GMenu
from gi.repository import Unity

# These category ids must match the order in which we add them to the lens
CATEGORY_ALL = 0

class App:
	"""Simple representation of an item in the result model.
	   This class implements simple comparisons, sorting, and hashing
	   methods to integrate with the Python routines for these things
	"""
	def __init__ (self, uri, icon, cat_id, mime,
	              display_name, comment, dnd_uri, score = 0):
		self.uri = uri
		self.icon = icon
		self.cat_id = cat_id
		self.mime = mime
		self.display_name = display_name
		self.comment = comment
		self.dnd_uri = dnd_uri
		self.score = score
	
	def __lt__ (self, other):
		if self.score :
			return self.score > other.score # scores sort reverse
		return self.display_name < other.display_name
	
	def __gt__ (self, other):
		if other.score :
			return self.score < other.score # scores sort reverse
		return self.display_name < other.display_name
	
	def __eq__ (self, other):
		return self.uri == other.uri
	
	def __ne__ (self, other):
		return self.uri != other.uri
		
	def __hash__ (self):
		return hash(self.uri)

class Scope (Unity.Scope):

	def __init__ (self):
		Unity.Scope.__init__ (self, dbus_path="/net/launchpad/unitylensbliss/scope/main")
		
		# Listen for changes and requests
		self.connect ("notify::active-search", self._on_search_changed)
		self.connect ("notify::active-global-search", self._on_global_search_changed)
		self.connect ("activate-uri", self.activate_uri)
		
		self.apps_tree = GMenu.Tree.new ("unity-lens-bliss.menu", GMenu.TreeFlags.NONE)
		self.apps_tree.load_sync ()
		
		self._current_browse_node = None
	
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
	
	def activate_uri (self, scope, uri):
		"""Activation handler to enable browsing of app directories"""
		
		print "Activate: %s" % uri
		
		# For all else than appdir:// uris we defer launching to Unity
		if not uri.startswith ("appdir://"):
			# Reset browsing state when an app is launched
			self.reset ()
			return Unity.ActivationResponse.new (Unity.HandledType.NOT_HANDLED,
			                                     uri)
		
		if self._current_browse_node is None:
			print >> sys.stderr, "Error: Unexpected browsing request for %s", uri
			return Unity.ActivationResponse.new (Unity.HandledType.NOT_HANDLED,
			                                     uri)
		
		# Extract directory id from the uri and find the corresponding
		# item in the currently browsed node
		menu_id = uri[9:]
		
		if menu_id == "..":
			self._current_browse_node = self._current_browse_node.get_parent ()
			self._do_browse (self.props.results_model) # FIXME: Broken for global search
			return Unity.ActivationResponse.new (Unity.HandledType.SHOW_DASH,
			                                     uri)
		
		tree_iter = self._current_browse_node.iter ()
		item_type = tree_iter.next ();
		while (item_type != GMenu.TreeItemType.INVALID):
			if item_type == GMenu.TreeItemType.DIRECTORY:
				d = tree_iter.get_directory ()
				if menu_id == d.get_menu_id ():
					self._current_browse_node = d
					self._do_browse (self.props.results_model) # FIXME: Broken for global search
					return Unity.ActivationResponse.new (Unity.HandledType.SHOW_DASH,
			                                             uri)
			else:
				pass
			item_type = tree_iter.next ();
		
		# We didn't find the expected folder. Reset the browsing state
		self.reset ()
		print >> sys.stderr, "Error: Unable to browse appdir '%s'", menu_id
		return Unity.ActivationResponse.new (Unity.HandledType.NOT_HANDLED,
		                                     uri)
	
	def reset (self):
		self._current_browse_node = None
		self._do_browse (self.props.results_model)
		self._do_browse (self.props.global_results_model)
	
	def _on_search_changed (self, scope, param_spec):
		search = self.get_search_string()
		results = scope.props.results_model
		
		print "Search changed to: '%s'" % search
		
		self._update_results_model (search, results)
		self.search_finished()
	
	def _on_global_search_changed (self, scope, param_spec):
		search = self.get_global_search_string()
		results = scope.props.global_results_model
		
		print "Global search changed to: '%s'" % search
		
		self._update_results_model (search, results)
		self.global_search_finished()
		
	def _update_results_model (self, search_string, model):
		if search_string:
			self._do_search (search_string, model)
		else:
			self._do_browse (model)
	
	def _do_browse (self, model):
		model.clear ()
		
		collector = set ()
		
		if self._current_browse_node is None:
			self._current_browse_node = self.apps_tree.get_root_directory ()
		elif self._current_browse_node.get_parent ():
			self._collect_back_dir (collector)
		
		tree_iter = self._current_browse_node.iter ()
		item_type = tree_iter.next ();
		while (item_type != GMenu.TreeItemType.INVALID):
			if item_type == GMenu.TreeItemType.ENTRY:
				self._collect_app (collector, tree_iter.get_entry ())
			elif item_type == GMenu.TreeItemType.DIRECTORY:
				self._collect_dir (collector, tree_iter.get_directory ())
			else:
				pass # we skip separators and other stuff
			item_type = tree_iter.next ();
		
		# Sort alphabetically
		results = sorted (collector)
		
		for app in results:
			model.append (app.uri,
			              app.icon,
			              app.cat_id,
			              app.mime,
			              app.display_name,
			              app.comment,
			              app.dnd_uri)
	
	def _do_search (self, search_string, model):
		model.clear ()
		
		# Reset browsing mode
		self._current_browse_node = None
		
		# Use a set to collect results to avoid duplicates
		# Apps are compared by app.uri to make this work
		collector = set ()
		self._search_visit_directory (search_string,
		                              collector,
		                              self.apps_tree.get_root_directory ())
		# Sort by score
		results = sorted (collector)
		
		for app in results:	
			model.append (app.uri,
			              app.icon,
			              app.cat_id,
			              app.mime,
			              app.display_name,
			              app.comment,
			              app.dnd_uri)
		
	
	def _search_visit_directory (self, search_string, collector, tree_dir):
		tree_iter = tree_dir.iter ()
		item_type = tree_iter.next ();
		while (item_type != GMenu.TreeItemType.INVALID):
			if item_type == GMenu.TreeItemType.ENTRY:
				self._search_visit_entry (search_string,
				                          collector,
				                          tree_iter.get_entry ())
			elif item_type == GMenu.TreeItemType.DIRECTORY:
				self._search_visit_directory (search_string,
				                              collector,
				                              tree_iter.get_directory())
			else:
				pass # we skip separators and other stuff
			item_type = tree_iter.next ();
	
	def _search_visit_entry (self, search_string, collector, tree_entry):
		if not search_string:
			self._collect_app (collector, tree_entry)
			return
		
		app_info = tree_entry.get_app_info ()		
		search_string = search_string.lower().strip()
		
		score = 1.0
		score *= self._test_string (search_string,
		                            app_info.get_description (),
		                            1.1)
		
		score *= self._test_string (search_string,
		                            app_info.get_display_name (),
		                            2.0)
		
		score *= self._test_string (search_string,
		                            app_info.get_name (),
		                            2.0)
		                            
		score *= self._test_string (search_string,
		                            app_info.get_executable (),
		                            3.0)
		
		if score > 1.0:
			self._collect_app (collector, tree_entry, score)
	
	def _test_string (self, search_string, target_string, boost):
		"""Tests if search_string matches target_string and returns a
		   boost factor calculated from the input boost and the quality of
		   the match.
		   Returns 1.0 on no match"""
		if not target_string:
			return 1
		
		target_string = target_string.lower()
		
		if target_string.startswith(search_string):
			return boost * 2
		elif search_string in target_string:
			return boost
		
		return 1
	
	def _collect_app (self, collector, tree_entry, score = 0):
		app_info = tree_entry.get_app_info ()
		app = App ("application://" + tree_entry.get_desktop_file_id (), # uri
		           app_info.get_icon().to_string (),            # string formatted GIcon
		           CATEGORY_ALL,                                # numeric category id
		           "application/x-desktop",                     # mimetype
		           app_info.get_display_name () or "???",       # display name
		           app_info.get_description () or "No description", # comment
		           tree_entry.get_desktop_file_path () or "",   # dnd uri
		           score)                                       # score
		
		collector.add (app)
	
	def _collect_dir (self, collector, tree_dir, score = 0):
		# Create folder icon with app category emblem if the dir has an icon,
		# else just use a folder icon
		if tree_dir.get_icon ():
			emblem = Gio.Emblem.new (tree_dir.get_icon ())
			icon = Gio.EmblemedIcon.new (Gio.ThemedIcon.new ("folder"),
		                                 emblem)
		else:
			emblem = Gio.Emblem.new (Gio.ThemedIcon.new ("gtk-execute"))
			icon = Gio.EmblemedIcon.new (Gio.ThemedIcon.new ("folder"),
		                                 emblem)
		
		app = App ("appdir://" + tree_dir.get_menu_id (),       # uri
		           icon.to_string (),                           # string formatted GIcon
		           CATEGORY_ALL,                                # numeric category id
		           "application/x-desktop",                     # mimetype
		           tree_dir.get_name () or "???",               # display name
		           tree_dir.get_comment () or "No description", # comment
		           tree_dir.get_desktop_file_path () or "",   # dnd uri
		           score)                                       # score
		
		collector.add (app)
	
	def _collect_back_dir (self, collector):
		icon = Gio.ThemedIcon.new ("gtk-go-back-ltr")
		app = App ("appdir://..",                               # uri
		           icon.to_string (),                           # string formatted GIcon
		           CATEGORY_ALL,                                # numeric category id
		           "application/x-desktop",                     # mimetype
		           "Back",                                      # display name
		           "Back to previous application directory",    # comment
		           "",                                          # dnd uri
		           100000)                                      # score
		
		collector.add (app)

