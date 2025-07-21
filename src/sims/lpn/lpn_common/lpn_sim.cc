#include "lpn_sim.hh"
#include <bits/stdint-uintn.h>
#include <string>
#include <deque>
static std::deque<uint64_t> ordered_fire_times; 

uint64_t NextCommitTimeFast(Transition* t_list[], int size){
    if(ordered_fire_times.empty()){
      LOOP_TS(trigger(t, 0), size);
      fire_time_list(t_list, size, ordered_fire_times);
    }

    if(ordered_fire_times.empty()){
      return lpn::LARGE;
    }

    uint64_t ret = ordered_fire_times.front();
    ordered_fire_times.pop_front();
    return ret;
}

uint64_t NextCommitTime(Transition* t_list[], int size){

    // to get update data
    LOOP_TS(trigger(t, 0), size);
    return min_time_g(t_list, size);
}

int CommitAtTime(Transition* t_list[], int size, uint64_t time){
    for(int i=0; i < size; i++){
        Transition* t = t_list[i];
        sync(t, time);
        // int fired = sync(t, time);
        // if(fired == 1){
        //     printf("@%ld sync t done: %s\n", time/1000, t->id.c_str());
        // }
    } 
    return 0;
}

void UpdateClk(Transition* t_list[], int size, uint64_t clk){
  for(int i=0; i<size; i++){
      Transition* t = t_list[i];
      t->time = clk;
      // trigger(t, 1);
  }
}


void TransitionResetCount(Transition* t_list[], int size){
  for(int i=0; i<size; i++){
      Transition* t = t_list[i];
      t->count = 0;
  }
}
    
void TransitionCountLog(Transition* t_list[], int size){
  for(int i=0; i<size; i++){
      Transition* t = t_list[i];
      std::cerr << "Transition:"<< t->id << " commit count=" << t->count << "\n";
  }
}

void PlaceTokensLog(Transition* t_list[], int size){
  std::set<std::string> place_set;
  for(int i=0; i<size; i++){
      Transition* t = t_list[i];
      std::cerr << "Transition:"<< t->id << " commit count=" << t->count << "\n";

      for (auto p : t->p_output){
          if(place_set.find(p->id) == place_set.end()){
            place_set.insert(p->id);
            std::cerr << "Place:"<< p->id << " token count=" << p->tokensLen() << "\n";
          }
      }

      for (auto p : t->p_input){
          if(place_set.find(p->id) == place_set.end()){
            place_set.insert(p->id);
            std::cerr << "Place:"<< p->id << " token count=" << p->tokensLen() << "\n";
          }
      }
  }
}