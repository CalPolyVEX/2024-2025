#define GOLD_BOT
#define GREEN_BOT

#include "auto_state_machine.h"

char* choose_next_state(char* prev_state) {
#ifdef GOLD_BOT
  return do_gold_tree(prev_state);
#endif

#ifdef GREEN_BOT
  return do_green_tree(prev_state);
#endif
}

int do_gold_tree(int prev_state) {
    if (prev_state == 0) { //prime_path
        // TODO. 
        //manipulate prev_state into a different int to get the next state
        
    }

    return do_gold_tree(prev_state); // recurse with new state
}

int do_green_tree(int prev_state) {
    //see other tree. TODO
    return do_green_tree(prev_state);
}