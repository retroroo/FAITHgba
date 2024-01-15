/*General Notes

Next steps:
        By default move speeds are 8, 16, 32, 48, and 56.
        
        I might want to create a different direction definition mapped to a binary sequence so that it is easy to do bitmask operations
        DIR_DOWN = 0, DIR_RIGHT = 1, DIR_UP = 2, DIR_LEFT = 3, DIR_NONE 4
        Actually, I may want two separate variables to store directionality so I can simply multiply by direction.

        Okay, here is the bug:
        It seems like Types 1 and 2 are working correctly
        Type 3 is ignoring the MIN/MAX section, even when it's using straight up numbers with player_vel_x and player_vel_y;
        Type 4 is treating adv_acceleration as a POSITIVE number regardless of whether I'm adding or subtracting it. Ie. as if it were doing an implicit absolute value.

        Other bug:
        Player can currently run off the left side of the screen

*/



#pragma bank 255

#include "data/states_defines.h"
#include "states/adventure.h"

#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "game_time.h"
#include "input.h"
#include "scroll.h"
#include "trigger.h"
#include "data_manager.h"
#include "rand.h"
#include "vm.h"
#include "math.h"

#ifndef ADVENTURE_CAMERA_DEADZONE
#define ADVENTURE_CAMERA_DEADZONE 8
#endif

UBYTE adv_solid_group;
UBYTE adv_solid_stick;
UBYTE adv_direction_type;
WORD adv_vel;
WORD adv_strafe_vel;
UBYTE adv_acceleration_type;
WORD adv_acc;
WORD adv_dec;

WORD delta_x;       //Offset from Solid Actor
WORD delta_y;       //Offset from Solid Actor
WORD temp_y;        //Player's position on the last frame
WORD temp_x;        //Player's position on the last frame
WORD mp_last_x;     //Solid actor's position on the last frame
WORD mp_last_y;     //Solid actor's position on the last frame

actor_t *actor_attached;
direction_e collisionDir;
direction_e new_dir;
UWORD angleVel;
UWORD angleAcc;
WORD player_vel_x;
WORD player_vel_y;

void adventure_init() BANKED {
    // Set camera to follow player
    delta_x = 0;
    delta_y = 0;
    temp_y = 0;
    temp_x = 0;
    mp_last_x = 0;
    mp_last_y = 0;

    //Initialization
    camera_offset_x = 0;
    camera_offset_y = 0;
    camera_deadzone_x = ADVENTURE_CAMERA_DEADZONE;
    camera_deadzone_y = ADVENTURE_CAMERA_DEADZONE;
    actor_attached = NULL;
    collisionDir = DIR_NONE;
    new_dir = DIR_NONE;


    //Basic Calculation from Engine Settings
    if (!adv_acceleration_type){
        adv_acc = adv_vel;
        adv_dec = adv_vel;
    }

    angleVel = (adv_vel/22) << 4;
    angleAcc = (adv_acc/22) << 4;


}

