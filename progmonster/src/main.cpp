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

    // Calibrate Chassis and Potentiometer
    chassis.calibrate();
    pros::delay(20);
    colorSensor.calibrate();

    // Team Color Selection via Potentiometer
    int pot_raw = colorSensor.get_value();
    int pot_mid = (POT_MIN_READING + POT_MAX_READING) / 2;
    if (pot_raw > pot_mid) {
        teamColor = 1; // Red
        controller.print(0, 0, "TEAM RED");
    } else {
        teamColor = 2; // Blue
        controller.print(0, 0, "TEAM BLUE");
    }

    pros::lcd::print(3, "Pot raw: %d", pot_raw);
    pros::delay(20);
    scraper.set_value(false);
    pros::Task selector(autonSelector);

    //mcl::initialize_filter_from_chassis(3.0f);
}

void disabled() {}

void competition_initialize() {}

void autonomous() {
    autonRunning = true;
    opRunning = false;
    
    mclTaskPtr = new pros::Task(mcl::mclRuntime, NULL, TASK_PRIORITY_DEFAULT, TASK_STACK_DEPTH_DEFAULT, "MCL Runtime Task");

    switch(autonomousSelection){
        case 0: left_4Rush(nullptr); break;
        case 1: right_4Rush(nullptr); break;
        case 2: left_7Rush(nullptr); break;
        case 3: right_7Rush(nullptr); break;
        case 4: right_halfSAWP(nullptr); break;
        case 5: skills(nullptr); break;
        default: break;
    }
    autonRunning = false;

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
    mclTaskPtr = new pros::Task(mcl::mclRuntime, NULL, TASK_PRIORITY_DEFAULT, TASK_STACK_DEPTH_DEFAULT, "MCL Runtime Task");

    // Keep the main task alive
    while (true) {
        pros::delay(20);
    }
}