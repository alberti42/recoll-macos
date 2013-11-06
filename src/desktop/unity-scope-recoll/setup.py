#!/usr/bin/env python
#
from distutils.core import setup
from DistUtilsExtra.command import build_extra
import subprocess
#import build_i18n_ext as build_i18n

setup(name="unity-scope-recoll",
      version="0.1",
      author="Jean-Francois Dockes",
      author_email="jfd@recoll.corg",
      url="http://www.recoll.org",
      license="GNU General Public License v3 (GPLv3)",
      data_files=[
    ('share/unity-scopes/recoll', ['unity_recoll_daemon.py']),
    ('share/applications', ['unity-scope-recoll.desktop']),
    ('share/dbus-1/services', ['unity-scope-recoll.service']),
    ('share/icons/hicolor/48x48/apps', ['unity-scope-recoll.png']),
    ('share/unity/scopes/files', ['recoll.scope']),
    ], cmdclass={"build":  build_extra.build_extra,})
