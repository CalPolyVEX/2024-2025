#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "../../../../packet_computer/buildmessage.h"
#include "astro_move/msg/motor_control_msg.hpp"
#include "std_msgs/msg/string.hpp"

using std::placeholders::_1;

// Node Dedicated to Listening for Commands to Send to Arduino Motors
class MotorControl : public rclcpp::Node
{
public:

    // Subscriber Object
    rclcpp::Subscription<astro_move::msg::MotorControlMSG>::SharedPtr subscription;

    // Constructor for the Motor Control Node   
    MotorControl() : Node("motor_control")
    {
        // Initialize File Descriptor
        init_serial();

        // Initialize Node as a Subscriber
        subscription = create_subscription<astro_move::msg::MotorControlMSG>("test", 10, std::bind(&MotorControl::callbackMotorControl, this, _1));
    }

    // Callback Function
    void callbackMotorControl(const astro_move::msg::MotorControlMSG& msg)
    {
        // Get the Signed Motor Speed
        int8_t motor_speed = (int8_t)(100.0f * msg.motor_speed);

        // Illegal Type Cast to Unsigned Byte
        uint8_t correct_motor_speed = *(uint8_t*)(&motor_speed);

        // Forward Command to Build Message
        send_set_motor(msg.motor_index, correct_motor_speed);
    }
};

int main(int argc, char* argv[])
{
    // Setup the Node
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorControl>());
    rclcpp::shutdown();
    return 0;
}