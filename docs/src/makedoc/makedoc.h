#ifndef MAKEDOC_H
#define MAKEDOC_H

/* Some defines */

#define TEXT_FLAG             0x00000001
#define HTML_FLAG             0x00000002
#define HTML_CMD_FLAG         0x00000004
#define TEXINFO_FLAG          0x00000008
#define TEXINFO_CMD_FLAG      0x00000010
#define RTF_FLAG              0x00000020
#define MAN_FLAG              0x00000040
#define HEADING_FLAG          0x00000080
#define DEFINITION_FLAG       0x00000100
#define CONTINUE_FLAG         0x00000200
#define TOC_FLAG              0x00000400
#define INDEX_FLAG            0x00000800
#define NO_EOL_FLAG           0x00001000
#define MULTIFILE_FLAG        0x00002000
#define NOCONTENT_FLAG        0x00004000
#define NO_INDENT_FLAG        0x00008000
#define NODE_FLAG             0x00010000
#define NONODE_FLAG           0x00020000
#define STARTOUTPUT_FLAG      0x00040000
#define ENDOUTPUT_FLAG        0x00080000
#define TALLBULLET_FLAG       0x00100000
#define SHORT_TOC_FLAG        0x00200000
#define XREF_FLAG             0x00400000
#define HEADER_FLAG           0x00800000
#define START_TITLE_FLAG      0x01000000
#define END_TITLE_FLAG        0x02000000
#define STRUCT_FLAG           0x04000000
#define SHORT_DESC_FLAG       0x08000000
#define RETURN_VALUE_FLAG     0x10000000
#define EREF_FLAG             0x20000000
#define CHAPTER_FLAG          0x40000000
#define CHAPTER_END_FLAG      0x80000000

#define TOC_SIZE     8192

/* Structures */

typedef struct LINE
{
   char *text;
   struct LINE *next;
   int flags;
} LINE;


typedef struct TOC
{
   char *text;
   char *alt;
   struct TOC *next;
   int root;
   int texinfoable;
   int htmlable;
   int otherfile;
} TOC;

/* Visible globals */

extern LINE *head;
extern TOC *tochead;
extern int flags;

#endif
