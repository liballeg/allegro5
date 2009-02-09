#ifndef ERROR_HPP
#define ERROR_HPP

#include "a5teroids.hpp"

class Error {
public:
   std::string& getMessage(void);
   Error(const char* message);
private:
   std::string message;
};

class ScriptError : public Error
{
public:
   ScriptError(const char* message) : Error(message) {}
};

class ReadError : public Error
{
public:
   ReadError(const char* message) : Error(message) {}
};

class WriteError : public Error
{
public:
   WriteError(const char* message) : Error(message) {}
};

class FileNotFoundError : public Error
{
public:
   FileNotFoundError(const char* message) : Error(message) {}
};

#endif // ERROR_HPP


