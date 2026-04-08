#include "main.h"

// ---------- Color / Sensor Helpers ----------
bool detect_red_optical() {
    double hue = optical_sensor.get_hue();
    return (hue >= 0 && hue <= 15);
}

bool detect_blue_optical() {
    double hue = optical_sensor.get_hue();
    return (hue >= 190 && hue <= 260);
}

bool detect_proximity(){
    if (optical_sensor.get_proximity() > 50){
        return true;
    }
    else{
        return false;
    }
}

int get_color_destination(bool last_red, bool last_blue) {
    if (teamColor == 2) { // TEAM BLUE
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

bool detect_double_park_macro(){
    double hue = dp_sensor.get_hue();
    return ((hue >= 0 && hue <= 15)||(hue>=190&&hue<=260)) && (dp_sensor.get_proximity() > 100);
}

// ---------- Subsystem Tasks ----------

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
    scraper.set_value(false);
}

void toggleBunnyEars(void* param) {
    while (opRunning){
        bunny_engaged = controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2);
        bunny.set_value(bunny_engaged);
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
    trapdoor.set_value(true);
}

void toggleDoublePark(void* param) {
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
        
        if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
            intake = !intake;
            outlow = false;
            outmid = false;
            outlong = false;
            park.set_value(false);
            park_engaged = false;
        }
        else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)){
            outlow = !outlow;
            intake = false;
            outmid = false;
            outlong = false;
            trapdoor_engaged = true;
            trapdoor.set_value(trapdoor_engaged);
            park_engaged = false;
            park.set_value(park_engaged);
            lift_intake_engaged = !lift_intake_engaged;
            lift_intake.set_value(lift_intake_engaged); //lift intake when outtaking center lower
        }
        else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)){
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
            outlong = !outlong;
            intake = false;
            outlow = false;
            outmid = false;
            trapdoor_engaged = true;
            trapdoor.set_value(trapdoor_engaged);
            hood.set_value(true);
            park_engaged = false;
            park.set_value(park_engaged);
        }

        if(intake==true){
            hood_engaged = false;
            hood.set_value(hood_engaged);
            lift_intake_engaged = false;
            lift_intake.set_value(lift_intake_engaged);
            intake_motor.move(127);
            evil_motor.move(-127);
            top_motor.move(127);
            if(teamColor != 0){
                bool blue_present = detect_blue_optical() && detect_proximity();
                bool red_present = detect_red_optical() && detect_proximity();
                bool new_last_red = last_red;
                bool new_last_blue = last_blue;
                if (red_present) { new_last_red = true; new_last_blue = false; }
                if (blue_present) { new_last_blue = true; new_last_red = false; }
                if (new_last_red != last_red || new_last_blue != last_blue) {
                    bool blue_confirm = detect_blue_optical();
                    bool red_confirm = detect_red_optical();
                    if (red_confirm) { last_red = true; last_blue = false; }
                    else if (blue_confirm) { last_blue = true; last_red = false; }
                }
                int destination = get_color_destination(last_red, last_blue);
                trapdoor_engaged = (destination == 0);
                trapdoor.set_value(trapdoor_engaged);
            }
        }

        if(outlow==true){
            evil_motor.move(127);
            intake_motor.move(-87);
            top_motor.move(-127);
        }

        if(outmid==true){
            evil_motor.move(-80);
            intake_motor.move(80);
            top_motor.move(-45);
            if(teamColor != 0){
                bool blue_present = detect_blue_optical() && detect_proximity();
                bool red_present = detect_red_optical() && detect_proximity();
                bool new_last_red = last_red;
                bool new_last_blue = last_blue;
                if (red_present) { new_last_red = true; new_last_blue = false; }
                if (blue_present) { new_last_blue = true; new_last_red = false; }
                if (new_last_red != last_red || new_last_blue != last_blue) {
                    bool blue_confirm = detect_blue_optical();
                    bool red_confirm = detect_red_optical();
                    if (red_confirm) { last_red = true; last_blue = false; }
                    else if (blue_confirm) { last_blue = true; last_red = false; }
                }
                int destination = get_color_destination(last_red, last_blue);
                trapdoor_engaged = (destination == 0);
                trapdoor.set_value(trapdoor_engaged);
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
                if (red_present) { new_last_red = true; new_last_blue = false; }
                if (blue_present) { new_last_blue = true; new_last_red = false; }
                if (new_last_red != last_red || new_last_blue != last_blue) {
                    bool blue_confirm = detect_blue_optical();
                    bool red_confirm = detect_red_optical();
                    if (red_confirm) { last_red = true; last_blue = false; }
                    else if (blue_confirm) { last_blue = true; last_red = false; }
                }
                int destination = get_color_destination(last_red, last_blue);
                trapdoor_engaged = (destination == 0);
                trapdoor.set_value(trapdoor_engaged);
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
        chassis.arcade(forward, turn);
        pros::delay(20);
    }
    left_motors.move(0);
    right_motors.move(0);
}

