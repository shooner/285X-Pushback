//10-19-2025




#include "lemlib/api.hpp" // IWYU pragma: keep
#include "main.h"
#define RED_SIG 1
#define BLUE_SIG 2
#define VISION_PORT 20




pros::Motor onetwo_motor(4, pros::MotorGearset::green);
pros::Motor threefour_motor(5, pros::MotorGearset::green);
pros::Motor five_motor(3, pros::MotorGearset::green);
pros::Motor six_motor(2, pros::MotorGearset::green);
pros::Controller controller(pros::E_CONTROLLER_MASTER);
pros::ADIPort scraper('A', pros::E_ADI_DIGITAL_OUT);
pros::Vision vision_sensor (VISION_PORT);
pros::MotorGroup left_motors({-16, 12,-13}, pros::MotorGearset::blue); // left motors on ports 1, 2, 3
pros::MotorGroup right_motors({6, -7, 8}, pros::MotorGearset::blue); // right motors on ports 4, 5, 6  
pros::Rotation vertical(14);
pros::Rotation horizontal(-15);
pros::Imu imu(19);
int basket = 1; // 1 is bottom, 2 is top
int aut_height = 0; // keep
int aut_basket = 0; // keep
int teamColor = 1; // CHANGE THIS TO 1 IF RED, 2 IF BLUE
lemlib::Drivetrain drivetrain(&left_motors, // left motor group
                              &right_motors, // right motor group
                              13, // 12.5 inch track width
                              lemlib::Omniwheel::OLD_325, // using old 3.25" omnis
                              400, // drivetrain rpm is 400
                              2 // horizontal drift is 2
);




lemlib::TrackingWheel vertical_wheel(&vertical, lemlib::Omniwheel::NEW_2, 1.5);


lemlib::OdomSensors sensors(&vertical_wheel, // vertical tracking wheel 1, set to null
                            nullptr, // vertical tracking wheel 2, set to nullptr as we are using IMEs
                            nullptr, // horizontal tracking wheel 1
                            nullptr, // horizontal tracking wheel 2, set to nullptr as we don't have a second one
                            &imu // inertial sensor
);








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
// angular PID controller
lemlib::ControllerSettings angular_controller(2, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              10, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);




lemlib::ExpoDriveCurve throttle_curve(20, // joystick deadband out of 127
                                     20, // minimum output where drivetrain will move out of 127
                                     1.038 // expo curve gain
);








// input curve for steer input during driver control
lemlib::ExpoDriveCurve steer_curve(20, // joystick deadband out of 127
                                  20, // minimum output where drivetrain will move out of 127
                                  1.048 // expo curve gain
);







lemlib::Chassis chassis(drivetrain,
                        lateral_controller,
                        angular_controller,
                        sensors,
                        &throttle_curve,
                        &steer_curve
);




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


void toggleBasket(void* param){
    while(true){
        if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)){
            if(basket==1){
                basket = 2;
                controller.print(0, 0, "Top Basket");
                pros::delay(100);
            }
            else if(basket==2){
                basket = 1;
                controller.print(0, 0, "Bottom Basket");
                pros::delay(100);
            }
        }
        pros::delay(50);
    }
}


 void toggleScraper(void* param) {
    bool scraper_engaged = false;
    while (true) {
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y)) {
            scraper_engaged = !scraper_engaged;  // Toggle scraper
            scraper.set_value(scraper_engaged);
            pros::delay(200);               // Prevent rapid toggling
        }
        

        pros::delay(20);
    }
}


