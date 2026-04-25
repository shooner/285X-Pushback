#include "main.h"
#include "autos.h"

void moveOut(void* param){
    right_motors.move(90);
    left_motors.move(90);
    pros::delay(400);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
}

void left43_Split(void* param){
    //4 in center upper, 3 in long
    chassis.setPose(-49.5, 17, 0);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, 46, 1000, {.maxSpeed = 80});
    chassis.turnToPoint(-28, 33, 700); //turn to three red things
    chassis.moveToPoint(-28, 33, 1000, {.maxSpeed = 80}); //move to three red things
    autonIntake(nullptr);
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-18, 27, 1000, {.maxSpeed = 50}); //move to intake three red things
    chassis.turnToHeading(300, 700); //turn to have back to center upper
    chassis.moveToPoint(-11, 21, 1000, {.forwards = false, .maxSpeed = 80}); //back up to center upper
    chassis.waitUntilDone();
    right_motors.move(-45);
    left_motors.move(-45);
    pros::delay(300);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    autonIdle(nullptr);
    chassis.waitUntilDone();
 /*   autonCenterUpper(nullptr);
    pros::delay(2500);
    autonIdle(nullptr);
    */
    chassis.moveToPoint(-49.5, 49, 1200);
    autonIntake(nullptr);
    chassis.turnToHeading(270, 700);
    chassis.waitUntilDone();
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
   chassis.moveToPoint(-25, 49, 1200, {.forwards = false, .maxSpeed = 80});
    chassis.waitUntilDone();
    /*
    autonLongGoal(nullptr);*/
    pros::delay(2000);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up

}

void left_7Rush(void* param){
    chassis.setPose(-49.5, 17, 0);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, 46, 1000, {.maxSpeed = 80});
    chassis.turnToPoint(-28, 33, 700); //turn to three red things
    chassis.moveToPoint(-28, 33, 1000, {.maxSpeed = 80}); //move to three red things
    autonIntake(nullptr);
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-18, 27, 1000, {.maxSpeed = 50}); //move to intake three red things 
    chassis.moveToPoint(-43, 44.5, 1000, {.forwards = false, .maxSpeed = 80}); //back out to in front of drop loader
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.waitUntilDone();
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    chassis.setPose(-58, 47, 270); //here
    /*right_motors.move(-70);
    left_motors.move(-70);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    */
   chassis.moveToPoint(-25, 47, 1200, {.forwards = false, .maxSpeed = 80});
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up

}

void right_7Rush(void* param){
    /*chassis.setPose(-49.5, -17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, -46, 1000, {.maxSpeed = 80});
    chassis.turnToPoint(-28, -33, 700); //turn to three red things
    chassis.moveToPoint(-28, -33, 1000, {.maxSpeed = 80}); //move to three red things
    autonIntake(nullptr);
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-18, -27, 1000, {.maxSpeed = 50}); //move to intake three red things 
    chassis.moveToPoint(-43, -44.5, 1000, {.forwards = false, .maxSpeed = 80}); //back out to in front of drop loader
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.waitUntilDone();
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    chassis.setPose(-58, 47, 270); //here
    /*right_motors.move(-70);
    left_motors.move(-70);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    */
   /*chassis.moveToPoint(-25, -47, 1200, {.forwards = false, .maxSpeed = 80});
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up
    */

    chassis.setPose(-49.5, -17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    right_motors.move(80);
    left_motors.move(80);
    pros::delay(625);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    autonIntake(nullptr);
    scraper_engaged = true;
    scraper.set_value(true); 
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.waitUntilDone();
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1200);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    right_motors.move(-45);
    left_motors.move(-45);
    pros::delay(1200);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone(); 
    autonLongGoal(nullptr);
    pros::delay(2000);

}

