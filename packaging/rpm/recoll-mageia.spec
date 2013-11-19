Name:   		recoll
Version:    	1.19.9
Release:    	%mkrel 1
Summary:    	Desktop full text search tool with a Qt GUI
URL:    		http://www.lesbonscomptes.com/recoll/
Group:  		Databases
License:    	GPLv2

Source0:    	http://www.lesbonscomptes.com/recoll/%{name}-%{version}.tar.gz

BuildRequires:	xapian-devel
BuildRequires:	pkgconfig(QtCore)
BuildRequires:	pkgconfig(QtWebKit)

Requires:   	xapian


%description
Recoll is a personal full text search package for Linux, FreeBSD and
other Unix systems. It is based on a very strong backend (Xapian), for
which it provides an easy to use, feature-rich, easy administration
interface.


%package -n kio-%{name}
Summary:    	Kioslave for %{name}
Group:		    Graphical desktop/KDE
BuildRequires:	cmake
BuildRequires:	kdelibs4-devel
Requires:       %{name} = %{version}-%{release}
Requires:       kdebase4-workspace

%description -n kio-%{name}
Kioslave for %{name}. Enables to perform querries and extract
results in konqueror and dolphin.



%prep
%setup -q

%build
%configure2_5x --disable-python-module
%make


%install
%makeinstall_std

pushd kde/kioslave/kio_recoll
%cmake_kde4
%make
%makeinstall_std
popd


%files
%doc README
%{_bindir}/*
%{_datadir}/applications/recoll-searchgui.desktop
%{_datadir}/icons/hicolor/48x48/apps/recoll.png
%{_datadir}/pixmaps/recoll.png
%{_datadir}/%{name}
%{_mandir}/man1/recoll*
%{_mandir}/man5/recoll*
%{_kde_libdir}/%name/librecoll.so.%version


%files -n kio-%{name}
%{_kde_libdir}/kde4/kio_recoll.so
%{_kde_datadir}/apps/kio_recoll/help.html
%{_kde_datadir}/apps/kio_recoll/welcome.html
%{_kde_datadir}/kde4/services/recoll.protocol
%{_kde_datadir}/kde4/services/recollf.protocol
