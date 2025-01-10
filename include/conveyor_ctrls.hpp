#ifndef CONVEYOR_CTRLS_H
#define CONVEYOR_CTRLS_H

void score_with_fish_mech();
void set_conveyor_target_in_inches(float inches, int speed = 300);
void conveyor_deposit_and_intake(int speed = 600);
void move_conveyor_backward();
bool load_for_fish_mech();
void deposit_with_fish_mech();
bool has_red_ring();
bool has_blue_ring();

#endif // CONVEYOR_CTRLS_H