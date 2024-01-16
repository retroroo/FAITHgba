#ifndef STATE_ADVENTURE_H
#define STATE_ADVENTURE_H

#include <gb/gb.h>

extern UBYTE adv_solid_group;
extern UBYTE adv_solid_stick;
extern UBYTE adv_direction_type;
extern WORD adv_vel;
extern WORD adv_strafe_vel;
extern UBYTE adv_acceleration_type;
extern WORD adv_acc;
extern WORD adv_dec;
extern UINT8 game_time;    
extern WORD pl_vel_x;
extern WORD pl_vel_y;

void adventure_init();
void adventure_update();
void adv_deceleration();


#endif
