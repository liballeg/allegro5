#ifndef GENERICRESOURCE_HPP
#define GENERICRESOURCE_HPP

class GenericResource : public Resource
{
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   GenericResource(void *data, void (*destroy)(void *));
private:
   void (*destroy_func)(void *);
   void *data;
};

#endif // GENERICRESOURCE_HPP
