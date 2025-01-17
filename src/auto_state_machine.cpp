#include "auto_state_machine.hpp"
#include "config.h"

int choose_next_state(int prev_state) {
#ifdef GOLD_BOT
  return do_gold_tree(prev_state);
#endif

#ifdef PROTOTYPE_BOT
  return do_green_tree(prev_state);
#endif
}

int do_gold_tree(int prev_state) {
  if (prev_state == 0) { // prime_path
    // TODO.
    // manipulate prev_state into a different int to get the next state
  }

  return choose_next_state(prev_state); // recurse with new state
}

int do_green_tree(int prev_state) {
  // see other tree.
  // TODO.
  return choose_next_state(prev_state);
}