void right_halfSAWP(void* param){
    chassis.setPose(-49.5, -17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, -43.5, 1000, {.maxSpeed = 80});
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(260, 500); 
    autonIntake(nullptr);
    right_motors.move(70);
    left_motors.move(70);
    pros::delay(1000);
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1200);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    chassis.moveToPoint(-30, -44.5, 1000, {.forwards = false, .maxSpeed = 60}); 
    chassis.waitUntilDone();
    autonIdle(nullptr);
    pros::delay(100);
    autonLongGoal(nullptr);
    chassis.moveToPoint(0, -44.5, 1300, {.forwards = false});
    scraper_engaged = false;
    scraper.set_value(false); //scraper up

    chassis.setPose(-30, -44.5, 270);
    chassis.moveToPoint(-45, -44.5, 1000);
    autonIntake(nullptr);
    chassis.turnToPoint(-22, -10, 700);
    chassis.moveToPoint(-22, -10, 1500, {.maxSpeed = 80});
    chassis.turnToPoint(-15, -3, 700);
    chassis.waitUntilDone();
    chassis.moveToPoint(-15, -3, 1600, {.maxSpeed = 60});
    chassis.waitUntilDone();
    autonCenterLower(nullptr);

    pros::delay(1500);

    chassis.setPose(-15, -3, 45);
    chassis.moveToPoint(-34, -24, 1500, {.forwards = false, .maxSpeed = 80});
    chassis.turnToHeading(0,700);
    chassis.moveToPoint(-34, -24, 1000, {.forwards = false});
    chassis.turnToHeading(270, 1000);
    chassis.waitUntilDone();
    right_motors.move(-70);
    left_motors.move(-70);
    pros::delay(550);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
}

void skills(void* param){

    //20
    autonIntake(nullptr);
    right_motors.move(-50);
    left_motors.move(-50);
    pros::delay(400);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    right_motors.move(127);
    left_motors.move(127);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);


    //119
    /*
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

}

int autonomousSelection = -1; // auton selection var

void autonSelector(void* param){
    int autonomousType = 0;
    autonomousSelection = -1;
    bool autonTypeSelected = false, autonSelected = false;

    Button autonType[] = {
        Button(10, 10, 225, 105, "Left", pros::Color::pale_violet_red, pros::Color::black),
        Button(245, 10, 225, 105, "Right", pros::Color::cyan, pros::Color::black),
        Button(245, 125, 225, 105, "Skills", pros::Color::light_green, pros::Color::black)
    };

    pros::screen::set_pen(pros::Color::light_gray);
    pros::screen::fill_rect(0, 0, 480, 240);
    while(!autonTypeSelected){
        for(int i = 0; i < 3; i++){
            autonType[i].render();
            if(autonType[i].isClicked()){
                autonomousType = i;
                autonTypeSelected = true;
            }
        }
        pros::delay(20);
    }
    pros::screen::erase();

    Button* auton = nullptr;
    int numAutons = 0;
    static Button leftAutons[] = {
        Button(10, 10, 225, 105, "Left 4 Rush", pros::Color::peach_puff, pros::Color::black),
        Button(245, 10, 225, 105, "Left 7 Rush", pros::Color::peach_puff, pros::Color::black),
    };
    static Button rightAutons[] = {
        Button(10, 10, 225, 105, "Right 4 Rush", pros::Color::peach_puff, pros::Color::black),
        Button(245, 10, 225, 105, "Right 7 Rush", pros::Color::peach_puff, pros::Color::black),
        Button(245, 125, 225, 105, "Right Half SAWP", pros::Color::peach_puff, pros::Color::black)
    };

    switch(autonomousType){
        case 0: auton = leftAutons; numAutons = 2; break;
        case 1: auton = rightAutons; numAutons = 3; break;
        case 2: autonomousSelection = 5; autonSelected = true; break;
    }

    pros::screen::set_pen(pros::Color::dark_gray);
    while(!autonSelected && autonTypeSelected){
        for(int i = 0; i < numAutons; i++){
            if(auton[i].isClicked()){
                autonomousSelection = i * 2 + autonomousType;
                if(autonomousType == 1 && i == 2) autonomousSelection = 4;
                auton[i].buttonColor = pros::Color::purple;
                autonSelected = true;
            }
            auton[i].render();
        }

        pros::delay(20);
    }
}

