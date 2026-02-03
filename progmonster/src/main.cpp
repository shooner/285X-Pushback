#include "lemlib/api.hpp" // IWYU pragma: keep
#include "main.h"


#define OPTICAL_PORT 19
#define DOUBLE_PARK_MACRO 15
#define POTENTIOMETER_PORT 'E'
// Potentiometer ADI raw range (adjust if your sensor reports a different span)
#define POT_MIN_READING 0
#define POT_MAX_READING 4095
ASSET (LongGoalManeuver1_txt);

// MotorGroup: negative numbers are okay here to indicate reversed motors inside the group
pros::MotorGroup left_motors({-21, 20, -16}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({8, -11, 7}, pros::MotorGearset::blue);
pros::Controller controller(pros::E_CONTROLLER_MASTER);

pros::Motor intake_motor(3, pros::MotorGearset::green);
pros::Motor evil_motor(9, pros::MotorGearset::blue);
pros::Motor top_motor(10, pros::MotorGearset::blue);

pros::adi::Port trapdoor('D', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port bunny('C', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port scraper('F', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port park('B', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port hood('A', pros::E_ADI_DIGITAL_OUT);

pros::Optical optical_sensor(OPTICAL_PORT);
pros::Optical dp_sensor(DOUBLE_PARK_MACRO);

pros::ADIAnalogIn sensor (POTENTIOMETER_PORT);

// Rotations / IMU
pros::Rotation vertical(-6);
// Replace negative port by positive with reversed flag
pros::Rotation horizontal(12);
pros::Imu imu(14);
ASSET(firstcurve_txt);
ASSET(secondcurve_txt);
ASSET(thirdcurve_txt);
ASSET(fourthcurve_txt);
ASSET(secondcurve70_txt);
ASSET(secondcurve48_txt);
ASSET(bottom_left_curve_txt);
ASSET(alignWithPark_txt);

// ---------- State ----------
int aut_height = -1; // conveyor command for auton thread
bool bunny_engaged = false;
bool dp_macro_active = false;
bool trapdoor_engaged = true;
bool park_engaged = false;
bool scraper_engaged = false;
bool hood_engaged = true;
bool last_blue = false;
bool last_red = false;
bool isIntaking = false;

// Flags used to coordinate tasks and safe shutdown between modes
volatile bool opRunning = false;
volatile bool autonRunning = false;

int teamColor = 0; // 0 = OFF, 1 = RED, 2 = BLUE

// Task pointers so we can remove tasks if needed
pros::Task* motorControlTaskPtr = nullptr;
pros::Task* driveTaskPtr = nullptr;
pros::Task* basketTaskPtr = nullptr;
pros::Task* scraperTaskPtr = nullptr;
pros::Task* bunnyTaskPtr = nullptr;
pros::Task* parkTaskPtr = nullptr;
pros::Task* hoodTaskPtr = nullptr;
pros::Task* trapdoorTaskPtr = nullptr;
pros::Task* convTaskPtr = nullptr;
pros::Task* dpTaskPtr = nullptr;
pros::Task* toggleColorSortTaskPtr = nullptr;
pros::Task* colorSortTaskPtr = nullptr;

// ---------- LEMLib objects ----------
lemlib::Drivetrain drivetrain(&left_motors,
                              &right_motors,
                              13, // 12.5-ish track width
                              lemlib::Omniwheel::OLD_325,
                              400,
                              2);

lemlib::TrackingWheel vertical_wheel(&vertical, lemlib::Omniwheel::NEW_2, .85);

lemlib::OdomSensors sensors(&vertical_wheel,
                            nullptr,
                            nullptr,
                            nullptr,
                            &imu);
    

lemlib::ControllerSettings lateral_controller(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);
lemlib::ControllerSettings angular_controller(9, // proportional gain (kP)
                                              0,    // integral gain (kI)
                                              65, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small  error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);
lemlib::ExpoDriveCurve throttle_curve(3, 10, 1.019);
lemlib::ExpoDriveCurve steer_curve(3, 10, 1.019);

lemlib::Chassis chassis(drivetrain,
                        lateral_controller,
                        angular_controller,
                        sensors,
                        &throttle_curve,
                        &steer_curve);

//color stuff
static bool detect_red_optical() {
    double hue = optical_sensor.get_hue();
    return (hue >= 0 && hue <= 15) /*|| (hue >= 345 && hue <= 360)*/;
}

static bool detect_blue_optical() {
    double hue = optical_sensor.get_hue();
    return (hue >= 190 && hue <= 260);
}

static bool detect_proximity(){
    if (optical_sensor.get_proximity() > 50){
        return true;
    }
    else{
        return false;
    }
}

static int get_color_destination(bool last_red, bool last_blue) {
    if (teamColor == 2) { // TEAM BLUE: keep blue, eject red
        if (last_blue) return 0; // keep
        if (last_red) return 1;  // eject
        return 0;
    } else if (teamColor == 1) { // TEAM RED: keep red, eject blue
        if (last_red) return 0;  // keep
        if (last_blue) return 1; // eject
        return 0;
    }
    return 0;
}

static bool detect_double_park_macro(){
    double hue = dp_sensor.get_hue();
    return ((hue >= 0 && hue <= 15)||(hue>=190&&hue<=260)) && (dp_sensor.get_proximity() > 100);
}

void toggleColorSort(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_LEFT)) {
            teamColor = (teamColor + 1) % 3; // cycles through 0, 1, 2
            if(teamColor == 0){
            controller.print(0,0, "ColorSort: OFF");
            trapdoor_engaged = false;
            trapdoor.set_value(trapdoor_engaged);
            }
            else if(teamColor == 1){
            controller.print(0,0, "TEAM RED");
            }
            else if(teamColor == 2){
            controller.print(0,0, "TEAM BLUE");
            }
            pros::delay(200);
            
        }
        pros::delay(20);
    }
}

void toggleScraper(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)) {
            scraper_engaged = !scraper_engaged;
            scraper.set_value(scraper_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    // on exit, retract scraper for safety
    scraper.set_value(false);
}

void toggleBunnyEars(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
            bunny_engaged = !bunny_engaged;
            bunny.set_value(bunny_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    bunny.set_value(false);
}

void togglePark(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B)) {
            park_engaged = !park_engaged;
            park.set_value(park_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    park.set_value(false);
}

void toggleHood(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y)) {
            hood_engaged = !hood_engaged;
            hood.set_value(hood_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    hood.set_value(true);
}

void toggleTrapdoor(void* param) {
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
            trapdoor_engaged = !trapdoor_engaged;
            trapdoor.set_value(trapdoor_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    trapdoor.set_value(false);
}


void toggleDoublePark(void* param) {
    bool dp_engaged = false;
    while (opRunning){
        dp_sensor.set_led_pwm(100);
        pros::lcd::print(3, "Proximity value: %ld \n", dp_sensor.get_proximity());
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
            dp_macro_active = true;
            evil_motor.move(127);
            intake_motor.move(-80);
            top_motor.move(-127);
            while (!detect_double_park_macro()) {
                pros::delay(20);
            }
            intake_motor.move(0);
            evil_motor.move(0);
            top_motor.move(0);
            pros::delay(100);
            park.set_value(true);
            park_engaged = true;
            dp_macro_active = false;
            pros::lcd::print(5, "Double Park Engaged");
        }
        pros::delay(20);
    }
}

void motorControl(void* param){

    bool intake = false;
    bool outlow = false;
    bool outmid = false;
    bool outlong = false;

    bool was_macro_active = false;

    while(opRunning){
        if (dp_macro_active) {
            was_macro_active = true;
            trapdoor_engaged = false;
            trapdoor.set_value(trapdoor_engaged);
            pros::delay(20);
            continue;
        }

        if (was_macro_active) {
            intake = false;
            outlow = false;
            outmid = false;
            outlong = false;
            was_macro_active = false;
        }
        optical_sensor.set_led_pwm(100);
        pros::lcd::print(2, "Proximity value: %ld \n", optical_sensor.get_proximity());
    if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_R2)) {
        //intake
        intake = !intake;
        outlow = false;
        outmid = false;
        outlong = false;
        park.set_value(false);
        park_engaged = false;

}
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)){
        //outtake center lower
        outlow = !outlow;
        intake = false;
        outmid = false;
        outlong = false;
        trapdoor_engaged = false;
        trapdoor.set_value(trapdoor_engaged);
        park_engaged = false;
        park.set_value(park_engaged);

}
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)){
        //outtake center upper
        outmid = !outmid;
        intake = false;
        outlow = false;
        outlong = false;
        trapdoor_engaged = false;
        trapdoor.set_value(trapdoor_engaged);
        park_engaged = false;
        park.set_value(park_engaged);
}

    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_R1)){
        //outtake long goal
        outlong = !outlong;
        bunny_engaged = false;
        bunny.set_value(bunny_engaged);
        intake = false;
        outlow = false;
        outmid = false;
        trapdoor_engaged = false;
        trapdoor.set_value(trapdoor_engaged);
        hood.set_value(true);
        park_engaged = false;
        park.set_value(park_engaged);
}

    if(intake==true){
        hood_engaged = false;
        hood.set_value(hood_engaged);
        intake_motor.move(127);
        evil_motor.move(-127);
        top_motor.move(127);
        if(teamColor != 0){
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();

        bool new_last_red = last_red;
            bool new_last_blue = last_blue;
            if (red_present) {
                new_last_red = true;
                new_last_blue = false;
            }
            if (blue_present) {
                new_last_blue = true;
                new_last_red = false;
            }

            // If a change was detected, wait 250ms and re-check before committing the switch
            if (new_last_red != last_red || new_last_blue != last_blue) {
                bool blue_confirm = detect_blue_optical();
                bool red_confirm = detect_red_optical();
                if (red_confirm) {
                    last_red = true;
                    last_blue = false;
                } else if (blue_confirm) {
                    last_blue = true;
                    last_red = false;
                }
                // If neither confirms, keep previous state
            }

            int destination = get_color_destination(last_red, last_blue);

            if(destination == 0){
                trapdoor_engaged = true;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
    }
}

    if(outlow==true){
        evil_motor.move(127);
        intake_motor.move(-127);
        top_motor.move(-127);
    }

    if(outmid==true){
        evil_motor.move(-60);
        intake_motor.move(60);
        top_motor.move(-60);
        if(teamColor != 0){
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();

        bool new_last_red = last_red;
            bool new_last_blue = last_blue;
            if (red_present) {
                new_last_red = true;
                new_last_blue = false;
            }
            if (blue_present) {
                new_last_blue = true;
                new_last_red = false;
            }

            // If a change was detected, wait 250ms and re-check before committing the switch
            if (new_last_red != last_red || new_last_blue != last_blue) {
                bool blue_confirm = detect_blue_optical();
                bool red_confirm = detect_red_optical();
                if (red_confirm) {
                    last_red = true;
                    last_blue = false;
                } else if (blue_confirm) {
                    last_blue = true;
                    last_red = false;
                }
                // If neither confirms, keep previous state
            }

            int destination = get_color_destination(last_red, last_blue);

            if(destination == 0){
                trapdoor_engaged = true;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
        }
    }

    if(outlong==true){
        intake_motor.move(127);
        evil_motor.move(-127);
        top_motor.move(127);
        if(teamColor != 0){
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();
        bool new_last_red = last_red;
            bool new_last_blue = last_blue;
            if (red_present) {
                new_last_red = true;
                new_last_blue = false;
            }
            if (blue_present) {
                new_last_blue = true;
                new_last_red = false;
            }

            // If a change was detected, wait 250ms and re-check before committing the switch
            if (new_last_red != last_red || new_last_blue != last_blue) {
                bool blue_confirm = detect_blue_optical();
                bool red_confirm = detect_red_optical();
                if (red_confirm) {
                    last_red = true;
                    last_blue = false;
                } else if (blue_confirm) {
                    last_blue = true;
                    last_red = false;
                }
                // If neither confirms, keep previous state
            }

            int destination = get_color_destination(last_red, last_blue);

            if(destination == 0){
                trapdoor_engaged = true;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
    }

    }
    if(outlong==false && intake==false && outlow==false && outmid==false){
        intake_motor.move(0);
        evil_motor.move(0);
        top_motor.move(0);
    }

    pros::delay(10);

    }
    trapdoor.set_value(false);
}

void drive(void* param) {
    while (opRunning) {
        double forward = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
        double turn = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
        // feed directly to chassis arcade (LEMLib handles scaling)
        chassis.arcade(forward, turn);
        pros::delay(20);
    }
    // stop chassis on exit
    left_motors.move(0);
    right_motors.move(0);
}

void convState(int state){
    aut_height = state;
    //-1 = idle
    //0 = intake
    //1 = center lower
    //2 = center upper
    //3 = long goal
}

void autonIntake(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = false;
    hood.set_value(hood_engaged);
    trapdoor.set_value(false);
}

// Background task: perform color-sorting while `isIntaking` is true.
void colorSortTask(void* param) {
    while (isIntaking) {
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();

        bool new_last_red = last_red;
        bool new_last_blue = last_blue;
        if (red_present) {
            new_last_red = true;
            new_last_blue = false;
        }
        if (blue_present) {
            new_last_blue = true;
            new_last_red = false;
        }

        if (new_last_red != last_red || new_last_blue != last_blue) {
            bool blue_confirm = detect_blue_optical();
            bool red_confirm = detect_red_optical();
            if (red_confirm) {
                last_red = true;
                last_blue = false;
            } else if (blue_confirm) {
                last_blue = true;
                last_red = false;
            }
        }

        int destination = get_color_destination(last_red, last_blue);
        if (destination == 0) {
            trapdoor_engaged = true;
            trapdoor.set_value(trapdoor_engaged);
        } else {
            trapdoor_engaged = false;
            trapdoor.set_value(trapdoor_engaged);
        }

        pros::delay(40);
    }
    colorSortTaskPtr = nullptr;
}

// Helper to stop intake and clean up the color-sort task safely.
void stopIntake() {
    isIntaking = false;
    intake_motor.move(0);
    evil_motor.move(0);
    top_motor.move(0);
    hood_engaged = true;
    hood.set_value(hood_engaged);
    if (colorSortTaskPtr != nullptr) {
        pros::delay(80);
        delete colorSortTaskPtr;
        colorSortTaskPtr = nullptr;
    }
}
//lydia auton intake with color sorting
void awpIntake(void* param){
    // Start intake motors and launch the color-sort task (non-blocking).
    isIntaking = true;
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = false;
    hood.set_value(hood_engaged);

    if (colorSortTaskPtr == nullptr) {
        colorSortTaskPtr = new pros::Task(colorSortTask, NULL, "AWP Color Sort Task");
    }
    }


void autonCenterLower(void* param){
    evil_motor.move(127);
    intake_motor.move(-127);
    top_motor.move(-127);
    trapdoor.set_value(false);

}

void autonCenterUpper(void* param){
    evil_motor.move(-127);
    intake_motor.move(127);
    top_motor.move(-127);
    trapdoor.set_value(false);
}

void autonLongGoal(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = true;
    hood.set_value(hood_engaged);
    trapdoor.set_value(false);
}

void autonIdle(void* param){
    intake_motor.move(0);
    evil_motor.move(0);
    top_motor.move(0);
}

void autonBunny(void* param){
    bunny_engaged = !bunny_engaged;
    bunny.set_value(bunny_engaged);
}

void initialize() {
    pros::lcd::initialize();
    chassis.calibrate();
    pros::delay(20);
    sensor.calibrate();
    int pot_raw = sensor.get_value();
    int pot_mid = (POT_MIN_READING + POT_MAX_READING) / 2;
    if (pot_raw > pot_mid) {
        teamColor = 1; // Red
        controller.print(0,0, "TEAM RED");
    } else {
        teamColor = 2; // Blue
        controller.print(0,0, "TEAM BLUE");
    }
    pros::lcd::print(3, "Pot raw: %d", pot_raw);
    pros::delay(20);
    scraper.set_value(false);
}


void opcontrol(){
    pros::lcd::initialize();
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    pros::delay(200);
    opRunning = true;
    driveTaskPtr = new pros::Task(drive, NULL, "Drive Task");
    motorControlTaskPtr = new pros::Task(motorControl, NULL, "Motor Control Task");
    scraperTaskPtr = new pros::Task(toggleScraper, NULL, "Scraper Task");
    bunnyTaskPtr = new pros::Task(toggleBunnyEars, NULL, "Bunny Ears Task");
    parkTaskPtr = new pros::Task(togglePark, NULL, "Park Task");
    hoodTaskPtr = new pros::Task(toggleHood, NULL, "Hood Task");
    dpTaskPtr = new pros::Task(toggleDoublePark, NULL, "Double Park Task");
    trapdoorTaskPtr = new pros::Task(toggleTrapdoor, NULL, "Trapdoor Task");
    toggleColorSortTaskPtr = new pros::Task(toggleColorSort, NULL, "Color Sort Toggle Task");
}

void autonomous(){
    autonRunning = true;

    int a = 1; // positive if right side auton
    int b = 1; // positive if right side auton

    //short auton wing
    /*chassis.setPose(0,0,180);
    chassis.moveToPoint(0, -29, 1500);
    chassis.turnToPoint(27.7, -5, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(27.7, -5, 3000, {.maxSpeed = 60});
    chassis.waitUntilDone();
    chassis.turnToPoint(4, -32, 700);
    autonIdle(nullptr);
    chassis.moveToPoint(4, -32, 2000);
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700);
    chassis.waitUntilDone();
    /*isIntaking = true;
    awpIntake(nullptr);*/
 /*   autonIntake(nullptr);
    chassis.moveToPoint(-50, -32, 1300); //intake while moving into the thingy
    chassis.resetLocalPosition();
    //stopIntake();
    chassis.moveToPoint(23, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    scraper.set_value(false); //scraper up
    autonLongGoal(nullptr);
    chassis.moveToPoint(50, 0, 2000, {.forwards=false}); //push into long goal
    pros::delay(2000);
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.moveToPoint(-10,0,1000); //back out of long goal
    chassis.turnToHeading(315, 700);
    chassis.moveToPoint(2.6, -10, 1000, {.forwards = false});
    chassis.turnToHeading(270, 700);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(20.7, -9.8, 2000, {.forwards = false});
*/

    //AWP
/*
    chassis.setPose(0,0,angle);
    chassis.moveToPoint(a*0,b*-29, 1000);
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    isIntaking = true;
    awpIntake(nullptr);
    chassis.moveToPoint(a*-50, b*-29, 1300); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(a*27, b*0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(a*50, b*0, 2000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    stopIntake();
    chassis.resetLocalPosition();
    chassis.waitUntilDone();
    chassis.moveToPoint(a*-11, b*0, 1000); //back away from long goal

    chassis.turnToPoint(a*1.3, b*16.8, 700); //turn to three red stuff
    chassis.moveToPoint(a*1.3, b*16.8, 2000, {.maxSpeed = 80}); //move to near three red stuff
    scraper.set_value(true); //scraper down
    //no one cares about isIntake here cause all the blocks are one color
    awpIntake(nullptr);
    chassis.moveToPoint(a*5.5, b*25, 2000, {.maxSpeed = 80}); //move to intake sum red stuff
    chassis.waitUntilDone();
    chassis.turnToPoint(a*5.5, b*60, 700); //turn to face the red blocks on the opposite side
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(a*5.5, b*60, 4000, {.maxSpeed = 80}); //move to intake red blocks on opposite side
    chassis.waitUntilDone();
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(a*5.5, b*72, 1000, {.maxSpeed = 80}); //move to intake sum red stuff
    chassis.waitUntilDone();
    chassis.turnToPoint(a*-9.5, b*87, 700); //turn to have back facing center goal
    chassis.moveToPoint(a*14.7, b*63.5, 1500, {.forwards = false}); //back up to center goal
    chassis.waitUntilDone();
    autonCenterUpper(nullptr);
    pros::delay(3000);
    stopIntake();

    chassis.resetLocalPosition();
    //scraper is already down from three red blocks, no need to 
    scraper.set_value(true); //scraper down
    chassis.turnToPoint(a*-32, b*36, 700); //turn to move idk
    chassis.moveToPoint(a*-32, b*36, 3000); //move go in front of drop loader
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.moveToPoint(a*-46, b*36, 1000); //move to drop loader
    isIntaking = true;
    awpIntake(nullptr);
    chassis.moveToPoint(a*-70, b*36, 1300); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    isIntaking = false;

    chassis.moveToPoint(a*29, b*0, 1500, {.forwards = false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(a*50, b*0, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    stopIntake();
*/

    //32
    /*
    chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-27.5, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 2700, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(25, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(50, 0, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();

    chassis.moveToPoint(-8, 0, 1000, {.forwards=false}); //back away from long goal
    chassis.resetLocalPosition();
    chassis.turnToHeading(90, 700); //hit a right hander
    chassis.moveToPoint(0, 16, 1000); //pre position to park on side
    chassis.resetLocalPosition();
    chassis.turnToHeading(-90, 700); //hit a left hander
    chassis.moveToPoint(-22, 0, 1000); //move to position to park from side

    chassis.resetLocalPosition();
    //follows curve that i havent uploaded yet to go from pre pos to park alignment
    //just kidding no pure pursuit
    chassis.turnToPoint(-24, 5, 700);
    chassis.moveToPoint(-24, 5, 1000); //angle yourself i guess
    chassis.turnToPoint(-28.3, 45, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(-28.3, 45, 4000); //park
    chassis.moveToPoint(-28.3, 38, 1000, {.forwards=false}); //back out a bit
    chassis.waitUntilDone();   
    chassis.moveToPoint(-28.3, 53, 5000); //park again
*/


    //48
    /*chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-27.5, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 2700, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToPoint(12, -13, 700); //turn to move to the side of long goal
    chassis.moveToPoint(12, -13, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    chassis.turnToPoint(110, -13, 700); //turn to move across long goal
    chassis.moveToPoint(110, -13, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();
    chassis.turnToHeading(0, 1000);
    chassis.waitUntilDone();
    chassis.moveToPoint(110, -2, 1000);
    chassis.turnToHeading(90, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(88, -2, 2000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    chassis.moveToPoint(60, -2, 2700, {.forwards=false}); //push into long goal
    scraper.set_value(true); //scraper down
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(50, 0, 2700, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(-27, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-50, 0, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000); //move out of long goal

    chassis.turnToPoint(12, 15, 700);
    chassis.moveToPoint(12, 15, 1000);
    chassis.turnToPoint(32, 15, 700); //get ready to move to the barrier
    chassis.moveToPoint(32, 15, 1000, {.maxSpeed = 60}); //move to barrier
    chassis.turnToPoint(34, 53, 700);
    chassis.moveToPoint(34, 53, 6000); //park
*/

    //59
    /*
    chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-27.5, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToPoint(12, -13, 700); //turn to move to the side of long goal
    chassis.moveToPoint(12, -13, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    chassis.turnToPoint(110, -13, 700); //turn to move across long goal
    chassis.moveToPoint(110, -13, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();
    chassis.turnToHeading(0, 1000);
    chassis.waitUntilDone();
    chassis.moveToPoint(110, -2, 1000);
    chassis.turnToHeading(90, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(88, -2, 2000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(60, -2, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(50, 0, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(-27, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-50, 0, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    

    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000); //move out of long goal

    chassis.turnToPoint(12, 98.5, 700); //turn to move to other side of field
    chassis.moveToPoint(12, 98.5, 5000, {.maxSpeed = 60}); //move to other side of field
    chassis.waitUntilDone();
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(90, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(500, 98.5, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(-24, 0, 1500, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    scraper.set_value(false); //scraper up
    autonLongGoal(nullptr);
    chassis.moveToPoint(-50, 0, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();

    chassis.moveToPoint(10, 0, 1000); //move out of long goal
    chassis.turnToPoint(27, -16, 700);
    chassis.moveToPoint(27, -16, 1000);
    chassis.turnToPoint(34, -47, 700);
    chassis.moveToPoint(34, -47, 6000, {.minSpeed = 127}); //park
    */

    //75 eeee
    
    chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-27.5, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToPoint(12, -14, 700); //turn to move to the side of long goal
    chassis.moveToPoint(12, -14, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    chassis.turnToPoint(110, -14, 700); //turn to move across long goal
    chassis.moveToPoint(110, -14, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();
    chassis.turnToHeading(0, 1000);
    chassis.waitUntilDone();
    chassis.moveToPoint(110, -3, 1000);
    chassis.turnToHeading(90, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(88, -3, 2000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(60, -3, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(50, 0, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(-27, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-50, 0, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000); //move out of long goal

    chassis.turnToPoint(12, 98.5, 700); //turn to move to other side of field
    chassis.moveToPoint(12, 98.5, 5000, {.maxSpeed = 60}); //move to other side of field
    chassis.waitUntilDone();
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(90, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(500, 98.5, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(-12, 0, 1500, {.forwards=false}); //move out of drop loader
    chassis.waitUntilDone();
    scraper.set_value(false); //scraper up

    chassis.turnToHeading(0, 700); //turn to get ready to align with barrier
    chassis.moveToPoint(-12, 15, 1000); //move to align with barrier
    chassis.turnToPoint(-99, 15, 700); //turn to move across the long goal
    chassis.moveToPoint(-99, 15, 4000, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();
    chassis.turnToPoint(-99, 2, 700);
    chassis.moveToPoint(-99, 2, 1500);
    chassis.turnToHeading(270, 700); //turn to make back to goal
    chassis.moveToPoint(-89, 2, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    chassis.moveToPoint(-40, 2, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-24, 2, 1000); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(-70, 2, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    
    chassis.moveToPoint(24, 0, 1500, {.forwards = false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(60, 0, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();

    chassis.moveToPoint(-10, 0, 1000, {.forwards=false}); //move out of long goal
    chassis.turnToPoint(-28.5, -16.8, 700);
    chassis.moveToPoint(-28.5, -16.8, 1500);

    chassis.turnToPoint(-31, -48, 700);
    chassis.moveToPoint(-31, -48, 6000, {.minSpeed = 127}); //park

    autonRunning = false;
    while (true) {
        pros::delay(50);
    }

}
