#include <memory>
#include <functional>
#include "rclcpp/rclcpp.hpp"
#include "astro_move/msg/motor_control_msg.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

// Node to send data to arduino and motor_control node
class AstroMove : public rclcpp::Node
{
public: 

    // The Current Speed of the Motor
    int motor_state = 0; // 0 = Forward, 1 = Stop, 2 = Reverse, 3 = Stop Again

    // Look Up Table for Motor Speeds
    float motor_speeds[4] = {0.5f, 0.0f, -0.5f, 0.0f};

    // Publisher Object
    rclcpp::Publisher<astro_move::msg::MotorControlMSG>::SharedPtr publish;

    // Timer Object
    rclcpp::TimerBase::SharedPtr timer;

    // How Long a Packet Should be Sent For (in ms)
    int packet_length = 5000;

    // How Much Time Should Be Between a Packet
    int send_length = 100;

    // The Number of Packets to Send Befor Switching Packets
    int packet_count = 0;

    // The Current Number of Packets Sent
    int packets_sent = 0;

    // Constructor for the AstroMove Node   
    AstroMove() : Node("astro_move")
    {
        // Initialize Node as a Publisher
        publish = create_publisher<astro_move::msg::MotorControlMSG>("test", 10);
        timer = create_wall_timer(100ms, std::bind(&AstroMove::callbackAstroMove, this));
        packet_count = 50;
    }

    // Setting up message to be sent to motor_control and arduino
    void callbackAstroMove() {
        auto message = astro_move::msg::MotorControlMSG();
        message.motor_index = 0;
        message.motor_speed = motor_speeds[motor_state];
        packets_sent++;
        if (packets_sent >= packet_count) {
            packets_sent = 0;
            motor_state++;
            if (motor_state >= 4)
            motor_state = 0;
        }
        publish->publish(message);
    }
};

int main(int argc, char* argv[])
{
    // Setup the Node
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<AstroMove>());
    rclcpp::shutdown();
    return 0;
}