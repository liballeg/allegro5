#ifndef WAVE_HPP
#define WAVE_HPP

class Wave
{
public:
   static Wave& getInstance(void);
   
   void init(void);
   bool next(void);

   ~Wave(void);
private:
   static Wave *wave;
   Wave(void);

   bool beginWave(void);

   int waveNum;
   int rippleNum;
//   Script script;
//   lua_State *luaState;
};

#endif // WAVE_HPP

