#include "main.h"

// ============================================
// MOTOR DEFINITIONS
// ============================================
pros::MotorGroup left_motors({-15, 14, -13}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({16, -19, 20}, pros::MotorGearset::blue);
pros::Motor intake_motor(5, pros::MotorGearset::green);
pros::Motor evil_motor(9, pros::MotorGearset::blue);
pros::Motor top_motor(10, pros::MotorGearset::blue);

// ============================================
// CONTROLLER & INPUT DEVICES
// ============================================
pros::Controller controller(pros::E_CONTROLLER_MASTER);

// ============================================
// ADI PORT DEFINITIONS (Pneumatics & Servos)
// ============================================
pros::adi::Port trapdoor('G', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port bunny('F', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port scraper('A', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port park('E', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port hood('H', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port lift_intake('D', pros::E_ADI_DIGITAL_OUT);

// ============================================
// SENSOR DEFINITIONS
// ============================================
pros::Optical optical_sensor(OPTICAL_PORT);
pros::Optical dp_sensor(DOUBLE_PARK_MACRO);
pros::ADIAnalogIn sensor(POTENTIOMETER_PORT);
pros::Imu imu(8);
pros::Rotation vertical_tracker(17);
pros::Distance front_distance_sensor(6);
pros::Distance right_distance_sensor(11);

// ============================================
// LEMLIB DRIVETRAIN & CHASSIS CONFIGURATION
// ============================================
lemlib::Drivetrain drivetrain(&left_motors, &right_motors, 13, lemlib::Omniwheel::OLD_325, 400, 2);
lemlib::TrackingWheel vertical_tracking_wheel(&vertical_tracker, lemlib::Omniwheel::NEW_2, -0.75);
lemlib::OdomSensors sensors(nullptr, nullptr, &vertical_tracking_wheel, nullptr, &imu);
lemlib::ControllerSettings lateral_controller(10, 0, 3, 3, 1, 100, 3, 500, 20);
lemlib::ControllerSettings angular_controller(7, 0, 65, 3, 1, 100, 3, 500, 0);
lemlib::ExpoDriveCurve throttle_curve(3, 10, 1.019);
lemlib::ExpoDriveCurve steer_curve(3, 10, 1.019);
lemlib::Chassis chassis(drivetrain, lateral_controller, angular_controller, sensors, &throttle_curve, &steer_curve);

// ============================================
// STATE VARIABLES
// ============================================
// Subsystem states
bool bunny_engaged = false;
bool dp_macro_active = false;
bool trapdoor_engaged = true;
bool park_engaged = true;
bool scraper_engaged = false;
bool hood_engaged = true;
bool last_blue = false;
bool last_red = false;
bool autoSort = false;
bool lift_intake_engaged = false; //false = up

// Game state
volatile bool opRunning = false;
volatile bool autonRunning = false;
int teamColor = 0; // 1 = red, 2 = blue
