#ifndef FONTRESOURCE_HPP
#define FONTRESOURCE_HPP

class FontResource : public Resource
{
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   FontResource(const char *filename);
private:
   A5O_FONT *font;
   std::string filename;
};

#endif // FONTRESOURCE_HPP
