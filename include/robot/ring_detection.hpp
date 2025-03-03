#include "pros/distance.hpp"
#include "pros/optical.hpp"

class Ring_Detector {
  public:
    Ring_Detector(int detector_port, int proximity_port);
    bool has_red_ring();
    bool has_blue_ring();
    bool has_ring();
  private:
    pros::Optical detector;
    pros::Distance proximity;
};