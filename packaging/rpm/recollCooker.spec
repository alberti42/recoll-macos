Summary:	Desktop full text search tool with a qt gui
Name:           recoll
Version:        1.8.1
Release:        %mkrel 1
License:	GPL
Group:          Databases
URL:            http://www.recoll.org/
Source0:	http://www.lesbonscomptes.com/recoll/%{name}-%{version}.tar.bz2
Patch1:		%{name}-configure.patch
BuildRequires:	libxapian-devel
BuildRequires:	libfam-devel
BuildRequires:	libqt-devel	>= 3.3.7
BuildRequires:	libaspell-devel
Requires:	xapian
BuildRoot:      %{_tmppath}/%{name}-%{version}--buildroot

%description
Recoll is a personal full text search tool for Unix/Linux.
It is based on the very strong Xapian backend, for which 
it provides an easy to use, feature-rich, easy administration, 
QT graphical interface.

%prep
%setup -q 
%patch1 -p0

%build
%configure2_5x \
	--with-fam \
	--with-aspell

%make

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%makeinstall_std
desktop-file-install --vendor="" \
	--add-category="X-MandrivaLinux-MoreApplications-Databases" \
	--dir %{buildroot}%{_datadir}/applications %{buildroot}%{_datadir}/applications/*

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files
%defattr(644,root,root,755)
%doc %{_datadir}/%{name}/doc
%attr(755,root,root) %{_bindir}/%{name}*
%{_datadir}/applications/recoll-searchgui.desktop
%{_datadir}/icons/hicolor/48x48/apps/recoll-searchgui.png
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/examples
%dir %{_datadir}/%{name}/filters
%dir %{_datadir}/%{name}/images
%dir %{_datadir}/%{name}/translations
%{_datadir}/%{name}/examples/mime*
%{_datadir}/%{name}/examples/*.conf
%attr(755,root,root) %{_datadir}/%{name}/examples/rclmon.sh
%attr(755,root,root) %{_datadir}/%{name}/filters/rc*
%{_datadir}/%{name}/filters/xdg-open
%{_datadir}/%{name}/images/*png
%{_mandir}/man1/recoll*
%{_mandir}/man5/recoll*
%{_datadir}/%{name}/translations/*.qm


%changelog
* Fri Apr 20 2007 Tomasz Pawel Gajc <tpg@mandriva.org> 1.8.1-1mdv2008.0
+ Revision: 16093
- new version
- drop P0

  + Mandriva <devel@mandriva.com>


* Tue Mar 06 2007 Tomasz Pawel Gajc <tpg@mandriva.org> 1.7.5-2mdv2007.0
+ Revision: 134128
- rebuild

* Tue Jan 30 2007 Tomasz Pawel Gajc <tpg@mandriva.org> 1.7.5-1mdv2007.1
+ Revision: 115423
- add patch 1 - fix build on x86_64
- add patch 0 - fix menu entry
- fix group
- add buildrequires
- set correct bits on files
- Import recoll

