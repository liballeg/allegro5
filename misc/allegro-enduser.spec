Summary:        A game programming library (end user distribution)
Summary(es):    Una libreria de programacion de juegos.
Summary(fr):    Une librairie de programmation de jeux.
Name:           allegro-enduser
Version:        3.9.35
Release:        1
Copyright:      Gift Ware
Packager:       George Foot <george.foot@merton.oxford.ac.uk>
Group:          Development/Libraries
Source:         allegro-%{version}-enduser.tar.gz
Buildroot:      %{_tmppath}/allegro-buildroot

%description
     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 3.9.35 (CVS)

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
				       \_/__/     Version 3.9.35 (CVS)

Allegro es una librería multi-plataforma creada para ser usada en la
programación de juegos u otro tipo de programación multimedia.
Fue concebida inicialmente en el Atari ST, pero esa plataforma murió
tristemente durante su nacimiento. Tras un corto paso por Borland C, fue
adoptada por el fantástico compilador djgpp, donde creció hasta la
madurez.

%description -l fr
     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 3.9.35 (CVS)

Allegro est une librairie multi-plateforme destinée à être utilisée dans
les jeux vidéo ou d'autres types de programmation multimédia. Elle fut
initialement conçue sur l'Atari ST, mais cette plateforme mourut
tristement durant son enfantement. Après un bref passage par Borland C,
elle fut adoptée par le fantastique compilateur djgpp, avec lequel elle
atteignit sa maturité.

%install
mkdir -p $RPM_BUILD_ROOT/usr/local/src
cp $RPM_SOURCE_DIR/allegro-%{version}-enduser.tar.gz $RPM_BUILD_ROOT/usr/local/src/allegro-%{version}-enduser.tar.gz

%post
cd /usr/local/src
rm -rf allegro-%{version}
gunzip -cd allegro-%{version}-enduser.tar.gz | tar -xf -
rm -f allegro-%{version}-enduser.tar.gz
cd allegro-%{version}
CFLAGS="$RPM_OPT_FLAGS" ./configure
make lib
make install
echo "#!/bin/sh" > /usr/local/bin/allegro-uninstall
make uninstall --dry-run >> /usr/local/bin/allegro-uninstall
echo "rm -f /usr/local/bin/allegro-uninstall" >> /usr/local/bin/allegro-uninstall
chmod 700 /usr/local/bin/allegro-uninstall
cd ..
rm -rf allegro-%{version}

%preun
/usr/local/bin/allegro-uninstall
cd /usr/local/src
touch allegro-%{version}-enduser.tar.gz

%files
/usr/local/src/allegro-%{version}-enduser.tar.gz