void adventure_update() BANKED {
    actor_t *hit_actor;
    UBYTE tile_start, tile_end;
    player_moving = FALSE;


    /////////////////////////////////
    //    MOVEMENT
    /////////////////////////////////

    //Switch to organize fundamental types
    switch (adv_direction_type){
        //Four Directional Movement
        case 0:
            new_dir = DIR_NONE; 
            if (INPUT_LEFT && INPUT_RECENT_LEFT){
                player_moving = TRUE;
                new_dir = DIR_LEFT;
                player_vel_x -= adv_acc;
                player_vel_x = MAX(player_vel_x, -adv_vel);
            } else if (INPUT_RIGHT && INPUT_RECENT_RIGHT){
                player_moving = TRUE;
                new_dir = DIR_RIGHT;
                player_vel_x += adv_acc;
                player_vel_x = MIN(player_vel_x, adv_vel);
            } else if (INPUT_UP && INPUT_RECENT_UP){
                player_moving = TRUE;
                new_dir = DIR_UP;
                player_vel_y -= adv_acc;
                player_vel_y = MAX(player_vel_y, -adv_vel);
            } else if (INPUT_DOWN && INPUT_RECENT_DOWN){
                player_moving = TRUE;
                new_dir = DIR_DOWN;
                player_vel_y += adv_acc;
                player_vel_y = MIN(player_vel_y, adv_vel);
            }
            adv_deceleration();
        break;
        //Normal GBS Adventure Movement
        case 1:
            new_dir = DIR_NONE; 
            if (INPUT_RECENT_LEFT) {
                new_dir = DIR_LEFT;
            } else if (INPUT_RECENT_RIGHT) {
                new_dir = DIR_RIGHT;
            } else if (INPUT_RECENT_UP) {
                new_dir = DIR_UP;
            } else if (INPUT_RECENT_DOWN) {
                new_dir = DIR_DOWN;
            }

            if (INPUT_LEFT) {
                player_moving = TRUE;
                if (INPUT_UP) {
                    player_vel_x -= angleAcc;
                    player_vel_x = MAX(player_vel_x, -angleVel);
                    player_vel_y -= angleAcc;
                    player_vel_y = MAX(player_vel_y, -angleVel);

                } else if (INPUT_DOWN) {
                    player_vel_x -= angleAcc;
                    player_vel_x = MAX(player_vel_x, -angleVel);
                    player_vel_y += angleAcc;
                    player_vel_y = MIN(player_vel_y, angleVel);
                } else {
                    player_vel_x -= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_vel);
                }
            } else if (INPUT_RIGHT) {
                player_moving = TRUE;
                if (INPUT_UP) {
                    player_vel_x += angleAcc;
                    player_vel_x = MIN(player_vel_x, angleVel);
                    player_vel_y -= angleAcc;
                    player_vel_y = MAX(player_vel_y, -angleVel);
                } else if (INPUT_DOWN) {
                    player_vel_x += angleAcc;
                    player_vel_x = MIN(player_vel_x, angleVel);
                    player_vel_y += angleAcc;
                    player_vel_y = MIN(player_vel_y, angleVel);
                } else {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_vel);
                }
            } else if (INPUT_UP) {
                player_moving = TRUE;
                player_vel_y -= adv_acc;
                player_vel_y = MAX(player_vel_y, -adv_vel);


            } else if (INPUT_DOWN) {
                player_moving = TRUE;
                player_vel_y += adv_acc;
                player_vel_y = MIN(player_vel_y, adv_vel);
            }
            adv_deceleration();
        break;
        case 2: // Strafing Movement 
            if (INPUT_LEFT && (new_dir == DIR_LEFT || new_dir == DIR_NONE)){
                new_dir = DIR_LEFT;
                player_moving = TRUE;
                if (INPUT_UP) {
                    player_vel_x -= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_vel);
                    player_vel_y -= adv_acc;
                    player_vel_y = MAX(player_vel_y, -adv_strafe_vel);
                } else if (INPUT_DOWN) {
                    player_vel_x-= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_vel);
                    player_vel_y += adv_strafe_vel;
                    player_vel_y = MIN(player_vel_y, adv_strafe_vel);
                } else {
                    player_vel_x-= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_vel);
                }
            } else if (INPUT_RIGHT && (new_dir == DIR_RIGHT || new_dir == DIR_NONE)){
                new_dir = DIR_RIGHT;
                player_moving = TRUE;
                if (INPUT_UP) {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_vel);
                    player_vel_y -= adv_acc;
                    player_vel_y = MAX(player_vel_y, -adv_strafe_vel);

                } else if (INPUT_DOWN) {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_vel);
                    player_vel_y += adv_acc;
                    player_vel_y = MIN(player_vel_y, adv_strafe_vel);
                } else {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_vel);
                }
            } else if (INPUT_UP && (new_dir == DIR_UP || new_dir == DIR_NONE)){
                new_dir = DIR_UP;
                player_moving = TRUE;
                if (INPUT_LEFT) {
                    player_vel_x -= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_strafe_vel);
                    player_vel_y -= adv_acc;
                    player_vel_y = MAX(player_vel_y, -adv_vel);
                } else if (INPUT_RIGHT) {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_strafe_vel);
                    player_vel_y -= adv_acc;
                    player_vel_y = MAX(player_vel_y,-adv_vel);
                } else {
                    player_vel_y -= adv_acc;
                    player_vel_y = MAX(player_vel_y, -adv_vel);
                }
            }else if (INPUT_DOWN && (new_dir == DIR_DOWN || new_dir == DIR_NONE)){
                new_dir = DIR_DOWN;
                player_moving = TRUE;
                if (INPUT_LEFT) {
                    player_vel_x -= adv_acc;
                    player_vel_x = MAX(player_vel_x, -adv_strafe_vel);
                    player_vel_y += adv_acc;
                    player_vel_y = MIN(player_vel_y, adv_vel);
                } else if (INPUT_RIGHT) {
                    player_vel_x+= adv_acc;
                    player_vel_x = MIN(player_vel_x, adv_strafe_vel);
                    player_vel_y += adv_acc;
                    player_vel_y = MIN(player_vel_y, adv_vel);
                } else {
                    player_vel_y += adv_acc;
                    player_vel_y = MIN(player_vel_y, adv_vel);
                }
            } else{
                new_dir = DIR_NONE;
            }
            adv_deceleration();
        break;
        //Isometric Movement
        case 3:
            new_dir = DIR_NONE; 
            if (INPUT_LEFT && INPUT_RECENT_LEFT){
                //Left and down by half
                player_moving = TRUE;
                new_dir = DIR_LEFT;
                player_vel_x -= adv_acc;
                player_vel_x = MAX(player_vel_x, -adv_vel);
                player_vel_y += adv_acc;
                player_vel_y = MIN(player_vel_y, adv_vel >> 1);
            } else if (INPUT_RIGHT && INPUT_RECENT_RIGHT){
                player_moving = TRUE;
                new_dir = DIR_RIGHT;
                player_vel_x += adv_acc;
                player_vel_x = MIN(player_vel_x, adv_vel);
                player_vel_y -= adv_acc;
                player_vel_y = MAX(player_vel_y, -(adv_vel>>1));
            } else if (INPUT_UP && INPUT_RECENT_UP){
                player_moving = TRUE;
                new_dir = DIR_UP;
                player_vel_x -= adv_acc;
                player_vel_x = MAX(player_vel_x, -(adv_vel >> 1));
                player_vel_y -= adv_acc;
                player_vel_y = MAX(player_vel_y, -adv_vel);
            } else if (INPUT_DOWN && INPUT_RECENT_DOWN){
                player_moving = TRUE;
                new_dir = DIR_DOWN;
                player_vel_x += adv_acc;
                player_vel_x = MIN(player_vel_x, adv_vel >> 1);
                player_vel_y += adv_acc;
                player_vel_y = MIN(player_vel_y, adv_vel);
            } else {
                //DECELERATION
                if( adv_acceleration_type == 1){
                    adv_deceleration();
                } else {
                    player_vel_x = 0;
                    player_vel_y = 0;
                }
            }


        break;
    }







    ///////////////////////////
    //   SOLID ACTOR OFFSET
    ///////////////////////////
    if (collisionDir != DIR_NONE){
        WORD delta_mp_x = actor_attached->pos.x - mp_last_x;
        WORD delta_mp_y = actor_attached->pos.y - mp_last_y;
        mp_last_x = actor_attached->pos.x;
        mp_last_y = actor_attached->pos.y;

        if (collisionDir == DIR_DOWN && delta_mp_y > player_vel_y){
            //PLAYER IS GOING UP
            player_vel_y = delta_mp_y;
            if (adv_solid_stick){
                //Pushing into a solid actor, so freeze other movements and directions
                player_vel_x= 0;
                new_dir = DIR_UP;
            }
        } else if (collisionDir == DIR_UP && delta_mp_y < player_vel_y){
            //PLAYER IS GOING DOWN
            player_vel_y = delta_mp_y;
            if (adv_solid_stick){
                player_vel_x= 0;
                new_dir = DIR_DOWN;
            }
        } else if (collisionDir == DIR_LEFT && delta_mp_x < player_vel_x){
            player_vel_x= delta_mp_x;
            if (adv_solid_stick){
                player_vel_y = 0;
                new_dir = DIR_RIGHT;
            }
        } else if (collisionDir == DIR_RIGHT && delta_mp_x > player_vel_x){
            player_vel_x= delta_mp_x;
            if (adv_solid_stick){
                player_vel_y = 0;
                new_dir = DIR_LEFT;
            }
        }

        
    } 

    ///////////////////////////
    //    COLLISION
    ///////////////////////////
    WORD new_x = 0;
    WORD new_y = 0;
    temp_x = PLAYER.pos.x;
    temp_y = PLAYER.pos.y;

    
    // Step X
    new_x = PLAYER.pos.x + delta_x + (player_vel_x >> 8);
    tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
    tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
    if (new_x > (WORD)PLAYER.pos.x) {
        UBYTE tile_x = ((new_x >> 4) + PLAYER.bounds.right) >> 3;
        while (tile_start != tile_end) {

            if (tile_at(tile_x, tile_start) & COLLISION_LEFT) {
                new_x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;           
                break;
            }
            tile_start++;
        }
        PLAYER.pos.x = MIN((image_width - 16) << 4, new_x);
    } else {
        UBYTE tile_x = ((new_x >> 4) + PLAYER.bounds.left) >> 3;
        while (tile_start != tile_end) {
            if (tile_at(tile_x, tile_start) & COLLISION_RIGHT) {
                new_x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;         
                break;
            }
            tile_start++;
        }
        PLAYER.pos.x = MAX(10, new_x);
    }
    
    // Step Y
    new_y += PLAYER.pos.y + delta_y + (player_vel_y >> 8);
    tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
    tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
    if (new_y > PLAYER.pos.y) {
        UBYTE tile_y = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;
        while (tile_start != tile_end) {
            if (tile_at(tile_start, tile_y) & COLLISION_TOP) {
                new_y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;
                break;
            }
            tile_start++;
        }
        PLAYER.pos.y = new_y;
    } else {
        UBYTE tile_y = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
        while (tile_start != tile_end) {
            if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {
                new_y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                break;
            }
            tile_start++;
        }
        PLAYER.pos.y = MAX(16, new_y);
    }
    

    if (new_dir != DIR_NONE) {
        actor_set_dir(&PLAYER, new_dir, player_moving);
    } else {
        actor_set_anim_idle(&PLAYER);
    }

    
    hit_actor = NULL;
    // Check for trigger collisions
    if (trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, FALSE)) {
        // Landed on a trigger
        return;
    }

    // Check for actor collisions
    delta_x = 0;
    delta_y = 0;
    hit_actor = actor_overlapping_player(FALSE);
    if (hit_actor != NULL && hit_actor->collision_group) {
        //Solid Actors
        if (hit_actor->collision_group == adv_solid_group){
            if(hit_actor != actor_attached){
                //ONLY USED FOR INITIAL POSITIONING
                actor_attached = hit_actor;
                mp_last_x = hit_actor->pos.x;
                mp_last_y = hit_actor->pos.y;
                if (temp_y + (PLAYER.bounds.bottom << 4) < (hit_actor->pos.y + (hit_actor->bounds.top << 4))){
                    delta_y += (hit_actor->pos.y + (hit_actor->bounds.top <<4)) - (PLAYER.pos.y + (PLAYER.bounds.bottom << 4));
                    collisionDir = DIR_UP;
                } else if (temp_y + (PLAYER.bounds.top << 4) > hit_actor->pos.y + (hit_actor->bounds.bottom<<4)){
                    delta_y += (hit_actor->pos.y + 8 + (hit_actor->bounds.bottom<<4)) - (PLAYER.pos.y + (PLAYER.bounds.top<<4));
                    collisionDir = DIR_DOWN;
                } else if (temp_x + (PLAYER.bounds.right << 4) < hit_actor->pos.x +(hit_actor->bounds.left << 4)){
                    delta_x += (hit_actor->pos.x + (hit_actor->bounds.left<<4)) - (PLAYER.pos.x + (PLAYER.bounds.right<<4));
                    collisionDir = DIR_LEFT;
                } else if (temp_x + (PLAYER.bounds.left << 4) > hit_actor->pos.x + (hit_actor->bounds.right << 4)){
                    delta_x += (hit_actor->pos.x + 8 + (hit_actor->bounds.right<<4)) - (PLAYER.pos.x + (PLAYER.bounds.left<<4));
                    collisionDir = DIR_RIGHT;
                } else {
                    collisionDir = hit_actor->dir;
                    //(hit_actor->pos.y + 8 + (hit_actor->bounds.bottom<<4)) - (PLAYER.pos.y + (PLAYER.bounds.top<<4))
                    //delta_y = hit_actor->pos.y - PLAYER.pos.y;
                }
            }
        }
        //All Other Collisions
        player_register_collision_with(hit_actor);
    } else {
            actor_attached = NULL;
            collisionDir = DIR_NONE;
            mp_last_x = NULL;
            mp_last_y = NULL;
        }



    if (INPUT_A_PRESSED) {
        if (!hit_actor) {
            hit_actor = actor_in_front_of_player(8, TRUE);
        }
        if (hit_actor && !hit_actor->collision_group && hit_actor->script.bank) {
            script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
        }
    }
}

void adv_deceleration() BANKED {
    ///////////////////////////
    //  Deceleration
    ///////////////////////////
    if (!INPUT_LEFT && !INPUT_RIGHT){
        if (player_vel_x > adv_dec){
            player_vel_x -= adv_dec;
        } else if (player_vel_x < -adv_dec){
            player_vel_x += adv_dec;
        } else {player_vel_x = 0;}
    }
    if (!INPUT_UP && !INPUT_DOWN){
        if (player_vel_y > adv_dec){
            player_vel_y -= adv_dec;
        } else if (player_vel_y < -adv_dec){
            player_vel_y += adv_dec;
        } else {player_vel_y = 0;}
    }    
}
