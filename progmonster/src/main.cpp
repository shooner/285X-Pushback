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

    //mcl::initialize_filter_from_chassis(3.0f);
}

void disabled() {}

void competition_initialize() {}

void autonomous() {
    autonRunning = true;
    opRunning = false;
    
    mclTaskPtr = new pros::Task(mcl::mclRuntime, NULL, "MCL Runtime Task");

    

    
/*
    
    chassis.setPose(-49.5, -17, 180);
    chassis.moveToPoint(-49.5, -47, 2000, {.maxSpeed = 80});
    chassis.waitUntilDone();
    chassis.turnToHeading(270, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(-58.5, -47, 1500, {.maxSpeed = 30});
    chassis.waitUntilDone();
    pros::delay(1000);
    chassis.moveToPoint(-29, -47, 2000, {.forwards = false, .maxSpeed = 50});
    pros::delay(2000);
    chassis.moveToPoint(-49.5, -47, 2000, {.maxSpeed = 80});
    chassis.waitUntilDone();
    chassis.turnToHeading(45, 700);
    chassis.moveToPoint(-24, -24, 2000, {.maxSpeed = 60});

*/
    

    // --- ELIM AUTO ---
    
    chassis.setPose(a*-49.5, b*-17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(a * -49.5, b * -48.5, 1000);
    
    /*
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); 
    autonIntake(nullptr);
    chassis.moveToPoint(a * -58.5, b * -48.5, 1200, {.maxSpeed = 45}); 
    chassis.waitUntilDone();
    chassis.moveToPoint(a * -31, b * -48.5, 700, {.forwards = false, .maxSpeed = 80}); 
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    pros::delay(2000);

    autonIntake(nullptr);
    left_motors.move(95);
    right_motors.move(-127);
    pros::delay(700);
    left_motors.move(0);
    right_motors.move(0);
    chassis.waitUntilDone();
    chassis.moveToPoint(a * -31, b * -34, 1200, {.maxSpeed = 50}); 
    pros::delay(800);
    chassis.turnToPoint(a * -14, b * -14.4, 500); 
    chassis.moveToPoint(a * -14, b * -14.4, 1700, {.maxSpeed = 50}); 
    if(a==1){
        autonCenterLower(nullptr);
    }
    else if(a==-1){
        autonCenterUpper(nullptr);
    }
    pros::delay(1000);

    chassis.moveToPoint(a * -43, b * -58, 1000, {.forwards = false}); 
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(a * -28, b * -58, 1000, {.forwards = false}); 
    chassis.waitUntilDone();
    bunny_engaged = false; //bunny down
    bunny.set_value(false); 
    chassis.moveToPoint(a * -13, b * -58, 1000, {.forwards = false}); 
    */


    /*
    //119
    chassis.setPose(-46,0,270);
    autonIntake(nullptr);
    chassis.moveToPoint(-62, 0, 1000, {.minSpeed = 127});
    chassis.moveToPoint(-58, 0, 700, {.forwards = false});
    chassis.moveToPoint(-62, 0, 1000);
    chassis.moveToPoint(-58, 0, 700, {.forwards = false});
    chassis.moveToPoint(-62, 0, 1000);
    chassis.moveToPoint(-41, 0, 1500, {.forwards = false});
    autonIdle(nullptr);
    chassis.moveToPoint(-27, 11.4, 1000, {.forwards = false});
    chassis.moveToPoint(-19.6, 14, 700, {.forwards = false});
    chassis.moveToPoint(-13.5, 14, 700, {.forwards = false});
    chassis.moveToPoint(-13, 13.5, 700, {.forwards = false});
    autonIntake(nullptr);
    chassis.moveToPoint(-15.9, 16.4, 700);
    pros::delay(1000);
    chassis.moveToPoint(-13, 13.5, 800, {.forwards = false});
    autonCenterUpper(nullptr);
    pros::delay(2000);
    autonIdle(nullptr);
    chassis.moveToPoint(-46, 47, 1000, {.maxSpeed = 100});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-58.4, 47, 1000, {.maxSpeed = 60}); //move to scraper
    pros::delay(2000);

    chassis.moveToPoint(-48.4, 47, 800, {.forwards = false}); //back out of scraper
    chassis.turnToPoint(-32.9, 59.5, 700); //turn to the side of long goal
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-32.9, 59.5, 1000); //move to the side of long goal
    chassis.turnToPoint(31.6, 59.5, 700); //turn across long ogal
    chassis.moveToPoint(31.6, 59.5, 2700, {.maxSpeed = 90}); //move across long goal
    chassis.turnToPoint(41.6, 47, 700); 
    chassis.moveToPoint(41.6, 47, 1000);
    chassis.turnToHeading(90,700);
    chassis.moveToPoint(32.6, 47, 800, {.forwards = false}); //move to align with long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    autonIdle(nullptr);

    scraper.set_value(true); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(57, 47, 1000); //move to scraper
    pros::delay(2000);
    chassis.moveToPoint(32.6, 47, 1000, {.forwards = false}); //move to align with lon gogla
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    scraper.set_value(false); //scrape rup
    chassis.moveToPoint(41.6, 47, 1000); //back out of lon gogla

    chassis.turnToPoint(41.6, 0, 700);
    chassis.moveToPoint(41.6, 0, 1500);
    chassis.turnToPoint(62, 0, 700); //turn to parking zone
    autonIntake(nullptr);
    chassis.moveToPoint(62, 0, 1000, {.minSpeed = 127});
    chassis.moveToPoint(58, 0, 700, {.forwards = false});
    chassis.moveToPoint(62, 0, 1000);
    chassis.moveToPoint(58, 0, 700, {.forwards = false});
    chassis.moveToPoint(62, 0, 1000);

    chassis.moveToPoint(41.6, 0, 1500, {.forwards = false});
    chassis.moveToPoint(27, 11.4, 1000, {.forwards = false});
    chassis.moveToPoint(19.6, 14, 700, {.forwards = false});
    chassis.moveToPoint(13.5, 14, 700, {.forwards = false});
    chassis.moveToPoint(13, 13.5, 700, {.forwards = false});
    autonIntake(nullptr);
    chassis.moveToPoint(15.9, 16.4, 800);
    pros::delay(1000);
    chassis.moveToPoint(13, 13.5, 800);
    autonCenterLower(nullptr);
    pros::delay(2000);
    autonIntake(nullptr);
    chassis.moveToPoint(28, 27.7, 1000, {.maxSpeed = 50});

    chassis.turnToPoint(41.6, -47, 700);
    chassis.moveToPoint(41.6, -47, 2000, {.maxSpeed = 90});
    chassis.turnToHeading(90, 7000); //turn to scraper
    scraper.set_value(true); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(57, -47, 1000, {.maxSpeed = 60});
    pros::delay(2000);
    chassis.moveToPoint(47, -47, 1000, {.forwards = false}); //back out of scraper
    chassis.turnToPoint(31.6, -59.5, 600); //turn to the side of long goal
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(31.6, -59.5, 1000); //move to the side of long goal
    chassis.turnToPoint(-32.9, -59.5, 700); //turn across long ogal
    chassis.moveToPoint(-32.9, -59.5, 2700, {.maxSpeed = 90}); //move across long goal
    
    chassis.turnToPoint(-42, -47, 700);
    chassis.moveToPoint(-42, -47, 1000);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(-32.6, -47, 1000, {.forwards = false}); //move to align with long gogla
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    scraper.set_value(true); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(-58.4, -47, 1000, {.maxSpeed = 60}); //move to scraper
    pros::delay(2000);
    chassis.moveToPoint(-32.6, -47, 1000, {.forwards = false}); //long goal
    chassis.waitUntilDone();
    scraper.set_value(false); //scraper up
    autonLongGoal(nullptr);
    pros::delay(2000);
    autonIdle(nullptr);
    chassis.moveToPoint(-42, -47, 1000); //back out of long goal
    chassis.turnToPoint(-58.7, -33.2, 700); 
    chassis.moveToPoint(-58.7, -33.2, 1000);
    chassis.turnToPoint(-60.9, -26.2, 700);
    chassis.moveToPoint(-60.9, -26.2, 1000);
    chassis.turnToPoint(-63, 3.6, 700);
    chassis.moveToPoint(-63, 3.6, 4000, {.minSpeed = 127});

    */
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
    mclTaskPtr = new pros::Task(mcl::mclRuntime, NULL, "MCL Runtime Task");

    // Keep the main task alive
    while (true) {
        pros::delay(20);
    }
}