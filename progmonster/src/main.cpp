// 10-19-2025 - REWRITTEN (fixed task/timing/auton stop issues)

#include "lemlib/api.hpp" // IWYU pragma: keep
#include "main.h"

// Vision signature ids
#define RED_SIG 1
#define BLUE_SIG 2
#define VISION_PORT 20

// ---------- Hardware ----------
pros::Motor onetwo_motor(4, pros::MotorGearset::green);
pros::Motor threefour_motor(5, pros::MotorGearset::green);
pros::Motor five_motor(3, pros::MotorGearset::green);
pros::Motor six_motor(2, pros::MotorGearset::green);
pros::Controller controller(pros::E_CONTROLLER_MASTER);
pros::ADIPort scraper('A', pros::E_ADI_DIGITAL_OUT);
pros::Vision vision_sensor (VISION_PORT);

// MotorGroup: negative numbers are okay here to indicate reversed motors inside the group
pros::MotorGroup left_motors({-16, 12, -13}, pros::MotorGearset::blue);
pros::MotorGroup right_motors({6, -7, 8}, pros::MotorGearset::blue);

// Rotations / IMU
pros::Rotation vertical(-14);
// Replace negative port by positive with reversed flag
pros::Rotation horizontal(15);

pros::Imu imu(19);

// ---------- State ----------
int basket = 1; // 1 bottom, 2 top
int aut_height = 0; // conveyor command for auton thread
int aut_basket = 0;
int teamColor = 1; // 1 = RED, 2 = BLUE

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

lemlib::ControllerSettings lateral_controller(10, 0, 3, 3, 1, 100, 3, 500, 20);
lemlib::ControllerSettings angular_controller(2, 0, 10, 3, 1, 100, 3, 500, 0);

lemlib::ExpoDriveCurve throttle_curve(20, 20, 1.038);
lemlib::ExpoDriveCurve steer_curve(20, 20, 1.048);

lemlib::Chassis chassis(drivetrain,
                        lateral_controller,
                        angular_controller,
                        sensors,
                        &throttle_curve,
                        &steer_curve);

// ---------- Utility ----------
static bool detect_signature (pros:: Vision& vision, std::uint8_t sig_id, int min_w = 6, int min_h = 6) {
    pros::vision_object_s_t objs[1];
    int32_t copied = vision.read_by_sig(0, sig_id, 1, objs);
    (void) copied;
    const auto& obj = objs[0];
    if (obj.signature == VISION_OBJECT_ERR_SIG) return false;
    if (obj.signature != sig_id) return false;
    // Filter out noise by requiring minimum size
    return (obj.width >= min_w && obj.height >= min_h);
}

// ---------- Tasks (safe: check opRunning / autonRunning) ----------
void toggleBasket(void* param){
    // Task toggles basket with X button, runs only in opcontrol
    while (opRunning) {
        if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)){
            if(basket == 1){
                basket = 2;
                controller.print(0, 0, "Top Basket ");
            } else {
                basket = 1;
                controller.print(0, 0, "Bottom Basket");
            }
            pros::delay(150); // debounce
        }
        pros::delay(20);
    }
    // ensure safe exit
    pros::delay(10);
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

void motorControl(void* param) {
    bool last_blue = false;
    bool last_red = false;
    bool last_yours = false;
    bool reversing = false;

    // Initialize last_* according to team color
    if (teamColor == 1) last_red = true;
    if (teamColor == 2) last_blue = true;

    while (opRunning) {
        // INTAKE / sorting while R2 held
        if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
            // small delay to let vision update
            pros::delay(20);
            bool blue_present = detect_signature(vision_sensor, BLUE_SIG);
            bool red_present = detect_signature(vision_sensor, RED_SIG);

            // always intake
            threefour_motor.move(127);

            if (red_present){
                last_red = true;
                last_blue = false;
            }
            if (blue_present){
                last_blue = true;
                last_red = false;
            }

            last_yours = (teamColor == 1) ? last_red : last_blue;

            if(!last_yours){
                // eject non-team color
                if (reversing){
                    five_motor.move(127);
                    pros::delay(100);
                    five_motor.move_velocity(0);
                    reversing = false;
                }
                onetwo_motor.move(-127);
                six_motor.move(-127);
            } else {
                five_motor.move(-127);
                onetwo_motor.move_velocity(0);
                six_motor.move_velocity(0);
                reversing=true;
            }

            pros::lcd::set_text(2, last_red ? "Red Detected " : "No Red");
            pros::lcd::set_text(3, last_blue ? "Blue Detected" : "No Blue");
        }
        // Outtake center lower (L1)
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L1)){
            if(basket==1){
                threefour_motor.move(-127);
                five_motor.move(127);
                onetwo_motor.move(0);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                onetwo_motor.move(127);
                threefour_motor.move(-127);
                five_motor.move(0);
            }
        }
        // Outtake center upper (R1)
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)){
            if(basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                five_motor.move(0);
            }
        }
        // Outtake long goal (L2)
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L2)){
            if(basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(-127);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                onetwo_motor.move(-127);
                threefour_motor.move(0);
                five_motor.move(0);
            }
        }
        else {
            // stop all conveyor motors when no buttons pressed
            six_motor.move(0);
            onetwo_motor.move(0);
            threefour_motor.move(0);
            five_motor.move(0);
        }

        pros::delay(20);
    }

    // ensure motors are stopped when task exits
    six_motor.move(0);
    onetwo_motor.move(0);
    threefour_motor.move(0);
    five_motor.move(0);
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

