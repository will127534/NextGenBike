#ifndef _US5881LUA_H_

#define _US5881LUA_H_


uint32_t revolutions;
uint32_t oldTime;
uint32_t rpm;

void virtual_timer_init(void);
uint32_t read_timer(void);
void setup(void);
void loop(void);

#endif