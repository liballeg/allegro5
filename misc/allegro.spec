Summary:        A game programming library.
Summary(es):    Una libreria de programacion de juegos.
Name:           allegro
Version:        3.9.33
Release:        1
Copyright:      Gift Ware
Packager:       George Foot <george.foot@merton.oxford.ac.uk>
Group:          Development/Libraries
Source:         allegro-3.9.33.tar.gz
Buildroot:      /var/tmp/allegro-buildroot

%description
     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 3.9.33 (CVS)

Allegro is a cross-platform library intended for use in computer games
and other types of multimedia programming. It was initially conceived
on the Atari ST, but that platform sadly died during childbirth. 
After a brief stay with Borland C, it was adopted by the fantastic djgpp
compiler, where it grew to maturity.

%description -l es
     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 3.9.33 (CVS)

Allegro es una librería multi-plataforma creada para ser usada en la
programación de juegos u otro tipo de programación multimedia.
Fue concebida inicialmente en el Atari ST, pero esa plataforma murió
tristemente durante su nacimiento. Tras un corto paso por Borland C, fue
adoptada por el fantástico compilador djgpp, donde creció hasta la
madurez.

%install
mkdir -p $RPM_BUILD_ROOT/usr/local/src
cp $RPM_SOURCE_DIR/allegro-3.9.33.tar.gz $RPM_BUILD_ROOT/usr/local/src/allegro-3.9.33.tar.gz

%post
cd /usr/local/src
rm -rf allegro-3.9.33
gunzip -cd allegro-3.9.33.tar.gz | tar -xf -
rm -f allegro-3.9.33.tar.gz
cd allegro-3.9.33
CFLAGS="$RPM_OPT_FLAGS" ./configure
make depend
make
make install
make install-man
make install-info

%preun
cd /usr/local/src
make -C allegro-3.9.33 uninstall
rm -rf allegro-3.9.33
touch allegro-3.9.33.tar.gz

%files
/usr/local/src/allegro-3.9.33.tar.gz

