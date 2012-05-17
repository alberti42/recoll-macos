Summary:        Desktop full text search tool with Qt GUI
Name:           recoll
Version:        1.17.2
Release:        1%{?dist}
Group:          Applications/Databases
License:        GPLv2+
URL:            http://www.lesbonscomptes.com/recoll/
Source0:        http://www.lesbonscomptes.com/recoll/recoll-%{version}.tar.gz
BuildRequires:  qt-devel
BuildRequires:  qtwebkit-devel
BuildRequires:  python-devel
BuildRequires:  zlib-devel
BuildRequires:  aspell-devel
BuildRequires:  xapian-core-devel
BuildRequires:  desktop-file-utils
Requires:       xdg-utils
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Recoll is a personal full text search package for Linux, FreeBSD and
other Unix systems. It is based on a very strong back end (Xapian), for
which it provides an easy to use, feature-rich, easy administration
interface.

%prep
%setup -q
# remove execute bit
%{__chmod} 0644 utils/{conftree.cpp,conftree.h,debuglog.cpp,debuglog.h}

%build
export QMAKE=qmake-qt4
%configure
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR=%{buildroot} STRIP=/bin/true INSTALL='install -p'

desktop-file-install --delete-original \
  --dir=%{buildroot}/%{_datadir}/applications \
  %{buildroot}/%{_datadir}/applications/%{name}-searchgui.desktop

# use /usr/bin/xdg-open
%{__rm} -f %{buildroot}/usr/share/recoll/filters/xdg-open

%post
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ] ; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor
fi
if [ -x %{_bindir}/update-desktop-database ] ; then
  %{_bindir}/update-desktop-database &> /dev/null
fi
exit 0

%postun
touch --no-create %{_datadir}/icons/hicolor 
if [ -x %{_bindir}/gtk-update-icon-cache ] ; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor
fi
if [ -x %{_bindir}/update-desktop-database ] ; then
  %{_bindir}/update-desktop-database &> /dev/null
fi
exit 0

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, -)
%doc COPYING ChangeLog README
%{_bindir}/%{name}
%{_bindir}/%{name}index
%{_datadir}/%{name}
%{_datadir}/applications/%{name}-searchgui.desktop
%{_datadir}/icons/hicolor/48x48/apps/%{name}.png
%{_datadir}/pixmaps/%{name}.png
%{python_sitearch}/Recoll*.egg-info
%{python_sitearch}/recoll.so
%{_mandir}/man1/%{name}.1*
%{_mandir}/man1/%{name}index.1*
%{_mandir}/man5/%{name}.conf.5*

%changelog
* Thu May 17 2012 J.F. Dockes <jf@dockes.org> - 1.17.2-1
- 1.17.2

* Sat Mar 31 2012 Terje Rosten <terje.rosten@ntnu.no> - 1.17.1-1
- 1.17.1

* Sun Mar 25 2012 Terje Rosten <terje.rosten@ntnu.no> - 1.17-1
- 1.17

* Tue Feb 28 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.16.2-3
- Rebuilt for c++ ABI breakage

* Wed Feb 15 2012 Terje Rosten <terje.rosten@ntnu.no> - 1.16.2-2
- Add patch to build with gcc 4.7

* Wed Feb 01 2012 Terje Rosten <terje.rosten@ntnu.no> - 1.16.2-1
- 1.16.2

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.16.1-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Mon Oct 24 2011 Terje Rosten <terje.rosten@ntnu.no> - 1.16.1-2
- Add patch to fix crash (bz #747472)

* Tue Oct 18 2011 Terje Rosten <terje.rosten@ntnu.no> - 1.16.1-1
- 1.16.1

* Tue May 24 2011 Terje Rosten <terje.rosten@ntnu.no> - 1.15.8-2
- add patch from upstream to fix crash.

* Sun May 08 2011 Terje Rosten <terje.rosten@ntnu.no> - 1.15.8-1
- 1.15.8

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.14.4-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sun Jan 28 2011 Terje Rosten <terje.rosten@ntnu.no> - 1.14.4-1
- 1.14.4

* Mon Nov 15 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.14.2-2
- Add patch to fix #631704

* Sun Nov  7 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.14.2-1
- 1.14.2

* Thu Jul 15 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.13.04-6
- add patch to build with xapian 1.2 (from J.F. Dockes, thanks)

* Sat Jul 10 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.13.04-5
- use system xdg-open
- trim chagenlog

* Fri Jul  9 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.13.04-4
- fix some review comments

* Mon May 10 2010 Terje Rosten <terje.rosten@ntnu.no> - 1.13.04-3
- use version macro in source url
- don't strip bins, giving non empty debuginfo package
- more explicit file listing
- try to preserve timestamps 
- use proper name for spec file
- fix changelog
- license seems to be GPLv2+
- include some %%doc files
- add buildroot tag
- some source add execute bit set
- explicit enable inotify
- add aspell-devel to bulldreq
- make with _smp_mflags seems to work
- add post scripts
- add patch to build gui with correct flags (ref: 338791)

* Sun May  9 2010  J.F. Dockes <jfd@recoll.org> 1.13.04-2
- bumped the release number to issue new rpms for fc10

* Sun May  9 2010  J.F. Dockes 1.13.04
- updated to recoll release 1.13.04 

* Fri Feb 12 2010 Terry Duell 1.13.02
- updated to release 1.13.02

* Mon Jan 12 2010 Terry Duell  1.13.01-3
- updated to fix Fedora desktop-file-install and install icon

* Sun Jan 10 2010 Jean-Francois Dockes <jfd@recoll.org> 1.13.01-2
- updated for recent fedoras: depend on xapian packages, use qt4

* Thu Jan 07 2010 Jean-Francois Dockes <jfd@recoll.org> 1.13.01-1
- update to release 1.13.01

* Wed Feb  1 2006 Jean-Francois Dockes <jfd@recoll.org> 1.2.0-1
- initial packaging