void convState(int b, int h){
    aut_height = h;
    aut_basket = b;
}

// Conveyor/auton task — only runs while autonRunning is true
void convAuton(void* param) {
    bool last_red=false;
    bool last_blue=false;
    bool last_yours = false;

    if (teamColor == 1) last_red = true;
    if (teamColor == 2) last_blue = true;

    while (autonRunning) {
        if (aut_height == 0){
            bool blue_present = detect_signature(vision_sensor, BLUE_SIG);
            bool red_present = detect_signature(vision_sensor, RED_SIG);

            threefour_motor.move(127);  // Always intake

            if (red_present) {
                last_red = true;
                last_blue = false;
            }
            if (blue_present) {
                last_blue = true;
                last_red = false;
            }

            last_yours = (teamColor == 1) ? last_red : last_blue;

            if(!last_yours){
                onetwo_motor.move(-127);
                five_motor.move(0);
                six_motor.move(-127);
            } else {
                five_motor.move(-127);
                onetwo_motor.move(0);
                six_motor.move(0);
            }
        }
        else if (aut_height == 1){
            // Outtake center lower
            if(aut_basket==1){
                threefour_motor.move(-127);
                five_motor.move(127);
                onetwo_motor.move(0);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                onetwo_motor.move(127);
                threefour_motor.move(-127);
                five_motor.move(0);
            }
        }
        else if (aut_height == 2){
            // Outtake center upper
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                five_motor.move(0);
            }
        }
        else if (aut_height == 3){
            // Outtake long goal
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(-127);
                six_motor.move(0);
            } else {
                six_motor.move(127);
                onetwo_motor.move(-127);
                threefour_motor.move(0);
                five_motor.move(0);
            }
        }
        else if (aut_height == -1){
            six_motor.move(0);
            onetwo_motor.move(0);
            threefour_motor.move(0);
            five_motor.move(0);
        }

        pros::delay(20);
    }

    // on exit, ensure motors are stopped
    six_motor.move(0);
    onetwo_motor.move(0);
    threefour_motor.move(0);
    five_motor.move(0);
}

// ---------- Lifecycle functions ----------
void initialize() {
    pros::lcd::initialize();

    // set vision signatures once at startup
    horizontal.set_reversed(true);
    pros::vision_signature_s_t BLUE_SIGNATURE =
        pros::Vision::signature_from_utility (BLUE_SIG, -3461, -2881, -3172, 5123, 6215, 5668, 3.0, 0);
    pros::vision_signature_s_t RED_SIGNATURE =
        pros::Vision::signature_from_utility(RED_SIG, 9843, 12289, 11066, -1681, -891, -1286, 3.0, 0);
    vision_sensor.set_signature (BLUE_SIG, &BLUE_SIGNATURE);
    vision_sensor.set_signature (RED_SIG, &RED_SIGNATURE);

    // Start LEMLib calibration (may be blocking). Make sure it has time to finish
    chassis.calibrate();

    // Give sensors and libraries time to settle (important — don't remove)
    pros::delay(1500);
}

