#ifndef SCRIPT_H
#define SCRIPT_H

#include "a5teroids.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#define SCRIPT_EXT "lua"

class Script {
public:
   void clear(void);
   bool create(bool readGlobalScript);
   bool source(const char* filename) throw (FileNotFoundError);
   void destroy(void);
   lua_State *getLuaState(void);

   bool getBool(char* name) throw (ScriptError);
   double getNumber(char* name) throw (ScriptError);
   const char* getString(char* name) throw (ScriptError);

   Script();
   ~Script();
private:
   bool created;
   lua_State *luaState;
};

void dumpLuaStack(lua_State *l);
double getNumberFromTable(lua_State *luaState, const char *name);

#endif // SCRIPT_H

