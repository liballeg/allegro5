#ifndef __included_make_doc_h
#define __included_make_doc_h

#include "dawk.h"

extern dstr pandoc;
extern dstr pandoc_options;
extern dstr protos_file;
extern dstr to_format;
extern bool raise_sections;
extern dstr tmp_preprocess_output;
extern dstr tmp_pandoc_output;

#define streq(a, b) (0 == strcmp((a), (b)))

const char *lookup_prototype(const char *name);
const char *lookup_source(const char *name);
const char *lookup_example(const char *name);
const char* example_source(dstr buffer, const char *file_name, const char *line_number);
extern void call_pandoc(const char *input, const char *output,
    const char *extra_options);
extern void make_single_doc(int argc, char *argv[]);
extern void make_man_pages(int argc, char *argv[]);
char *load_allegro5_cfg(void);

#endif

/* vim: set sts=3 sw=3 et: */
