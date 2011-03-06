%define name kio_recoll
%define version 1.15.6
%define release 0

Name:           %{name}
Version:        %{version}
Release:        %{release}

Summary:	KIO slave for the Recoll full text search tool
Source0:	http://www.recoll.org/recoll-%{version}.tar.gz
URL:            http://www.recoll.org/
Group:          Applications/Databases

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot
License:	GPL

BuildRequires: libkde4-devel zlib-devel-static libxapian-devel libuuid-devel
Requires: recoll

%description
Recoll is a personal full text search package for Linux, FreeBSD and
other Unix systems. It is based on a very strong backend (Xapian), for
which it provides an easy to use, feature-rich, easy administration
interface.

# ---------------------------------------------------------------------------

%prep
%setup -q -n recoll-%{version}

# ---------------------------------------------------------------------------

%build

%cmake_kde4 kde/kioslave/kio_recoll
pwd
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
%{_libdir}/kde4/kio_recoll.so
%{_datadir}/kde4/apps/kio_recoll
%{_datadir}/kde4/apps/kio_recoll/help.html
%{_datadir}/kde4/apps/kio_recoll/welcome.html
%{_datadir}/kde4/services/recoll.protocol
%{_datadir}/kde4/services/recollf.protocol
%if 0%{?suse_version} > 1120
%dir %{_datadir}/kde4/apps
%dir %{_datadir}/kde4/services
%dir %{_libdir}/kde4
%endif

# ---------------------------------------------------------------------------

%changelog
* Sun Mar 06 2011 Jean-Francois Dockes <jfd@recoll.org> 1.15.5-0
- Initial spec file for kio