void motorControl(void* param) {
    bool last_blue = false;
    bool last_red = false;
    bool last_yours = false;
    bool reversing = false;
    if (teamColor == 1){
        last_red=true;
    }
    if (teamColor == 2){
        last_blue=true;
    }
    while (true) {

        if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)){
            pros::delay(20);
            // Intake with color sorting
            bool blue_present = detect_signature(vision_sensor, BLUE_SIG);
            bool red_present = detect_signature(vision_sensor, RED_SIG);


            threefour_motor.move(127);  // Always intake
            if (red_present){
                last_red = true;
                last_blue = false;
            }
            

            if(blue_present){
                last_blue = true;
                last_red = false;
            }
           
            if (teamColor == 1){
                last_yours = last_red;
            }
            
            if (teamColor == 2){
                last_yours=last_blue;
            }
                
           
            if(!last_yours){
                if (reversing){
                    five_motor.move(127);
                    pros::delay(100);
                    five_motor.move_velocity(0);
                    reversing = false;
                }  
                onetwo_motor.move(-127);
                six_motor.move(-127);
            } else if (last_yours){
                five_motor.move(-127);
                onetwo_motor.move_velocity(0);
                six_motor.move_velocity(0);
                reversing=true;
            }
            pros::lcd::set_text(2, last_red ? "Red Detected" : "No Red");
            pros::lcd::set_text(3, last_blue ? "Blue Detected" : "No Blue");
        }
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L1)){
            // Outtake center lower
            if(basket==1){
                threefour_motor.move(-127);
                five_motor.move(127);
                onetwo_motor.move(0);
                six_motor.move(0);
            } else if(basket==2){
                six_motor.move(127);
                onetwo_motor.move(127);
                threefour_motor.move(-127);
                five_motor.move(0);
            }
        }
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)){
            // Outtake center upper
            if(basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                six_motor.move(0);
            }
            else if(basket==2){
                six_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                five_motor.move(0);
            }
        }
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L2)){
            // Outtake long goal
            if(basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(-127);
                six_motor.move(0);
            }
            else if(basket==2){
                six_motor.move(127);
                onetwo_motor.move(-127);
                threefour_motor.move(0);
                five_motor.move(0);
            }
        }
        else {
            // No button pressed - stop all motors
            pros::delay(20);
            six_motor.move(0);
            onetwo_motor.move(0);
            threefour_motor.move(0);
            five_motor.move(0);
        }
        pros::delay(20);
         
    }
}


void drive(void* param) {
    while (true) {
        // Use controller joysticks to drive
        double forward = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
        double turn = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
        chassis.arcade(forward, turn);
        pros::delay(20);
    }
}


void convState(int b, int h){
    aut_height = h;
    aut_basket = b;
}
void convAuton(void* param) {
    bool last_red=false;
    bool last_blue=false;
    bool last_yours = false;
    if (teamColor == 1){
        last_red=true;
    }
    if (teamColor == 2){
        last_blue=true;
    }
    while(true){
        if (aut_height == 0){
            // Intake with color sorting
            bool blue_present = detect_signature(vision_sensor, BLUE_SIG);
            bool red_present = detect_signature(vision_sensor, RED_SIG);
           
            threefour_motor.move(127);  // Always intake
           
            if (red_present){
                last_red = true;
                last_blue = false;
            }


            if(blue_present){
                last_blue = true;
                last_red = false;
            }


            if (teamColor == 1){
                last_yours = last_red;
            }
            if (teamColor == 2){
                last_yours=last_blue;
            }
           
            if(!last_yours){
                onetwo_motor.move(-127);
                five_motor.move(0);
                six_motor.move(-127);
            } if (last_yours){
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
            } else if(aut_basket==2){
                six_motor.move(127);
                onetwo_motor.move(127);
                threefour_motor.move(-127);
                five_motor.move(0);
            }
        }
        else if (aut_height==2){
            // Outtake center upper
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                six_motor.move(0);
            } else if(aut_basket==2){
                six_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                five_motor.move(0);
            }
        }
        else if (aut_height==3){
            // Outtake long goal
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(-127);
                six_motor.move(0);
            }
            else if(aut_basket==2){
                six_motor.move(127);
                onetwo_motor.move(-127);
                threefour_motor.move(0);
                five_motor.move(0);
            }
        }
        else if (aut_height==-1){
            six_motor.move(0);
            onetwo_motor.move(0);
            threefour_motor.move(0);
            five_motor.move(0);
        }  
        pros::delay(20);
    }
}


