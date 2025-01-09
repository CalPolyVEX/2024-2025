
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

char* do_gold_tree(char* prev_state) {
  if (prev_state == "prime_start") {
    // TODO
  }
}

char* do_green_tree(char* prev_state) {}