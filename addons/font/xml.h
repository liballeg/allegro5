typedef enum {
   Outside,
   ElementName,
   AttributeName,
   AttributeStart,
   AttributeValue
} XmlState;

void _al_xml_parse(ALLEGRO_FILE *f,
        int (*callback)(XmlState state, char const *value, void *u),
        void *u);
