#include "main.h"

// ============================================
// MOTOR DEFINITIONS
// ============================================
pros::MotorGroup left_motors({-21, 20, -16}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({8, -11, 7}, pros::MotorGearset::blue);
pros::Motor intake_motor(3, pros::MotorGearset::green);
pros::Motor evil_motor(9, pros::MotorGearset::blue);
pros::Motor top_motor(10, pros::MotorGearset::blue);

// ============================================
// CONTROLLER & INPUT DEVICES
// ============================================
pros::Controller controller(pros::E_CONTROLLER_MASTER);

// ============================================
// ADI PORT DEFINITIONS (Pneumatics & Servos)
// ============================================
pros::adi::Port trapdoor('D', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port bunny('C', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port scraper('F', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port park('B', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port hood('A', pros::E_ADI_DIGITAL_OUT);

// ============================================
// SENSOR DEFINITIONS
// ============================================
pros::Optical optical_sensor(OPTICAL_PORT);
pros::Optical dp_sensor(DOUBLE_PARK_MACRO);
pros::ADIAnalogIn sensor(POTENTIOMETER_PORT);
pros::Imu imu(14);

// ============================================
// LEMLIB DRIVETRAIN & CHASSIS CONFIGURATION
// ============================================
lemlib::Drivetrain drivetrain(&left_motors, &right_motors, 13, lemlib::Omniwheel::OLD_325, 400, 2);
lemlib::OdomSensors sensors(nullptr, nullptr, nullptr, nullptr, &imu);
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
bool park_engaged = false;
bool scraper_engaged = false;
bool hood_engaged = true;
bool last_blue = false;
bool last_red = false;

// Game state
volatile bool opRunning = false;
volatile bool autonRunning = false;
int teamColor = 0;
