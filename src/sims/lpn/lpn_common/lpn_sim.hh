#ifndef __LPN_SIM__
#define __LPN_SIM__
#include <bits/stdint-uintn.h>
#include "place_transition.hh"
#define LOOP_TS(func, t_size) for(int i=0;i < t_size; i++){ \
        Transition* t = t_list[i]; \
        func; \
    } 
    
void TransitionCountLog(Transition* t_list[], int size);

void TransitionResetCount(Transition* t_list[], int size);

void PlaceTokensLog(Transition* t_list[], int size);

uint64_t NextCommitTime(Transition* t_list[], int size);

uint64_t NextCommitTimeFast(Transition* t_list[], int size);

int CommitAtTime(Transition* t_list[], int size, uint64_t time);

// need to let outside world to update lpn clk
void UpdateClk(Transition* t_list[], int size, uint64_t clk);
#endif