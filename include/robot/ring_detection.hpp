#include "pros/distance.hpp"
#include "pros/optical.hpp"

class Ring_Detector {
    public:
        Ring_Detector(int detector_port, int proximity_port);
        void initialize();
        bool has_red_ring();
        bool has_blue_ring();
        bool has_ring();
        void enable_led();
        void disable_led();
    private:
        pros::Optical optical;
        pros::Distance proximity;

};