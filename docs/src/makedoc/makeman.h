#ifndef MAKEMAN_H
#define MAKEMAN_H

extern char manheader[256];
extern char mansynopsis[256];
extern char *man_shortdesc_force1;
extern char *man_shortdesc_force2;

int write_man(char *filename);

#endif
