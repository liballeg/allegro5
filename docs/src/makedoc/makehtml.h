#ifndef MAKEHTML_H
#define MAKEHTML_H

#define HTML_OLD_H_TAG_FLAG         0x00000001
#define HTML_DOCUMENT_TITLE_FLAG    0x00000002
#define HTML_OLD_F_TAG_FLAG         0x00000004
#define HTML_FOOTER_FLAG            0x00000008
#define HTML_SPACED_LI              0x00000010
#define HTML_IGNORE_CSS             0x00000020
#define HTML_OPTIMIZE_FOR_CHM       0x00000040
#define HTML_OPTIMIZE_FOR_DEVHELP   0x00000080


extern int html_flags;
extern char charset[256];
extern const char *html_extension;
extern char *html_document_title;
extern char *html_footer;
extern char *html_see_also_text;
extern char *html_examples_using_this_text;
extern char *html_css_filename;
extern char *html_return_value_text;
extern char *html_text_substitution[256];


int write_html(char *filename);

#endif
