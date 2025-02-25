#ifndef CONFIG_H
#define CONFIG_H

/* Tim */
#define GOLD_BOT

/* Joseph */
//#define GREEN_BOT

/* Jeffy */
// #define PROTOTYPE_BOT

#endif

#ifdef GOLD_BOT

#define RED_STARTING_POSE lemlib::Pose(-52, 24, 90) // 72 - (8 + 12)
#define BLUE_STARTING_POSE lemlib::Pose(-52, 24, 90) // 72 - (8 + 12)
#endif

#ifdef GREEN_BOT
#define RED_STARTING_POSE lemlib::Pose(-44.75, -24, 90) // 72 - (15.75 + 12) (offset for dash)
#define BLUE_STARTING_POSE lemlib::Pose(-44.75, -24, 90) // 72 - (15.75 + 12) (offset for dash)
#endif

#ifdef PROTOTYPE_BOT
#define RED_STARTING_POSE lemlib::Pose(0, 0, -90)
#define BLUE_STARTING_POSE lemlib::Pose(0, 0, 90)
#endif