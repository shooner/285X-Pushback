#include "main.h"

// Task pointers for management
pros::Task* motorControlTaskPtr = nullptr;
pros::Task* driveTaskPtr = nullptr;
pros::Task* scraperTaskPtr = nullptr;
pros::Task* bunnyTaskPtr = nullptr;
pros::Task* parkTaskPtr = nullptr;
pros::Task* hoodTaskPtr = nullptr;
pros::Task* trapdoorTaskPtr = nullptr;
pros::Task* dpTaskPtr = nullptr;
pros::Task* toggleColorSortTaskPtr = nullptr;
pros::Task* mclTaskPtr = nullptr;
pros::Task* autonColorSortTaskPtr = nullptr;

void initialize() {
    pros::lcd::initialize();

    // Configure motor encoder units
    left_motors.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_motors.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    
    // Calibrate Chassis and Potentiometer
    chassis.calibrate();
    pros::delay(20);
    sensor.calibrate();

    // Team Color Selection via Potentiometer
    int pot_raw = sensor.get_value();
    int pot_mid = (POT_MIN_READING + POT_MAX_READING) / 2;
    if (pot_raw > pot_mid) {
        teamColor = 1; // Blue
        controller.print(0, 0, "TEAM BLUE");
    } else {
        teamColor = 2; // Red
        controller.print(0, 0, "TEAM RED");
    }

    pros::lcd::print(3, "Pot raw: %d", pot_raw);
    pros::delay(20);
    scraper.set_value(false);

    mcl::initialize_filter_from_chassis(3.0f);
}

void disabled() {}

void competition_initialize() {}

void autonomous() {
    autonRunning = true;
    mcl::initialize_filter_from_chassis(2.5f);
    autonColorSortTaskPtr = new pros::Task(autonColorSort, NULL, "Auton Color Sort Task");

    int a = 1; // positive if right side auton
    int b = 1; // positive if right side auton

    

    // --- ELIM AUTO ---
    /*
    chassis.setPose(0, 0, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(a * 0, b * -25, 1000);
    scraper.set_value(true); 
    chassis.turnToHeading(270, 500); 
    autonIntake(nullptr);
    chassis.moveToPoint(a * -50, b * -25, 1200, {.maxSpeed = 45}); 
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(a * 27, b * 0, 700, {.forwards = false, .maxSpeed = 80}); 
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); 
    chassis.moveToPoint(a * 50, b * 0, 1300, {.forwards = false}); 
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();
    chassis.waitUntilDone();

    autonIntake(nullptr);
    left_motors.move(95);
    right_motors.move(-127);
    pros::delay(900);
    left_motors.move(0);
    right_motors.move(0);
    chassis.waitUntilDone();
    chassis.moveToPoint(a * 2.5, b * 21.3, 1200, {.maxSpeed = 50}); 
    pros::delay(800);
    chassis.turnToPoint(a * 6.5, b * 26, 500); 
    chassis.moveToPoint(a * 6.5, b * 26, 1700, {.maxSpeed = 50}); 
    autonCenterLower(nullptr);
    pros::delay(1000);

    chassis.moveToPoint(-15, 0, 1000, {.forwards = false}); 
    chassis.turnToHeading(0, 700);
    chassis.moveToPoint(-15, -13.5, 1000, {.forwards = false}); 
    chassis.waitUntilDone();
    bunny_engaged = false;
    bunny.set_value(false); 
    chassis.moveToPoint(0, -13.5, 1000, {.forwards = false}); 
    */

}

void opcontrol() {
    pros::lcd::initialize();
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    pros::delay(200);
    
    opRunning = true;

    // Start all background tasks defined in tasks.cpp
    driveTaskPtr = new pros::Task(drive, NULL, "Drive Task");
    motorControlTaskPtr = new pros::Task(motorControl, NULL, "Motor Control Task");
    scraperTaskPtr = new pros::Task(toggleScraper, NULL, "Scraper Task");
    bunnyTaskPtr = new pros::Task(toggleBunnyEars, NULL, "Bunny Ears Task");
    parkTaskPtr = new pros::Task(togglePark, NULL, "Park Task");
    hoodTaskPtr = new pros::Task(toggleHood, NULL, "Hood Task");
    dpTaskPtr = new pros::Task(toggleDoublePark, NULL, "Double Park Task");
    trapdoorTaskPtr = new pros::Task(toggleTrapdoor, NULL, "Trapdoor Task");
    toggleColorSortTaskPtr = new pros::Task(toggleColorSort, NULL, "Color Sort Toggle Task");
    mclTaskPtr = new pros::Task(mcl::mclRuntime, NULL, "MCL Runtime Task");

    // Keep the main task alive
    while (true) {
        pros::delay(20);
    }
}

#include "subsystems.hpp"