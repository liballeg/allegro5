#ifndef MAKEHTML_H
#define MAKEHTML_H

#define HTML_OLD_H_TAG_FLAG         0x00000001
#define HTML_DOCUMENT_TITLE_FLAG    0x00000002
#define HTML_OLD_F_TAG_FLAG         0x00000004
#define HTML_FOOTER_FLAG            0x00000008
#define HTML_SPACED_LI              0x00000010
#define HTML_IGNORE_CSS             0x00000020


extern int html_flags;
extern char charset[256];
extern const char *html_extension;
extern char *document_title;
extern char *html_footer;


int write_html(char *filename);

#endif
