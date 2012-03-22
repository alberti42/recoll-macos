
import sys
from gi.repository import GLib, GObject, Gio
from gi.repository import Unity

import recoll

# These category ids must match the order in which we add them to the lens
CATEGORY_ALL = 0

class Scope (Unity.Scope):

	def __init__ (self):
		Unity.Scope.__init__ (self, dbus_path="/org/recoll/unitylensrecoll/scope/main")
		
		# Listen for changes and requests
		self.connect ("notify::active-search", self._on_search_changed)
		self.connect ("notify::active-global-search", self._on_global_search_changed)
		self.connect ("activate-uri", self.activate_uri)

		# Bliss loaded the apps_tree menu here, let's connect to 
                # the index
                self.db = recoll.connect()
		
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
		
		# Defer all activations to Unity
                # Reset browsing state when an app is launched
                self.reset ()
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
		self.search_finished()
	
	def _do_search (self, search_string, model):
		model.clear ()
		
		# Reset browsing mode
		self._current_browse_node = None
		
                # Do the recoll thing
                query = self.db.query()
                nres = query.execute(search_string)
                if nres > 20:
                        nres = 20
                while query.next >= 0 and query.next < nres: 
                        doc = query.fetchone()
                        titleorfilename = doc.title
                        if titleorfilename == "":
                                titleorfilename = doc.filename
			model.append (doc.url,
			              "",
			              CATEGORY_ALL,
			              doc.mimetype,
			              titleorfilename,
			              titleorfilename,
			              doc.url)
		