// ---------- Autonomous Helpers ----------

void autonIntake(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = false;
    hood.set_value(hood_engaged);
    trapdoor.set_value(true);
}

void autonColorSort(void* param){
    while(autoSort){
        bool blue_present = detect_blue_optical() && detect_proximity();
        bool red_present = detect_red_optical() && detect_proximity();
        bool new_last_red = last_red;
        bool new_last_blue = last_blue;
        if (red_present) { new_last_red = true; new_last_blue = false; }
            if (blue_present) { new_last_blue = true; new_last_red = false; }
                if (new_last_red != last_red || new_last_blue != last_blue) {
                    bool blue_confirm = detect_blue_optical();
                    bool red_confirm = detect_red_optical();
                    if (red_confirm) { last_red = true; last_blue = false; }
                    else if (blue_confirm) { last_blue = true; last_red = false; }
                }
                int destination = get_color_destination(last_red, last_blue);
                trapdoor_engaged = (destination == 0);
                trapdoor.set_value(trapdoor_engaged);

                pros::delay(20);
    }

}

void autonCenterLower(void* param){
    evil_motor.move(127);
    intake_motor.move(-127);
    top_motor.move(-127);
    trapdoor_engaged = true;
    trapdoor.set_value(true);
}

void autonCenterUpper(void* param){
    evil_motor.move(-127);
    intake_motor.move(127);
    top_motor.move(-127);
    trapdoor_engaged = true;
    trapdoor.set_value(true);
}

void autonLongGoal(void* param){
    intake_motor.move(127);
    evil_motor.move(-127);
    top_motor.move(127);
    hood_engaged = true;
    hood.set_value(hood_engaged);
    trapdoor_engaged = true;
    trapdoor.set_value(true);
}

void slowAutonLongGoal(void* param){
    intake_motor.move(100);
    evil_motor.move(-100);
    top_motor.move(100);
    hood_engaged = true;
    hood.set_value(hood_engaged);
    trapdoor_engaged = true;
    trapdoor.set_value(true);
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

void moveToPointSmooth(float x, float y, int timeout, float maxSpeed) {
    lemlib::Pose targetPose(x, y, 0);
    float initialDistance = chassis.getPose().distance(targetPose);
    
    // Define many phase thresholds for extremely gradual deceleration
    float phase2Threshold = initialDistance * 0.85;    // 85%
    float phase3Threshold = initialDistance * 0.70;    // 70%
    float phase4Threshold = initialDistance * 0.55;    // 55%
    float phase5Threshold = initialDistance * 0.40;    // 40%
    float phase6Threshold = initialDistance * 0.28;    // 28%
    float phase7Threshold = initialDistance * 0.16;    // 16%
    float phase8Threshold = initialDistance * 0.08;    // 8%
    float phase9Threshold = initialDistance * 0.03;    // 3%
    float phase10Threshold = initialDistance * 0.01;   // 1%
    
    long startTime = pros::millis();
    int lastPhase = -1;
    
    while (pros::millis() - startTime < timeout) {
        float currentDistance = chassis.getPose().distance(targetPose);
        
        // Exit if very close to target
        if (currentDistance < 0.15) {
            left_motors.move(0);
            right_motors.move(0);
            break;
        }
        
        // Determine which phase we're in - extremely gradual deceleration with lower final speeds
        int phase = 1;
        float phaseMaxSpeed = maxSpeed;
        
        if (currentDistance <= phase10Threshold) {
            phase = 10;
            phaseMaxSpeed = 3;
        } else if (currentDistance <= phase9Threshold) {
            phase = 9;
            phaseMaxSpeed = 5;
        } else if (currentDistance <= phase8Threshold) {
            phase = 8;
            phaseMaxSpeed = 8;
        } else if (currentDistance <= phase7Threshold) {
            phase = 7;
            phaseMaxSpeed = 12;
        } else if (currentDistance <= phase6Threshold) {
            phase = 6;
            phaseMaxSpeed = 20;
        } else if (currentDistance <= phase5Threshold) {
            phase = 5;
            phaseMaxSpeed = 35;
        } else if (currentDistance <= phase4Threshold) {
            phase = 4;
            phaseMaxSpeed = 55;
        } else if (currentDistance <= phase3Threshold) {
            phase = 3;
            phaseMaxSpeed = 80;
        } else if (currentDistance <= phase2Threshold) {
            phase = 2;
            phaseMaxSpeed = 110;
        }
        
        // Only change moveToPoint call when phase actually changes
        if (phase != lastPhase) {
            lastPhase = phase;
            long remainingTime = timeout - (pros::millis() - startTime);
            chassis.moveToPoint(x, y, remainingTime + 500, {.maxSpeed = phaseMaxSpeed});
        }
        
        pros::delay(100);
    }
    
    // Final stop
    left_motors.move(0);
    right_motors.move(0);
}