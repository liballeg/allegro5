#ifndef __included_make_doc_h
#define __included_make_doc_h

#include "dawk.h"

extern dstr pandoc;
extern dstr pandoc_options;
extern dstr protos_file;
extern dstr to_format;
extern bool raise_sections;
extern char tmp_preprocess_output[80];
extern char tmp_pandoc_output[80];

#define streq(a, b) (0 == strcmp((a), (b)))

const char *lookup_prototype(const char *name);
extern void call_pandoc(const char *input, const char *output,
    const char *extra_options);
extern void make_single_doc(int argc, char *argv[]);
extern void make_man_pages(int argc, char *argv[]);

#endif

/* vim: set sts=3 sw=3 et: */
