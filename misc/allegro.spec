# RPM spec file for Allegro.

Summary: A game programming library.
Summary(es): Una libreria de programacion de juegos.
Summary(fr): Une librairie de programmation de jeux.
Summary(it): Una libreria per la programmazione di videogiochi.
Name: allegro
Version: 4.0.0
Release: 1
License: Gift Ware
Packager: Allegro development team <conductors@canvaslink.com>
Group: System/Libraries
Source: ftp://sunsite.dk/allegro/%{name}-%{version}.tar.gz
URL: http://alleg.sourceforge.net
# If you don't have the icon, just comment it out.
Icon: alex.xpm
Buildroot: %{_tmppath}/%{name}-buildroot
# Older rpms don't support this; just make sure you have it.
#BuildRequires: texinfo
# Automatic dependency generation picks up module dependencies
# which is exactly what we don't want...
Autoreq: off
Requires: /sbin/ldconfig, /sbin/install-info
Requires: ld-linux.so.2, libc.so.6, libdl.so.2, libm.so.6
Requires: libpthread.so.0, libX11.so.6, libXext.so.6
Requires: libc.so.6(GLIBC_2.0), libc.so.6(GLIBC_2.1), libc.so.6(GLIBC_2.1.3)
Requires: libc.so.6(GLIBC_2.2), libdl.so.2(GLIBC_2.0), libdl.so.2(GLIBC_2.1)
Requires: libpthread.so.0(GLIBC_2.0), libpthread.so.0(GLIBC_2.1)

%description
Allegro is a cross-platform library intended for use in computer games
and other types of multimedia programming.

%description -l es
Allegro es una librería multi-plataforma creada para ser usada en la
programación de juegos u otro tipo de programación multimedia.

%description -l fr
Allegro est une librairie multi-plateforme destinée à être utilisée dans
les jeux vidéo ou d'autres types de programmation multimédia.

%description -l it
Allegro è una libreria multipiattaforma dedicata all'uso nei videogiochi
ed in altri tipi di programmazione multimediale.

%package devel
Summary: A game programming library.
Summary(es): Una libreria de programacion de juegos.
Summary(fr): Une librairie de programmation de jeux.
Summary(it): Una libreria per la programmazione di videogiochi.
Group: Development/C
Prereq: allegro
Autoreq: on

%description devel
Allegro is a cross-platform library intended for use in computer games
and other types of multimedia programming. This package is needed to
build programs written with Allegro.

%description devel -l es
Allegro es una librería multi-plataforma creada para ser usada en la
programación de juegos u otro tipo de programación multimedia. Este
paquete es necesario para compilar los programas que usen Allegro.

%description devel -l fr
Allegro est une librairie multi-plateforme destinée à être utilisée dans
les jeux vidéo ou d'autres types de programmation multimédia. Ce package
est nécessaire pour compiler les programmes utilisant Allegro.

%description devel -l it
Allegro è una libreria multipiattaforma dedicata all'uso nei videogiochi
ed in altri tipi di programmazione multimediale. Questo pacchetto è
necessario per compilare programmi scritti con Allegro.

%package tools
Summary: Extra tools for the Allegro programming library.
Summary(es): Herramientas adicionales para la librería de programación Allegro.
Summary(fr): Outils supplémentaires pour la librairie de programmation Allegro.
Summary(it): Programmi di utilità aggiuntivi per la libreria Allegro.
Group: Development/Other
Prereq: allegro
Autoreq: on

%description tools
Allegro is a cross-platform library intended for use in computer games
and other types of multimedia programming. This package contains extra
tools which are useful for developing Allegro programs.

%description tools -l es
Allegro es una librería multi-plataforma creada para ser usada en la
programación de juegos u otro tipo de programación multimedia. Este
paquete contiene herramientas adicionales que son útiles para
desarrollar programas que usen Allegro.

%description tools -l fr
Allegro est une librairie multi-plateforme destinée à être utilisée dans
les jeux vidéo ou d'autres types de programmation multimédia. Ce package
contient des outils supplémentaires qui sont utiles pour le développement
de programmes avec Allegro.

%description tools -l it
Allegro è una libreria multipiattaforma dedicata all'uso nei videogiochi
ed in altri tipi di programmazione multimediale. Questo pacchetto
contiene programmi di utilità aggiuntivi utili allo sviluppo di programmi
con Allegro.

%prep
%setup -q

%build
./configure --enable-shared \
            --enable-static \
	    --enable-pentiumopts \
	    --prefix=%{_prefix} \
            --mandir=%{_mandir} \
            --infodir=%{_infodir}
make
MKDATA_PRELOAD=../../lib/unix/liballeg-%{version}.so DAT=../../tools/dat misc/mkdata.sh

%install
rm -rf %{buildroot}
# If your rpm doesn't automatically compress documentation, you can
# use install-gzipped-man and install-gzipped-info.
make prefix=%{buildroot}%{_prefix} \
     mandir=%{buildroot}%{_mandir} \
     infodir=%{buildroot}%{_infodir} \
     install \
     install-man \
     install-info
install -D -m 644 allegro.cfg %{buildroot}%{_sysconfdir}/allegrorc
install -d -m 755 %{buildroot}%{_datadir}/allegro
install -D -m 644 keyboard.dat language.dat %{buildroot}%{_datadir}/allegro
find demo examples setup -type f -perm +111 -print | xargs rm

%post
/sbin/ldconfig
 
%postun
/sbin/ldconfig

%post devel
install-info %{_infodir}/allegro.info* %{_infodir}/dir

%postun devel
install-info --delete allegro %{_infodir}/dir

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc readme.txt docs/build/unix.txt docs/build/linux.txt
%doc AUTHORS CHANGES THANKS
%config(noreplace) %{_sysconfdir}/allegrorc
%{_libdir}/*.so
%{_libdir}/allegro
%{_datadir}/allegro

%files devel
%defattr(-,root,root)
%doc docs/txt/abi.txt docs/txt/ahack.txt docs/txt/allegro.txt
%doc docs/txt/const.txt docs/txt/faq.txt docs/txt/help.txt
%doc todo.txt docs/html
%doc demo examples setup
%{_bindir}/allegro-config
%{_libdir}/*.a
%{_includedir}/*
%{_infodir}/allegro.info*
%{_mandir}/man3/*

%files tools
%defattr(-,root,root)
%doc tools/grabber.txt
%doc docs/makedoc.c
%{_bindir}/colormap
%{_bindir}/dat
%{_bindir}/dat2s
%{_bindir}/exedat
%{_bindir}/grabber
%{_bindir}/pack
%{_bindir}/pat2dat
%{_bindir}/rgbmap
%{_bindir}/textconv

%changelog
* Fri Dec 07 2001 Angelo Mottola <lillo@users.sourceforge.net>  4.0.0-1
- added italian translation

* Tue Oct 02 2001 Peter Wang <tjaden@users.sourceforge.net>  3.9.39-1
- icon courtesy of Johan Peitz

* Mon Sep 24 2001 Peter Wang <tjaden@users.sourceforge.net>
- remaining translations by Eric Botcazou and Grzegorz Adam Hankiewicz

* Sun Sep 23 2001 Peter Wang <tjaden@users.sourceforge.net>
- translations by Eric Botcazou and Javier González
- language.dat and keyboard.dat moved to main package
- devel split into devel and tools packages
- makedoc added to tools package

* Wed Sep 16 2001 Peter Wang <tjaden@users.sourceforge.net>
- merged Osvaldo's spec file with gfoot's spec and some other changes

* Wed Sep 27 2000 Osvaldo Santana Neto <osvaldo@conectiva.com>
- updated to 3.9.33
