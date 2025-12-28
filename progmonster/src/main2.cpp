#include "lemlib/api.hpp" // IWYU pragma: keep
#include "main.h"

#define OPTICAL_PORT 20

// MotorGroup: negative numbers are okay here to indicate reversed motors inside the group
pros::MotorGroup left_motors({-21, 20, -16}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({4, -11, 7}, pros::MotorGearset::blue);
pros::Controller controller(pros::E_CONTROLLER_MASTER);
pros::Motor intake_motor(1, pros::MotorGearset::green);
pros::Motor evil_motor(2, pros::MotorGearset::blue);
pros::Motor top_motor(3, pros::MotorGearset::blue);

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
int colorAssignment = 2; // 1 = RED to bottom/BLUE to top, 2 = BLUE to bottom/RED to top

// Flags used to coordinate tasks and safe shutdown between modes
volatile bool opRunning = false;
volatile bool autonRunning = false;

// Task pointers so we can remove tasks if needed
pros::Task* motorControlTaskPtr = nullptr;
pros::Task* driveTaskPtr = nullptr;
pros::Task* basketTaskPtr = nullptr;
pros::Task* scraperTaskPtr = nullptr;
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

void motorControl(void* param){
    if(controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        //intake
        intake_motor.move(127);
        evil_motor.move(-127);
        top_motor.move(127);
}
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L1)){
        //outtake center lower
        evil_motor.move(127);
        intake_motor.move(-127);
        top_motor.move(-127);
}
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)){
        //outtake center upper
        evil_motor.move(-127);
        intake_motor.move(127);
        top_motor.move(-127);
}

    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L2)){
        //outtake long goal
        intake_motor.move(-127);
        evil_motor.move(127);
        top_motor.move(-127);
}
    else {
        intake_motor.move(0);
        evil_motor.move(0);
        top_motor.move(0);}

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
