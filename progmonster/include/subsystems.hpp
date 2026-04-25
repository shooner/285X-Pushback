#pragma once
#include "api.h"
#include "lemlib/api.hpp"
#include "mcl/runtime.hpp"

// Constants & Macros
#define OPTICAL_PORT 7
#define DOUBLE_PARK_MACRO 15
#define POTENTIOMETER_PORT 'E'
#define POT_MIN_READING 0
#define POT_MAX_READING 4095

// Assets
ASSET(LongGoalManeuver1_txt);
ASSET(firstcurve_txt);
ASSET(secondcurve_txt);
ASSET(thirdcurve_txt);
ASSET(fourthcurve_txt);
ASSET(secondcurve70_txt);
ASSET(secondcurve48_txt);
ASSET(bottom_left_curve_txt);
ASSET(alignWithPark_txt);

// Objects
extern pros::MotorGroup left_motors;
extern pros::MotorGroup right_motors;
extern pros::Controller controller;
extern pros::Motor intake_motor;
extern pros::Motor evil_motor;
extern pros::Motor top_motor;
extern pros::adi::Port trapdoor;
extern pros::adi::Port bunny;
extern pros::adi::Port scraper;
extern pros::adi::Port park;
extern pros::adi::Port hood;
extern pros::adi::Port lift_intake;
extern pros::Optical optical_sensor;
extern pros::Optical dp_sensor;
extern pros::ADIAnalogIn colorSensor;
extern pros::Imu imu;
extern pros::Rotation vertical_tracker;
extern lemlib::Chassis chassis;

// State Variables
extern bool bunny_engaged;
extern bool dp_macro_active;
extern bool trapdoor_engaged;
extern bool park_engaged;
extern bool scraper_engaged;
extern bool hood_engaged;
extern bool last_blue;
extern bool last_red;
extern bool autoSort;
extern bool lift_intake_engaged;
extern volatile bool opRunning;
extern volatile bool autonRunning;
extern int teamColor;


// Task Functions
void motorControl(void* param);
void drive(void* param);
void toggleColorSort(void* param);
void toggleScraper(void* param);
void toggleBunnyEars(void* param);
void togglePark(void* param);
void toggleHood(void* param);
void toggleTrapdoor(void* param);
void toggleDoublePark(void* param);

// Auton Helpers
void autonIntake(void* param);
void autonCenterLower(void* param);
void autonCenterUpper(void* param);
void autonLongGoal(void* param);
void slowAutonLongGoal(void* param);
void autonIdle(void* param);
void autonBunny(void* param);
void autonColorSort(void* param);

void moveOut(void* param);
void left43_Split(void* param);
void left_7Rush(void* param);
void right_7Rush(void* param);
void right_halfSAWP(void* param);
void skills(void* param);

extern int autonomousSelection;
void autonSelector(void* param);