/*void drive(void* param) {
    while (true) {
        // Use controller joysticks to drive
        double forward = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
        double turn = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
        chassis.arcade(forward, turn);
        pros::delay(20);
    }
}
void toggleOnOff(){
    
    while (true){
        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_LEFT)){
            colorSorting=!colorSorting;
            pros::delay(50);
            six_motor.move(0);
          onetwo_motor.move(0);
          threefour_motor.move(0);
            five_motor.move(0);
            last_blue=false;
            last_red=false;
            last_yours=false;
            
            
        }
        if (!colorSorting){
            controller.print(0, 4, "CS: OFF");
        } else {
            controller.print(0, 4, "CS: ON ");
        }
        pros::delay(10);
        if (basket==1){
            controller.print(0, 0, "BB ");
        } else {
            controller.print(0, 0, "TB");
        }

    }
}

void convState(int b, int h){
    aut_height = h;
    aut_basket = b;
}
void convAuton(void* param) {
    last_blue = false;
    last_red = false;
    last_yours = false;
    if (teamColor == 1){
        last_red=true;
    }
    if (teamColor == 2){
        last_blue=true;
    }
    while(true){
        if (aut_height == 0){
            // Intake with color sorting
            bool blue_present = detect_signature(vision_sensor, BLUE_SIG);
            bool red_present = detect_signature(vision_sensor, RED_SIG);
           
            threefour_motor.move(127);  // Always intake
           
            if (red_present){
                last_red = true;
                last_blue = false;
            }


            if(blue_present){
                last_blue = true;
                last_red = false;
            }


            if (teamColor == 1){
                last_yours = last_red;
            }
            if (teamColor == 2){
                last_yours=last_blue;
            }
           
            if(!last_yours){
                onetwo_motor.move(-127);
                five_motor.move(0);
                six_motor.move(-127);
            } if (last_yours){
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
            } else if(aut_basket==2){
                six_motor.move(127);
                onetwo_motor.move(127);
                threefour_motor.move(-127);
                five_motor.move(0);
            }
        }
        else if (aut_height==2){
            // Outtake center upper
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                six_motor.move(0);
            } else if(aut_basket==2){
                six_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(127);
                five_motor.move(0);
            }
        }
        else if (aut_height==3){
            // Outtake long goal
            if(aut_basket==1){
                five_motor.move(127);
                threefour_motor.move(127);
                onetwo_motor.move(-127);
                six_motor.move(0);
            }
            else if(aut_basket==2){
                six_motor.move(127);
                onetwo_motor.move(-127);
                threefour_motor.move(0);
                five_motor.move(0);
            }
        }
        else if (aut_height==-1){
            six_motor.move(0);
            onetwo_motor.move(0);
            threefour_motor.move(0);
            five_motor.move(0);
        }  
        pros::delay(20);
    }
    
}

*/




void initialize() {
    pros::lcd::initialize();
    chassis.calibrate(); // Start calibration

    

}
void opcontrol(){

    pros::lcd::initialize();
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    pros::Task motorControlTask (motorControl, NULL, "Motor Control Task");
    pros::vision_signature_s_t BLUE_SIGNATURE =
    pros::Vision::signature_from_utility (BLUE_SIG, -3461, -2881, -3172, 5123, 6215, 5668, 3.0, 0);
    pros::vision_signature_s_t RED_SIGNATURE =
    pros::Vision::signature_from_utility(RED_SIG, 9843, 12289, 11066, -1681, -891, -1286, 3.0, 0);
    vision_sensor.set_signature (BLUE_SIG, &BLUE_SIGNATURE);
    vision_sensor.set_signature (RED_SIG, &RED_SIGNATURE);
    pros::Task basketTask (toggleBasket, NULL, "Basket Task");
    pros::Task scraperTask (toggleScraper, NULL, "Scraper Task");
    
    pros::Task driveTask (drive, NULL, "Drive Task");
    while(true){
        if (motorControlTask.get_state() == pros::E_TASK_STATE_RUNNING) {
            pros::lcd::set_text(4, "a");
        } else {
        pros::lcd::set_text(4, "b");
        }
        pros::delay(20);
    }

    }





void autonomous() {
    
onetwo_motor.move(-67);

pros::delay(1000);
onetwo_motor.move(0); 

    aut_height = 0;
    aut_basket = 1;
    int a = -1;
    int b = -1;
    pros::Task convTask (convAuton, NULL, "Conveyor Task");
/*

    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);
    chassis.setPose(a*62, b*17, 90);

    pros::delay(500);
    chassis.moveToPoint(a*44, b*17, 1000);
    pros::delay(1000);
    convState(0, 0);
    //intake 3 from drop loader


    chassis.turnToPoint(a*44, b*52, 1000, {.maxSpeed=70});
    pros::delay(200);
    chassis.moveToPoint(a*44, b*52, 1000, {.maxSpeed=70});
    pros::delay(500);
    scraper.set_value(true);  
    convState(0, 0);
    chassis.turnToPoint(a*75, b*53 , 1500, {.maxSpeed=90});
    pros::delay(200);
    chassis.moveToPoint(a*75, b*53, 2000, {.maxSpeed=60});
    pros::delay(4000);
    //chassis.turnToPoint(a*44, b*50, 1000, {.forwards=false}); //remove for scraper auton
    chassis.moveToPoint(a*60, b*53, 1000, {.forwards=false});
    chassis.waitUntilDone();
    convState(0, -1);
    scraper.set_value(false);
    chassis.turnToPoint(a*14.6, b*12.3, 1500);
    pros::delay(500);
    chassis.moveToPoint(a*14.6, b*12.3, 3000, {.maxSpeed=50});
    pros::delay(500);
    chassis.waitUntilDone();
    convState(1, 1);
    pros::delay(3000);
    */
    convState(0, 2);
    convTask.remove(); // Stop conveyor task




    while (true){
        pros::delay(20);
    }
        
}




 //end of q2 bonus route


//start of q3 bonus route