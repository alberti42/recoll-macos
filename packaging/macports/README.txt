To use/test the port out of the official macports tree:

- Edit sources.conf /opt/local/etc/macports/sources.conf, and insert a URL
  pointing to your local repository before the rsync one:
    file:///Users/dockes/projets/fulltext/recoll/packaging/macports
    rsync://rsync.macports.org/release/ports [default]

  (inserting before ensures it's used before the macports one)

- The port should live under category/portname (ie: textproc/

- After you create or update your Portfile, use the MacPorts portindex
  command in the local repository's directory to create or update the index
  of the ports in your local repository.

    %% cd ~/path/to/macports
    %% portindex

  Once the local port is added to the PortIndex, it becomes available for
  searching or installation as with any other Portfile in the MacPorts
  tree

http://guide.macports.org/#development.local-repositories

