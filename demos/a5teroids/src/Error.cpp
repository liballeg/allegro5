#include "a5teroids.hpp"

std::string& Error::getMessage(void)
{
   return message;
}

Error::Error(const char* message)
{
   this->message = std::string(message);
}

