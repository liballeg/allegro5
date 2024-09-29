typedef enum {
   Outside,
   ElementName,
   AttributeName,
   AttributeStart,
   AttributeValue
} XmlState;

void _al_xml_parse(A5O_FILE *f,
        int (*callback)(XmlState state, char const *value, void *u),
        void *u);
