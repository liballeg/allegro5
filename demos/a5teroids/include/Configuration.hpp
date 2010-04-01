#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "a5teroids.hpp"

class Configuration
{
public:
   static Configuration& getInstance(void);

   void destroy(void);
   void clear(void);
   bool read(void);
   bool write(void);

   std::string& getVersion(void);
   bool isFullscreen(void);
   bool debuggingEnabled(void);
private:
   static Configuration* cfg;

   Configuration();

   static std::string version;
   static bool fullscreen;
   static bool debugging;
};

#endif // CONFIG_HPP