void opcontrol(){
    // opcontrol runs forever while driver control is active
    pros::lcd::initialize();

    // set chassis to coast for driver control
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);

    // small delay to let PROS scheduler settle after mode change
    pros::delay(200);

    // turn on flag so tasks know they should run
    opRunning = true;

    // create tasks (store pointers so we can remove later if needed)
    motorControlTaskPtr = new pros::Task(motorControl, NULL, "Motor Control Task");
    basketTaskPtr = new pros::Task(toggleBasket, NULL, "Basket Task");
    scraperTaskPtr = new pros::Task(toggleScraper, NULL, "Scraper Task");
    driveTaskPtr = new pros::Task(drive, NULL, "Drive Task");

    // little delay so tasks get scheduled before entering opcontrol loop
    pros::delay(200);

    // main opcontrol loop
    while (opRunning) {
        // simple status on LCD showing whether tasks are running
        if (motorControlTaskPtr && motorControlTaskPtr->get_state() == pros::E_TASK_STATE_RUNNING) {
            pros::lcd::set_text(4, "MotorCtrl: RUN");
        } else {
            pros::lcd::set_text(4, "MotorCtrl: STOP");
        }

        // show basket state
        controller.print(0, 0, basket == 1 ? "Basket: Bottom" : "Basket: Top   ");

        pros::delay(100);
    }

    // If we ever leave opcontrol (shouldn't normally in comp), shut down tasks
    if (motorControlTaskPtr) {
        motorControlTaskPtr->remove();
        delete motorControlTaskPtr;
        motorControlTaskPtr = nullptr;
    }
    if (driveTaskPtr) {
        driveTaskPtr->remove();
        delete driveTaskPtr;
        driveTaskPtr = nullptr;
    }
    if (basketTaskPtr) {
        basketTaskPtr->remove();
        delete basketTaskPtr;
        basketTaskPtr = nullptr;
    }
    if (scraperTaskPtr) {
        scraperTaskPtr->remove();
        delete scraperTaskPtr;
        scraperTaskPtr = nullptr;
    }
}

