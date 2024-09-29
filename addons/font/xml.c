#include "allegro5/allegro.h"
#include <ctype.h>

#include "xml.h"

typedef struct {
   XmlState state;
   bool closing;
   A5O_USTR *value;
   int (*callback)(XmlState state, char const *value, void *u);
   void *u;
} XmlParser;

static void scalar(XmlParser *x) {
   x->callback(x->state, al_cstr(x->value), x->u);
   al_ustr_assign_cstr(x->value, "");
}

static void opt_scalar(XmlParser *x) {
   if (al_ustr_size(x->value)) {
      scalar(x);
   }
}

static void discard_scalar(XmlParser *x) {
   al_ustr_assign_cstr(x->value, "");
}

static void close_tag(XmlParser *x) {
   x->state = Outside;
   x->closing = false;
}

static void add_char(XmlParser *x, char c) {
   char s[] = {c, 0};
   al_ustr_append_cstr(x->value, s);
}

static void create_tag(XmlParser *x) {
   scalar(x);
}

static void open_tag(XmlParser *x) {
   x->state = Outside;
}

void _al_xml_parse(A5O_FILE *f,
        int (*callback)(XmlState state, char const *value, void *u),
        void *u)
{
   XmlParser x_;
   XmlParser *x = &x_;
   x->value = al_ustr_new("");
   x->state = Outside;
   x->closing = false;
   x->callback = callback;
   x->u = u;

   while (true) {
      int c = al_fgetc(f);
      if (c < 0) {
         break;
      }
      if (x->state == Outside) {
         if (c == '<') {
            opt_scalar(x);
            x->state = ElementName;
            continue;
         }
      }
      else if (x->state == ElementName) {
         if (c == '/') {
            x->closing = true;
            continue;
         }
         else if (c == '>') {
            if (x->closing) {
               discard_scalar(x);
               close_tag(x);
            }
            else {
               create_tag(x);
               open_tag(x);
            }
            continue;
         }
         else if (isspace(c)) {
            create_tag(x);
            x->state = AttributeName;
            continue;
         }
      }
      else if (x->state == AttributeName) {
         if (isspace(c)) {
            continue;
         }
         else if (c == '/') {
            x->closing = true;
            continue;
         }
         else if (c == '?') {
            x->closing = true;
            continue;
         }
         else if (c == '>') {
            if (x->closing) {
               close_tag(x);
            }
            else {
               open_tag(x);
            }
            continue;
         }
         else if (c == '=') {
            scalar(x);
            x->state = AttributeStart;
            continue;
         }
      }
      else if (x->state == AttributeStart) {
         if (c == '"') {
            x->state = AttributeValue;
         }
         continue;
      }
      else if (x->state == AttributeValue) {
         if (c == '"') {
            scalar(x);
            x->state = AttributeName;
            continue;
         }
      }
      add_char(x, c);
   }
   
   al_fclose(f);
   al_ustr_free(x->value);
}
