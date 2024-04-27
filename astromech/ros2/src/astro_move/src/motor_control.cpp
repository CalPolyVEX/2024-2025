#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "../../../../packet_computer/buildmessage.h"
#include "astro_move/msg/motor_control_msg.hpp"
#include "std_msgs/msg/string.hpp"

using std::placeholders::_1;

// Temp Struct for Redboard Turbo Data
struct RedboardData
{
    // Left Motor Data
    uint8_t left_motor_data = 0;

    // Right Motor Data
    uint8_t right_motor_data = 0;

    // Some Other Data to be Used Later
    uint8_t other_data[6] = {0};
};

// Node Dedicated to Listening for Commands to Send to Arduino Motors
class MotorControl : public rclcpp::Node
{
public:

    // Subscriber Object
    rclcpp::Subscription<astro_move::msg::MotorControlMSG>::SharedPtr subscription;

    // Temporary Publisher Object
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr temp_pub;

    // Input Data
    uint8_t* input_buffer;
    int* input_size;

    // Constructor for the Motor Control Node   
    MotorControl() : Node("motor_control")
    {
        // Initialize File Descriptor
        init_serial();

        // Get Pointers to Redboard Turbo Data
        get_input_buffer(&input_buffer, &input_size);

        // Initialize Node as a Subscriber
        subscription = create_subscription<astro_move::msg::MotorControlMSG>("test", 10, std::bind(&MotorControl::callbackMotorControl, this, _1));

        // Temporary Publisher to Publish Data Read from Redboard
        temp_pub = create_publisher<std_msgs::msg::String>("temp_pub", 10);
    }

    // Callback Function
    void callbackMotorControl(const astro_move::msg::MotorControlMSG& msg)
    {
        // Get the Signed Motor Speed
        int8_t motor_speed = (int8_t)(100.0f * msg.motor_speed);

        // Illegal Type Cast to Unsigned Byte
        uint8_t correct_motor_speed = *(uint8_t*)(&motor_speed);

        RCLCPP_INFO(this->get_logger(), "1");

        // Forward Command to Build Message
        send_set_motor(msg.motor_index, correct_motor_speed);

        RCLCPP_INFO(this->get_logger(), "2");

        // Read Data Sent From Arduino
        read_arduino_data();

        // Temp, Get Data From Redboard and Print it
        RCLCPP_INFO(this->get_logger(), "test%d", ((RedboardData*)(input_buffer))->left_motor_data);

        // Temporarily Publish Results
        auto message = std_msgs::msg::String();
        message.data = std::to_string(((RedboardData*)(input_buffer))->left_motor_data);
        temp_pub->publish(message);
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