void autonomous() {
    // ensure auton flag and stop any op tasks (safety)
    autonRunning = true;

    // small motion to settle (mirrors your old auton start)
    onetwo_motor.move(-67);
    pros::delay(1000);
    onetwo_motor.move(0);

    // ensure chassis braking for auton
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);

    // convAuton task handles conveyor while autonRunning is true
    convTaskPtr = new pros::Task(convAuton, NULL, "Conveyor Task");

    // Set initial auton state
    aut_height = 0;
    aut_basket = 1;
    int a = -1;
    int b = -1;

    // --- AUTON ROUTE (kept from original) ---
    // (I left your commented-out sequences unchanged; below is your skills route rewritten to rely on autonRunning)

    /*
    //skills
    chassis.setPose(a*50, b*17, 180);
    chassis.turnToPoint(a*50, b*47, 200);
    pros::delay(200);
    chassis.moveToPoint(a*50, b*47, 1000);
    pros::delay(700);
    chassis.turnToPoint(a*56.5, b*47, 500);
    pros::delay(200);
    scraper.set_value(true);
    chassis.moveToPoint(a*56.5, b*47, 700);
    pros::delay(200);
    convState(0, 0); //intake 3 red 3 blue
    pros::delay(2700);
    convState(0, -1); //stop motors
    chassis.moveToPoint(a*56.5, b*47, 500, {.forwards=false});
    pros::delay(300);
    chassis.turnToPoint(a*23.7, b*33, 700); //trying to push away the double blocks
    pros::delay(200);
    scraper.set_value(false);


    chassis.moveToPoint(a*23.7, b*33, 1000);
    pros::delay(200);
    chassis.turnToPoint(a*23.7, b*24.2, 500);
    pros::delay(200);
    chassis.moveToPoint(a*23.7, b*24.2, 500);  //push away double blocks
    pros::delay(200);
    chassis.turnToPoint(a*13.5, b*13.7, 1000);
    convState(1, 1); //outtake center lower from bottom basket which has red
    pros::delay(2000);
    convState(0, -1); //stop motors


    chassis.turnToPoint(a*-47.8, b*47, 400);
    pros::delay(200);
    chassis.moveToPoint(a*-47.8, b*47, 4000);
    pros::delay(3500);
    scraper.set_value(true);
    chassis.turnToPoint(a*-56.5, b*47, 400);
    pros::delay(200);
    chassis.moveToPoint(a*-56.5, b*47, 600);
    pros::delay(300);
    convState(0,0); //intake 3 red 3 blue from the loader
    pros::delay(2700);
    convState(0,-1); //stop motors
    chassis.moveToPoint(a*-47.8, b*47, 500, {.forwards=false});
    pros::delay(300);
    chassis.turnToPoint(a*-23.7, b*33, 200);
    pros::delay(200);
    scraper.set_value(false);
    chassis.moveToPoint(a*-23.7, b*33, 2000);
    pros::delay(1500);
    chassis.turnToPoint(a*-23.7, b*24.2, 200);
    pros::delay(200);
    chassis.moveToPoint(a*-23.7, b*24.2, 500);
    pros::delay(500);
    chassis.turnToPoint(a*-13.5, b*13.8, 500);
    pros::delay(200);
    chassis.moveToPoint(a*-13.5, b*13.8, 700);
    pros::delay(700);
    convState(2, 2); //outtake from top basket to center upper 6 blue
    pros::delay(2700);
    convState(0,-1); //stop motors


    chassis.turnToPoint(a*-38.7, b*-8.4, 500);
    pros::delay(200);
    chassis.moveToPoint(a*-38.7, b*-8.4, 2000);
    pros::delay(3000);
    chassis.turnToPoint(a*-31.8, b*-15.4, 300);
    pros::delay(200);
    chassis.moveToPoint(a*-31.8, b*-15.4, 1000, {.maxSpeed = 60});
    pros::delay(700);
    convState(0, 0); //intake one blue block
    pros::delay(700);
    convState(0, -1); //stop motors
    chassis.turnToPoint(a*-23.7, b*-23.5, 500);
    pros::delay(200);
    chassis.moveToPoint(a*-23.7, b*-23.5, 500);
    pros::delay(500);
    chassis.turnToPoint(a*-13.8, b*-13.6, 500);
    pros::delay(200);
    chassis.moveToPoint(a*-13.8, b*-13.6, 700);
    pros::delay(700);
    convState(1, 1); //outtake 3 red blocks from bottom to center lower
    pros::delay(1500);
    convState(0, -1);


    chassis.turnToPoint(a*0, b*-26.7, 500);
    pros::delay(200);
    chassis.moveToPoint(a*0, b*-26.7, 700);
    pros::delay(700);
    chassis.turnToPoint(a*17.2, b*-17.2, 500);
    pros::delay(200);
    chassis.moveToPoint(a*17.2, b*-17.2, 700);
    pros::delay(800);
    chassis.turnToPoint(a*13.8, b*-13.6, 500);
    pros::delay(200);
    chassis.moveToPoint(a*13.8, b*-13.6, 600);
    pros::delay(600);
    convState(2,2); //outtake one top blue to center upper, center filled
    pros::delay(1500);
    convState(0,-1); //stop motors
    chassis.turnToPoint(a*50, b*-47, 500);
    pros::delay(200);
    chassis.moveToPoint(a*50, b*-47, 2500);
    pros::delay(2000);
    chassis.turnToPoint(a*56.5, b*-47, 500);
    pros::delay(200);
    scraper.set_value(true);
    chassis.moveToPoint(a*56.5, b*-47, 700);
    pros::delay(800);
    convState(0,0); //intake 3 red 3 blue
    pros::delay(2700);
    convState(0,-1); //stop motors
    chassis.moveToPoint(a*50, b*-47, 500, {.forwards=false});
    pros::delay(300);
    chassis.turnToPoint(a*32.6, b*-47, 500);
    pros::delay(200);
    scraper.set_value(false);
    chassis.moveToPoint(a*32.6, b*-47, 1000);
    pros::delay(1000);
    convState(1,3); //outtake 3 red from bottom to long goal
    pros::delay(2000);
    convState(2,3); //outtake 3 blue from top to long goal
    pros::delay(2000);
    convState(0,-1); //stop motors

    //parking
    chassis.turnToPoint(a*59.6, b*-28.6, 500);
    pros::delay(700);
    chassis.moveToPoint(a*59.6, b*-28.6, 1000);
    pros::delay(1500);
    chassis.turnToPoint(a*60.7, b*-1, 400);
    pros::delay(500);
    chassis.moveToPoint(a*60.7, b*-1, 800);
    pros::delay(700);

    */
    // End of autonomous routine: stop conveyor task and mark auton finished
    autonRunning = false;
    if (convTaskPtr) {
        convTaskPtr->remove();
        delete convTaskPtr;
        convTaskPtr = nullptr;
    }
    
    // final safe stop
    six_motor.move(0);
    onetwo_motor.move(0);
    threefour_motor.move(0);
    five_motor.move(0);

    

    // Auton maxxing auton auton not skills autonomous
    chassis.setPose(a*50, b*17, 180);
    chassis.turnToPoint(a*50, b*47, 1500);
    chassis.moveToPoint(a*50, b*47, 1000);

    chassis.turnToPoint(a*56.5, b*47, 1500);
    scraper.set_value(true);
    chassis.moveToPoint(a*56.5, b*47, 1000);
    convState(0, 0); //intake 3 red 3 blue
    pros::delay(2700);
    convState(0, -1); //stop motors
    chassis.moveToPoint(a*56.5, b*47, 500, {.forwards=false});

    chassis.turnToPoint(a*31.3, b*11.8, 700); //trying to intake the corner stack
    convState(0,0); //intake the 3 red corner blocks
    chassis.moveToPoint(a*31.3, b*11.8, 1000);

    chassis.turnToPoint(a*23, b*22.7, 1000);
    chassis.moveToPoint(a*23, b*22.7, 700);

    chassis.turnToPoint(a*13.7, b*13.5, 1000);
    chassis.moveToPoint(a*13.7, b*13.5, 700);

    pros::delay(2000);
    convState(1,1); //outtake center lower from bottom basket which has red


    // hold here (typical auton ends and does not return)
    while (true) {
        pros::delay(50);
    }
}