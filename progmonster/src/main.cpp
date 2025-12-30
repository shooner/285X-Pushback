#include "lemlib/api.hpp" // IWYU pragma: keep
#include "main.h"

#define OPTICAL_PORT 'E'

// MotorGroup: negative numbers are okay here to indicate reversed motors inside the group
pros::MotorGroup left_motors({-21, 20, -16}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({4, -11, 7}, pros::MotorGearset::blue);
pros::Controller controller(pros::E_CONTROLLER_MASTER);

pros::Motor intake_motor(3, pros::MotorGearset::green);
pros::Motor evil_motor(9, pros::MotorGearset::blue);
pros::Motor top_motor(10, pros::MotorGearset::blue);

pros::adi::Port trapdoor('H', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port bunny('A', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port scraper('G', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port park('D', pros::E_ADI_DIGITAL_OUT);
pros::adi::Port hood('F', pros::E_ADI_DIGITAL_OUT);

pros::Optical optical_sensor(OPTICAL_PORT);

// Rotations / IMU
pros::Rotation vertical(-14);
// Replace negative port by positive with reversed flag
pros::Rotation horizontal(15);
pros::Imu imu(19);
ASSET(firstcurve_txt);
ASSET(secondcurve_txt);
ASSET(thirdcurve_txt);
ASSET(fourthcurve_txt);

// ---------- State ----------
int basket = 1; // 1 bottom, 2 top
int aut_height = 0; // conveyor command for auton thread
int aut_basket = 0;
int teamColor = 2; // 1 = RED, 2 = BLUE
int colorAssignment = 2; // 1 = RED, 2 = BLUE
bool bunny_engaged = false;

// Flags used to coordinate tasks and safe shutdown between modes
volatile bool opRunning = false;
volatile bool autonRunning = false;

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

// ---------- LEMLib objects ----------
lemlib::Drivetrain drivetrain(&left_motors,
                              &right_motors,
                              13, // 12.5-ish track width
                              lemlib::Omniwheel::OLD_325,
                              400,
                              2);

lemlib::TrackingWheel vertical_wheel(&vertical, lemlib::Omniwheel::NEW_2, 2.5);

lemlib::OdomSensors sensors(&vertical_wheel,
                            nullptr,
                            nullptr,
                            nullptr,
                            &imu);


lemlib::ControllerSettings lateral_controller(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3, // derivative gain (kD)
                                              0, // anti windup
                                              0, // small error range, in inches
                                              0, // small error range timeout, in milliseconds
                                              0, // large error range, in inches
                                              0, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);
lemlib::ControllerSettings angular_controller(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              120, // derivative gain (kD)
                                              0, // anti windup
                                              0, // small error range, in inches
                                              0, // small error range timeout, in milliseconds
                                              0, // large error range, in inches
                                              0, // large error range timeout, in milliseconds
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
    return (hue >= 0 && hue <= 15) || (hue >= 345 && hue <= 360);
}

static bool detect_blue_optical() {
    double hue = optical_sensor.get_hue();
    return (hue >= 190 && hue <= 260);
}

static int get_color_destination(bool last_red, bool last_blue) {
    if (colorAssignment == 2) {
        if (last_red) return 0;
        if (last_blue) return 1;
        return 0;
    } else {
        if (last_blue) return 0;
        if (last_red) return 1;
        return 0;
    }
}

void toggleScraper(void* param) {
    bool scraper_engaged = false;
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y)) {
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
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
            bunny_engaged = !bunny_engaged;
            bunny.set_value(bunny_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    // on exit, retract bunny ears for safety
    bunny.set_value(false);
}

void togglePark(void* param) {
    bool park_engaged = false;
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)) {
            park_engaged = !park_engaged;
            park.set_value(park_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    // on exit, retract bunny ears for safety
    park.set_value(false);
}

void toggleHood(void* param) {
    bool hood_engaged = false;
    while (opRunning){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B)) {
            hood_engaged = !hood_engaged;
            hood.set_value(hood_engaged);
            pros::delay(200);
        }
        pros::delay(20);
    }
    // on exit, retract bunny ears for safety
    hood.set_value(false);
}


void motorControl(void* param){
    bool intake = false;
    bool outlow = false;
    bool outmid = false;
    bool outlong = false;

    bool last_blue = false;
    bool last_red = false;

    while(opRunning){
    if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_R2)) {
        //intake
        intake = !intake;
        bunny_engaged = true;
        bunny.set_value(bunny_engaged);
        outlow = false;
        outmid = false;
        outlong = false;

}
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)){
        //outtake center lower
        outlow = !outlow;
        intake = false;
        outmid = false;
        outlong = false;

}
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_R1)){
        //outtake center upper
        outmid = !outmid;
        intake = false;
        outlow = false;
        outlong = false;
}

    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)){
        //outtake long goal
        outlong = !outlong;
        bunny_engaged = false;
        bunny.set_value(bunny_engaged);
        intake = false;
        outlow = false;
        outmid = false;
}

    if(intake==true){
        intake_motor.move(127);
        evil_motor.move(-127);
        top_motor.move(127);

        bool blue_present = detect_blue_optical();
        bool red_present = detect_red_optical();

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
                trapdoor.set_value(true);
            }
            else{
                trapdoor.set_value(false);
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
        top_motor.move(-127);
    }

    if(outlong==true){
        intake_motor.move(-127);
        evil_motor.move(127);
        top_motor.move(-127);
    }

    if(outlong==false && intake==false && outlow==false && outmid==false){
        intake_motor.move(0);
        evil_motor.move(0);
        top_motor.move(0);
        bunny_engaged = false;
        bunny.set_value(bunny_engaged);
    }

    pros::delay(20);

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
    trapdoorTaskPtr = new pros::Task(toggleHood, NULL, "Trapdoor Task");

}

void autonomous(){
    chassis.setPose(-50, -17, 180);
    chassis.moveToPoint(-50, -47, 1500);
    chassis.turnToHeading(270, 700);
    //scraper down here
    chassis.moveToPoint(-58, -47, 1000);
    //intake all blocks here
    chassis.moveToPoint(-50, -47, 500, {.forwards=false});
    //scraper up here
    chassis.turnToHeading(0,700);
    chassis.follow(firstcurve_txt, 10, 4000);
    chassis.setPose(40, -47, 180);
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(33, -47, 700, {.forwards=false});
    //outtake all blocks here
    //scraper down after
    chassis.moveToPoint(58, -47, 800);
    //intake all blocks here
    chassis.moveToPoint(33, -47, 800, {.forwards=false});
    //outtake all blocks here
    //scraper up after
    chassis.follow(secondcurve_txt, 10, 4000);
    chassis.setPose(63, 19.7, 0);
    chassis.turnToPoint(50, 47, 700);
    chassis.moveToPoint(50, 47, 1000);
    //scraper down here
    chassis.turnToHeading(90, 700);
    chassis.moveToPoint(58, 47, 800);
    //intake all blocks here
    chassis.moveToPoint(50, 47, 800, {.forwards=false});
    chassis.turnToHeading(180, 700);
    chassis.follow(thirdcurve_txt, 10, 4000);
    chassis.setPose(-50, 47, 0);
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(-33, 47, 800, {.forwards=false});
    //outtake all blocks here
    //scraper down here
    chassis.moveToPoint(-58, 47, 1000);
    //intake all blocks here
    chassis.moveToPoint(-33, 47, 700, {.forwards=false});
    //outtake all blocks here
    //scraper up after
    chassis.follow(fourthcurve_txt, 10, 4000);

}
