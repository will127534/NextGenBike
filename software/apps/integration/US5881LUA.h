#ifndef _US5881LUA_H_

#define _US5881LUA_H_


// uint32_t revolutions;
uint32_t oldTime;
uint32_t rpm;
uint32_t degreeWidth;
uint32_t degreeCount;
uint32_t picWidth;

void virtual_timer_init(void);
void timer_start(void);
uint32_t read_timer(void);
void setup(void);
// void loop(void);

#endif