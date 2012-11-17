%define name recoll
%define version 1.18.1
%define release  %mkrel 1

Name:           %{name}
Version:        %{version}
Release:        %{release}

Summary:	Desktop full text search tool with a qt gui
Source0:	http://www.recoll.org/%{name}-%{version}.tar.gz
URL:            http://www.recoll.org/
Group:          Applications/Databases

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot
License:	GPL
BuildRequires:  libqt4-devel
BuildRequires:  libqtwebkit2.2-devel
BuildRequires:  libpython-devel
BuildRequires:  libxapian-devel

%description
Recoll is a personal full text search package for Linux, FreeBSD and
other Unix systems. It is based on a very strong backend (Xapian), for
which it provides an easy to use, feature-rich, easy administration
interface.

# ---------------------------------------------------------------------------

%prep
%setup -q 

# ---------------------------------------------------------------------------

%build
[ -n "$QTDIR" ] || . %{_sysconfdir}/profile.d/60qt4.sh
%configure
make %{?_smp_mflags}

# ---------------------------------------------------------------------------

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

# ---------------------------------------------------------------------------

%clean
rm -rf $RPM_BUILD_ROOT

# ---------------------------------------------------------------------------

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/applications/recoll-searchgui.desktop
%{_datadir}/icons/hicolor/48x48/apps/recoll.png
%{_datadir}/pixmaps/recoll.png
%{_datadir}/%{name}
%{_mandir}/man1/recoll*
%{_mandir}/man5/recoll*
%{python_sitearch}/Recoll*.egg-info
%{python_sitearch}/recoll.so

# ---------------------------------------------------------------------------

%changelog
* Mon Nov 05 2012 Jean-Francois Dockes <jfd@recoll.org> 1.18.1-1
- Update to release 1.18.1
* Thu May 17 2012 Jean-Francois Dockes <jfd@recoll.org> 1.17.2-1
- Update to release 1.17.2
* Mon Nov 07 2011 Jean-Francois Dockes <jfd@recoll.org> 1.16.2-1
- Update to release 1.16.2
* Wed Sep 28 2011 Jean-Francois Dockes <jfd@recoll.org> 1.16.1-1
- Update to release 1.16.1
* Wed Sep 21 2011 Jean-Francois Dockes <jfd@recoll.org> 1.16.0-1
- Update to release 1.16.0
* Tue May 03 2011 Jean-Francois Dockes <jfd@recoll.org> 1.15.8-1
- Update to release 1.15.8
* Fri Mar 04 2011 Jean-Francois Dockes <jfd@recoll.org> 1.15.5-1
- Update to release 1.15.5
* Thu Feb 15 2011 Jean-Francois Dockes <jfd@recoll.org> 1.15.2-1
- Update to release 1.15.2
* Wed Feb 02 2011 Jean-Francois Dockes <jfd@recoll.org> 1.15.0-1
- Update to release 1.15.0
* Thu Nov 25 2010 Jean-Francois Dockes <jfd@recoll.org> 1.14.3-1
- Update to release 1.14.2
* Sat Sep 24 2010 Jean-Francois Dockes <jfd@recoll.org> 1.14.2-1
- Update to release 1.14.2
* Mon Sep 13 2010 Jean-Francois Dockes <jfd@recoll.org> 1.14.0-1
- Update to release 1.14.0
* Thu Apr 14 2010 Jean-Francois Dockes <jfd@recoll.org> 1.13.04-1
- Update to release 1.13.01
* Thu Jan 07 2010 Jean-Francois Dockes <jfd@recoll.org> 1.13.01-1
- Update to release 1.13.01
* Thu Dec 10 2009 Jean-Francois Dockes <jfd@recoll.org> 1.12.4-1
- Update to release 1.12.2
* Mon Oct 19 2009 Jean-Francois Dockes <jfd@recoll.org> 1.12.2-1
- Update to release 1.12.2
* Thu Jan 29 2009 Jean-Francois Dockes <jfd@recoll.org> 1.12.0-1
- Update to release 1.12.0
* Mon Oct 13 2008 Jean-Francois Dockes <jfd@recoll.org> 1.11.0-1
- Update to release 1.11.0
* Thu May 27 2008 Jean-Francois Dockes <jfd@recoll.org> 1.10.2-1
- Update to release 1.10.2
* Thu Jan 31 2008 Jean-Francois Dockes <jfd@recoll.org> 1.10.1-1
- Update to release 1.10.1
* Wed Nov 21 2007 Jean-Francois Dockes <jfd@recoll.org> 1.10.0-1
- Update to release 1.10.0
* Tue Sep 11 2007 Jean-Francois Dockes <jfd@recoll.org> 1.9.0-1
- Update to release 1.9.0
* Tue Mar 6 2007 Jean-Francois Dockes <jfd@recoll.org> 1.8.1-1
- Update to release 1.8.1
* Mon Jan 15 2007 Jean-Francois Dockes <jfd@recoll.org> 1.7.5-1
- Update to release 1.7.5
* Mon Jan 08 2007 Jean-Francois Dockes <jfd@recoll.org> 1.7.3-1
- Update to release 1.7.3
* Tue Nov 28 2006 Jean-Francois Dockes <jfd@recoll.org> 1.6.1-1
- Update to release 1.6.1
* Mon Nov 20 2006 Jean-Francois Dockes <jfd@recoll.org> 1.5.11-1
- Update to release 1.5.11
* Mon Oct 2 2006 Jean-Francois Dockes <jfd@recoll.org> 1.5.2-1
- Update to release 1.5.2
* Sun May 7 2006 Jean-Francois Dockes <jfd@recoll.org> 1.4.3-1
- Update to release 1.4.3
* Fri Mar 31 2006 Jean-Francois Dockes <jfd@recoll.org> 1.3.3-1
- Update to release 1.3.3
* Thu Feb  2 2006 Jean-Francois Dockes <jfd@recoll.org> 1.2.2-1
- Update to release 1.2.2
* Thu Jan 10 2006 Jean-Francois Dockes <jfd@recoll.org> 1.1.0-1
- Initial packaging
