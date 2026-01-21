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
pros::MotorGroup right_motors({4, -11, 7}, pros::MotorGearset::blue);
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
bool trapdoor_engaged = false;
bool park_engaged = false;
bool scraper_engaged = true;

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
    if (teamColor == 2) {
        if (last_red) return 0;
        if (last_blue) return 1;
        return 0;
    } 
    else if(teamColor==1){
        if (last_blue) return 0;
        if (last_red) return 1;
        return 0;
    }
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
            trapdoor_engaged = true;
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
    scraper.set_value(true);
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
    bool hood_engaged = false;
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y)) {
            hood_engaged = !hood_engaged;
            hood.set_value(hood_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    hood.set_value(false);
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
    trapdoor.set_value(true);
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

    bool last_blue = false;
    bool last_red = false;
    bool was_macro_active = false;

    while(opRunning){
        if (dp_macro_active) {
            was_macro_active = true;
            trapdoor_engaged = true;
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
        trapdoor_engaged = true;
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
        trapdoor_engaged = true;
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
        trapdoor_engaged = true;
        trapdoor.set_value(trapdoor_engaged);
        hood.set_value(false);
        park_engaged = false;
        park.set_value(park_engaged);
}

    if(intake==true){
        hood.set_value(true);
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
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = true;
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
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = true;
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
                trapdoor_engaged = false;
                trapdoor.set_value(trapdoor_engaged);
            }
            else{
                trapdoor_engaged = true;
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
    trapdoor.set_value(true);
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
    hood.set_value(true);
    trapdoor.set_value(true);
}

void autonCenterLower(void* param){
    evil_motor.move(127);
    intake_motor.move(-127);
    top_motor.move(-127);
    trapdoor.set_value(true);

}

void autonCenterUpper(void* param){
    evil_motor.move(-127);
    intake_motor.move(127);
    top_motor.move(-127);
    trapdoor.set_value(true);
}

void autonLongGoal(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood.set_value(false);
    trapdoor.set_value(true);
}

void autonIdle(void* param){
    intake_motor.move(0);
    evil_motor.move(0);
    top_motor.move(0);
}

void autonMotor(void* param){
    while(autonRunning){
        if(aut_height==0){ //intake
            intake_motor.move(127);
            evil_motor.move(-127);
            top_motor.move(127);
        }
        else if(aut_height==1){ //outtake center lower
            evil_motor.move(127);
            intake_motor.move(-127);
            top_motor.move(-127);
        }
        else if(aut_height ==2){ //outtake center upper
            evil_motor.move(-127);
            intake_motor.move(127);
            top_motor.move(-127);
        }
        else if(aut_height==3){ //outtake long goal
            intake_motor.move(-127);
            evil_motor.move(127);
            top_motor.move(-127);
        }
        else if(aut_height ==-1){ //idle
            intake_motor.move(0);
            evil_motor.move(0);
            top_motor.move(0);
        }
    }
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
    scraper.set_value(true);
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

    int a = -1;
    int b = -1;

    chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-27.5, 1000, {.maxSpeed = 80});
    scraper.set_value(false); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 2700, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(25, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper up
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

    //bracket match auton
    /*chassis.setPose(a*50, b*17, 180);
    chassis.moveToPoint(a*50, b*47, 1500);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(a*57, b*47, 1000);
    convState(0); //intake 
    pros::delay(2700);
    convState(-1);
    chassis.moveToPoint(a*33, b*47, 1000, {.forwards=false});
    scraper_engaged = true;
    scraper.set_value(scraper_engaged); //true is up and false is down the evil solenoid
    convState(3); //outtake long goal
    pros::delay(2000);
    convState(-1);
    chassis.turnToHeading(180, 700);
    chassis.moveToPoint(a*33, b*35, 1000, {.forwards=false});
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(a*17, b*35, 1000, {.forwards=false});
*/
    
    //ROUGH AWP
    /*
    chassis.setPose(a*55, 17, 180);
    convState(0);
    chassis.turnToPoint(a*55, b*47, 1500);
    chassis.moveToPoint(a*55, b*47, 1000);

    chassis.turnToPoint(a*60, b*47, 700);   
    scraper.set_value(true);
    chassis.moveToPoint(a*60, b*47, 1000);
    pros::delay(2000);


    chassis.moveToPoint(a*26, b*47, 500, {.forwards=false});
    convState(3);
    scraper.set_value(false);
    pros::delay(2000); //outtake all long goal
    convState(0);
    chassis.follow(LongGoalManeuver1_txt, 15, 1000);




    chassis.moveToPoint(a*22, b*22, 1000);
    chassis.turnToPoint(a*12, b*-12, 700);
    chassis.moveToPoint(a*12, b*-12, 500);
    convState(2);
    pros::delay(100);
    convState(-1);

    //chassis.moveToPoint(a*10, b*-11, 1500, {.forwards=false});

    chassis.turnToPoint(a*47, b*-47, 700);    
    chassis.moveToPoint(a*47, b*-47, 500);
    chassis.moveToPoint(a*65, b*-47, 1000);
    chassis.moveToPoint(a*26, b*-47, 1000, {.forwards=false});
    */
    //ROUGH AWP

    //80 skills
    /*chassis.setPose(-50, -17, 180);
    chassis.moveToPoint(-50, -47, 1500);
    chassis.turnToHeading(270, 700);
    scraper.set_value(true);
    chassis.moveToPoint(-58, -47, 1000);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    scraper.set_value(false);
    chassis.turnToHeading(0,700);
    chassis.follow(firstcurve_txt, 10, 4000);
    chassis.setPose(40, -47, 180);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(33, -47, 700, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(true);
    chassis.moveToPoint(58, -47, 800);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(33, -47, 800, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(false);
    chassis.follow(secondcurve_txt, 10, 4000);
    chassis.setPose(63, 19.7, 0);
    chassis.turnToPoint(50, 47, 700);
    chassis.moveToPoint(50, 47, 1000);
    scraper.set_value(true);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(58, 47, 800);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(50, 47, 800, {.forwards=false});
    scraper.set_value(false);
    chassis.turnToHeading(180, 700);
    chassis.follow(thirdcurve_txt, 10, 4000);
    chassis.setPose(-50, 47, 0);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(-33, 47, 800, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(true);
    chassis.moveToPoint(-58, 47, 1000);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(-33, 47, 700, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(false);
    chassis.follow(fourthcurve_txt, 10, 4000);
*/

    // 75 skills             
    
    /*
    chassis.setPose(-50, -17, 180);
    trapdoor.set_value(true);
    chassis.moveToPoint(-50, -47, 1500);
    chassis.turnToHeading(270, 700);
    scraper.set_value(false); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(-68, -47, 1000); //move into the scraper

    //chassis.setPose(-68, -47, 270);
    pros::delay(2000); //intake all blocks
    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    chassis.moveToPoint(-68, -47, 1000); //move into the scraper
    //chassis.setPose(-68, -47, 270);
    pros::delay(2000);


    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    scraper.set_value(true);
    chassis.turnToHeading(0, 700);
    chassis.moveToPoint(-50, -57, 1000, {.forwards = false, .maxSpeed = 60});
    chassis.setPose(50, -57, 0);

    chassis.turnToHeading(90, 700);

    chassis.moveToPoint(50, -57, 2000, {.maxSpeed = 50});
    chassis.turnToHeading(0, 700);
    chassis.moveToPoint(50, -47, 1000);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(33, -47, 700, {.forwards=false});
    autonLongGoal(nullptr);
    pros::delay(5000); //outtake all long goal
    autonIntake(nullptr);

    scraper.set_value(false);
    
    chassis.moveToPoint(68, -47, 800);
    pros::delay(1200);
    chassis.moveToPoint(50, -47, 500, {.forwards=false});
    chassis.moveToPoint(68, -47, 800);
    pros::delay(1200); //intake all blocks

    chassis.moveToPoint(33, -47, 800, {.forwards=false});
    scraper.set_value(true);
    autonLongGoal(nullptr);
    pros::delay(2000); //outtake all long goal
    autonIdle(nullptr);

    chassis.moveToPoint(47, -47, 800);
    chassis.follow(bottom_left_curve_txt, 10, 1000);

    chassis.turnToHeading(0, 700);
    */









    /*
    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    scraper.set_value(false);
    chassis.turnToHeading(0,700);
    chassis.follow(firstcurve_txt, 10, 4000);
    chassis.setPose(40, -47, 180);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(33, -47, 700, {.forwards=false});
    autonLongGoal(nullptr);
    pros::delay(2000); //outtake all long goal
    autonIdle(nullptr);
    scraper.set_value(true);
    chassis.moveToPoint(58, -47, 800);
    autonIntake(nullptr);
    pros::delay(2700); //intake all blocks
    autonIdle(nullptr);
    chassis.moveToPoint(33, -47, 800, {.forwards=false});
    autonLongGoal(nullptr);
    pros::delay(2000); //outtake all long goal
    autonIdle(nullptr);
    scraper.set_value(true);
    chassis.turnToHeading(0,700);
    chassis.follow(secondcurve70_txt, 10, 4000);
    chassis.setPose(63, 19.7, 90);
    chassis.turnToPoint(50, 47, 700);
    chassis.moveToPoint(50, 47, 1000);
    scraper.set_value(true);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(58, 47, 800);
    autonIntake(nullptr);
    pros::delay(2700); //intake all blocks
    autonIdle(nullptr);
    chassis.moveToPoint(50, 47, 800, {.forwards=false});
    scraper.set_value(true);
    chassis.turnToHeading(180, 700);
    chassis.follow(thirdcurve_txt, 10, 4000);
    chassis.setPose(-50, 47, 0);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(-33, 47, 800, {.forwards=false});
    autonLongGoal(nullptr);
    pros::delay(2000); //outtake all long goal
    autonIdle(nullptr);
    scraper.set_value(false);
    chassis.moveToPoint(-58, 47, 1000);
    autonIntake(nullptr);
    pros::delay(2700); //intake all blocks
    autonIdle(nullptr);
    chassis.moveToPoint(-33, 47, 700, {.forwards=false});
    autonLongGoal(nullptr);
    pros::delay(2000); //outtake all long goal
    autonIdle(nullptr);
    scraper.set_value(false);
    chassis.follow(fourthcurve_txt, 10, 4000);
    */
    /*
    // 48 skills
    chassis.setPose(-50, -17, 180);
    chassis.moveToPoint(-50, -47, 1500);
    chassis.turnToHeading(270, 700);
    scraper.set_value(true);
    chassis.moveToPoint(-58, -47, 1000);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    scraper.set_value(false);
    chassis.turnToHeading(0,700);
    chassis.follow(firstcurve_txt, 10, 4000);
    chassis.setPose(40, -47, 180);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(33, -47, 700, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(true);
    chassis.moveToPoint(58, -47, 800);
    convState(0);
    pros::delay(2700); //intake all blocks
    convState(-1);
    chassis.moveToPoint(33, -47, 800, {.forwards=false});
    convState(3);
    pros::delay(2000); //outtake all long goal
    convState(-1);
    scraper.set_value(false);
    chassis.turnToHeading(0,700);
    chassis.follow(secondcurve48_txt, 10, 4000);
    */

    autonRunning = false;
    while (true) {
        pros::delay(50);
    }

}
