# motor_control

This ROS node communicates with the Herbie control board to:  

* read the encoders (sent at 30Hz from the Herbie board over USB)
* compute and publish odometry
* run the PID control loop for motor control
* queue up and send commands to the Herbie board (print to LCD, send servo commands, blink LEDs)

