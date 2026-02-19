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
//pros::Rotation vertical(-6);
// IMU for heading


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
bool bunny_engaged = false;
bool dp_macro_active = false;
bool trapdoor_engaged = true;
bool park_engaged = false;
bool scraper_engaged = false;
bool hood_engaged = true;
bool last_blue = false;
bool last_red = false;


// Flags used to coordinate tasks and safe shutdown between modes
volatile bool opRunning = false;
volatile bool autonRunning = false;

int teamColor = 0; // 0 = OFF, 1 = BLUE, 2 = RED

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
//pros::Task* awpIntakeTaskPtr = nullptr;

// ---------- LEMLib objects ----------
lemlib::Drivetrain drivetrain(&left_motors,
                              &right_motors,
                              13, // 12.5-ish track width
                              lemlib::Omniwheel::OLD_325,
                              400,
                              2);

// OdomSensors using IMU for heading
// Internal motor encoders are configured separately in initialize()
lemlib::OdomSensors sensors(nullptr,
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
lemlib::ControllerSettings angular_controller(7, // proportional gain (kP)
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
    if (teamColor == 2) { // TEAM RED
        if (last_blue) return 0; // keep
        if (last_red) return 1;  // eject
        return 0;
    } else if (teamColor == 1) { // TEAM BLUE: keep red, eject blue
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
            controller.print(0,0, "TEAM BLUE");
            }
            else if(teamColor == 2){
            controller.print(0,0, "TEAM RED");
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
            pros::delay(50);
            park.set_value(true);
            park_engaged = true;
            dp_macro_active = false;
            pros::lcd::print(5, "Double Park Engaged");
        }
        pros::delay(10);
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
        evil_motor.move(-127);
        intake_motor.move(127);
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


void autonIntake(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = false;
    hood.set_value(hood_engaged);
    trapdoor.set_value(false);
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


/*void awpIntake(void* param){
    while(autonRunning){
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();
        intake_motor.move(127);
        evil_motor.move(-127);
        top_motor.move(127);
        hood_engaged = false;
        hood.set_value(hood_engaged);
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
}
*/

void initialize() {
    pros::lcd::initialize();

    // Initialize motor encoders for position tracking
    pros::Motor left_motor_1(21);
    pros::Motor left_motor_2(20);
    pros::Motor left_motor_3(16);
    pros::Motor right_motor_1(8);
    pros::Motor right_motor_2(11);
    pros::Motor right_motor_3(7);
    
    left_motor_1.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_motor_2.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_motor_3.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_motor_1.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_motor_2.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_motor_3.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    chassis.calibrate();
    pros::delay(20);
    sensor.calibrate();
    int pot_raw = sensor.get_value();
    int pot_mid = (POT_MIN_READING + POT_MAX_READING) / 2;
    if (pot_raw > pot_mid) {
        teamColor = 1; // Blue
        controller.print(0,0, "TEAM BLUE");
    } else {
        teamColor = 2; // Red
        controller.print(0,0, "TEAM RED");
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

    
 //   awpIntakeTaskPtr = new pros::Task(awpIntake, NULL, "Auton Control Task");


    //park and clear

    /*chassis.setPose(0,0,0);
    chassis.moveToPoint(0,-7, 2000, {.forwards = false});
    autonIntake(nullptr);
    chassis.moveToPoint(-2,10,5000, {.minSpeed = 127});
    chassis.waitUntilDone();
    autonIdle(nullptr);
*/
/*
    //viggy auton
    chassis.setPose(0,0,180);
    bunny_engaged = true;//bunny ears up
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(0, -26, 1500);
    chassis.turnToPoint(27, -4, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(27, -4, 3000, {.maxSpeed = 60});
    chassis.waitUntilDone();
    chassis.turnToPoint(4, -25.7, 700);
    chassis.moveToPoint(4, -25.7, 2000);
    scraper.set_value(true); //scraper down
    chassis.turnToPoint(-50, -25.7, 700); //turn to scraper
    chassis.waitUntilDone();
    //awpIntake(nullptr);
    autonIntake(nullptr);
    right_motors.move(80);
    left_motors.move(80);
    pros::delay(700);
    right_motors.move(30);
    left_motors.move(30);
    pros::delay(700);
    right_motors.move(0);
    left_motors.move(0);
    chassis.resetLocalPosition();
    chassis.moveToPoint(23, 0, 1000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    scraper.set_value(false); //scraper up
    autonLongGoal(nullptr);
    chassis.moveToPoint(50, 0, 2000, {.forwards=false}); //push into long goal
    pros::delay(2000);
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.resetLocalPosition();
    chassis.moveToPoint(-10,0,1000); //back out of long goal
    chassis.turnToHeading(225, 700);
    chassis.moveToPoint(2.6, 9, 1000, {.forwards = false});
    chassis.turnToHeading(270, 700);
    bunny_engaged = false;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(18.7, 9, 2000, {.forwards = false});

*/

    //AWP
/*
    chassis.setPose(0,0,angle);
    chassis.moveToPoint(a*0,b*-29, 1000);
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
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
    autonIntake(nullptr);
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
    if(a==1){
    autonCenterUpper(nullptr);
}
    if(a==-1){
    autonCenterLower(nullptr);
    }
    pros::delay(3000);
    stopIntake();

    chassis.resetLocalPosition();
    //scraper is already down from three red blocks, no need to 
    scraper.set_value(true); //scraper down
    chassis.turnToPoint(a*-32, b*36, 700); //turn to move idk
    chassis.moveToPoint(a*-32, b*36, 3000); //move go in front of drop loader
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.moveToPoint(a*-46, b*36, 1000); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(a*-70, b*36, 1300); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();

    chassis.moveToPoint(a*29, b*0, 1500, {.forwards = false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(a*50, b*0, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
*/

    //32
    /*
    chassis.setPose(0,0,180);
    chassis.moveToPoint(0,-23.3, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    trapdoor_engaged = false;
    trapdoor.set_value(trapdoor_engaged);
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -23.3, 4000, {.maxSpeed = 80}); //intake while moving into the thingy
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
    chassis.turnToPoint(-22.5, 3, 700);
    chassis.moveToPoint(-22.5, 3, 1000); //angle yourself i guess
    chassis.turnToPoint(-28, 45, 700);
    autonIntake(nullptr);
    chassis.moveToPoint(-28, 45, 4000, {.minSpeed = 127}); //park
    chassis.waitUntilDone();
    autonIdle(nullptr);
*/

    //48
    
/*
    chassis.setPose(0,0,180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(0,-23, 1000, {.maxSpeed = 80});
    chassis.turnToHeading(270, 700); //turn to scraper
    scraper.set_value(true); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -23, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToHeading(0, 700);
    chassis.moveToPoint(12, 15, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    //motto ganbare lydia!!!
    chassis.turnToPoint(85, 15, 700); //turn to move across long goal
    chassis.moveToPoint(85, 15, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();//wait until done 
    chassis.turnToHeading(180, 1000);
    chassis.waitUntilDone();
    right_motors.move(70);
    left_motors.move(70);
    pros::delay(1000);
    right_motors.move(35);
    left_motors.move(35);
    pros::delay(700);
    right_motors.move(0);
    left_motors.move(0);
    chassis.resetLocalPosition();
    chassis.moveToPoint(0,12, 1500, {.forwards = false});
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(-20, 12, 1000, {.forwards = false});
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-50, 12, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000, {.maxSpeed = 60}); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(50, 0, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    right_motors.move(-80);
    left_motors.move(-80);
    pros::delay(700);
    right_motors.move(-30);
    left_motors.move(-30);
    pros::delay(700);
    right_motors.move(0);
    left_motors.move(0);

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
    chassis.turnToPoint(-82, 15, 700);
    chassis.moveToPoint(-82, 15, 6000, {.maxSpeed = 80}); //move across field
    chassis.turnToPoint(-93, 21.7, 700);
    chassis.moveToPoint(-93, 21.7, 1000);
    chassis.turnToPoint(-94, 54, 700);
    chassis.moveToPoint(-94, 54, 6000, {.minSpeed = 127}); //park
*/

    //59
    
    chassis.setPose(0,0,180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(0,-23, 1000, {.maxSpeed = 80});
    chassis.turnToHeading(270, 700); //turn to scraper
    scraper.set_value(true); //scraper down
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -23, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToHeading(0, 700);
    chassis.moveToPoint(12, 15, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    //motto ganbare lydia!!!
    chassis.turnToPoint(85, 15, 700); //turn to move across long goal
    chassis.moveToPoint(85, 15, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();//wait until done 
    chassis.turnToHeading(180, 1000);
    chassis.waitUntilDone();
    right_motors.move(70);
    left_motors.move(70);
    pros::delay(1000);
    right_motors.move(35);
    left_motors.move(35);
    pros::delay(700);
    right_motors.move(0);
    left_motors.move(0);
    chassis.resetLocalPosition();
    chassis.moveToPoint(0,12, 1500, {.forwards = false});
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(-20, 12, 1000, {.forwards = false});
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-50, 12, 3000, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000, {.maxSpeed = 60}); //move to drop loader
    autonIntake(nullptr);
    chassis.moveToPoint(50, 0, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    right_motors.move(-80);
    left_motors.move(-80);
    pros::delay(700);
    right_motors.move(-30);
    left_motors.move(-30);
    pros::delay(700);
    right_motors.move(0);
    left_motors.move(0);

    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-50, 0, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);
    
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000); //move out of long goal

    chassis.turnToPoint(12, 103, 700); //turn to move to other side of field
    chassis.moveToPoint(12, 103, 5000, {.maxSpeed = 60}); //move to other side of field
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(0,-11, 1500, {.forwards = false});
    chassis.turnToHeading(90,700);
    autonIntake(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(500, -11, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
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
    chassis.turnToHeading(180, 700);
    chassis.moveToPoint(10, -15, 1500);
    chassis.turnToPoint(-76, -15, 700);
    chassis.moveToPoint(-76,-15,3000, {.maxSpeed = 80});
    chassis.turnToHeading(220, 700);
    chassis.moveToPoint(-76, -40, 4000, {.minSpeed = 127});

    

    //75 eeee
    /*
    chassis.setPose(0,0,180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged);
    chassis.moveToPoint(0,-29, 1000, {.maxSpeed = 80});
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(270, 700); //turn to scraper
    autonIntake(nullptr);
    chassis.moveToPoint(-500, -29, 3000, {.maxSpeed = 60}); //intake while moving into the thingy
    chassis.waitUntilDone();
    chassis.resetLocalPosition();
    chassis.moveToPoint(12, 0, 1000, {.forwards=false}); //move out of scraper
    scraper.set_value(false); //scraper up

    chassis.turnToPoint(12, 17, 700); //turn to move to the side of long goal
    chassis.moveToPoint(12, 17, 1000); //at the point to move parallel to the long goal
    autonIdle(nullptr);

    //motto ganbare lydia!!!
    chassis.turnToPoint(110, 17, 700); //turn to move across long goal
    chassis.moveToPoint(110, 17, 3500, {.maxSpeed = 60}); //move across long goal
    chassis.waitUntilDone();//wait until done 
    chassis.turnToHeading(180, 1000);
    chassis.waitUntilDone();
    chassis.moveToPoint(110, 5, 1000);
    chassis.turnToHeading(90, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(88, 5, 2000, {.forwards=false}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(60, 5, 2700, {.forwards=false}); //push into long goal
    chassis.waitUntilDone();
    autonIdle(nullptr);

    chassis.resetLocalPosition();
    chassis.moveToPoint(27, 0, 1000, {.maxSpeed = 60}); //move to drop loader
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

    chassis.turnToHeading(180, 700); //turn to get ready to align with barrier
    chassis.moveToPoint(-12, -15, 1000); //move to align with barrier
    chassis.turnToPoint(-99, -15, 700); //turn to move across the long goal
    chassis.moveToPoint(-99, -15, 4000, {.maxSpeed = 60}); //move across long goal
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
*/

    autonRunning = false;
    while (true) {
        pros::delay(50);
    }

}
