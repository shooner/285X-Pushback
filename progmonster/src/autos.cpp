#include "main.h"
#include "autos.h"

void left_4Rush(void* param){

}

void right_4Rush(void* param){

}

void left_7Rush(void* param){

}

void right_7Rush(void* param){
    //7 block rush
    chassis.setPose(-49.5, -17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, -46, 1000, {.maxSpeed = 80});
    chassis.turnToPoint(-31.6, -30.4, 700); //turn to three red things
    chassis.moveToPoint(-31.6, -30.4, 1000, {.maxSpeed = 80}); //move to three red things
    autonIntake(nullptr);
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-24.8, -24.2, 1000, {.maxSpeed = 50}); //move to intake three red things 
    chassis.moveToPoint(-49.5, -46, 1000, {.forwards = false}); //back out to in front of drop loader
    chassis.turnToHeading(270, 700); //turn to drop loader
    chassis.moveToPoint(-58.5, -46, 1200, {.maxSpeed = 45}); 
    pros::delay(1300); //intake 3 red blocks
    chassis.moveToPoint(-31, -46, 700, {.forwards = false, .maxSpeed = 80}); //move to long goal
    chassis.waitUntilDone();
    autonLongGoal(nullptr);
    pros::delay(2000);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up
    chassis.moveToPoint(-40, -46, 1000); //back out of long goal
    chassis.turnToPoint(-33, -58, 700);
    chassis.moveToPoint(-33, -58, 1000); //move to side of long goal
    chassis.turnToHeading(2270, 700);
    chassis.moveToPoint(-23.8, -58, 800, {.forwards = false});
    bunny_engaged = false;
    bunny.set_value(false); //bnuuy down
    chassis.moveToPoint(-14, -58, 1000, {.forwards = false}); //wing wing wing

}

void right_halfSAWP(void* param){
    chassis.setPose(-49.5, -17, 180);
    bunny_engaged = true;
    bunny.set_value(bunny_engaged); //bunny up
    chassis.moveToPoint(-49.5, -44.5, 1000, {.maxSpeed = 80});
    chassis.waitUntilDone();
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.turnToHeading(260, 700); 
    autonIntake(nullptr);
    right_motors.move(70);
    left_motors.move(70);
    pros::delay(1000);
    right_motors.move(45);
    left_motors.move(45);
    pros::delay(1000);
    right_motors.move(0);
    left_motors.move(0);
    pros::delay(50);
    chassis.waitUntilDone();
    chassis.moveToPoint(-30, -44.5, 1000, {.forwards = false, .maxSpeed = 80}); 
    chassis.waitUntilDone();
    autonIdle(nullptr);
    pros::delay(100);
    autonLongGoal(nullptr);
    pros::delay(2700);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up

    chassis.moveToPoint(-55, -44.5, 1000);
    chassis.turnToPoint(-31, -34, 700);
    chassis.waitUntilDone();
    autonIntake(nullptr);
    chassis.moveToPoint(-31, -34, 1000, {.maxSpeed = 80});
    scraper_engaged = true;
    scraper.set_value(true); //scraper down
    chassis.moveToPoint(-24, -26, 1000, {.maxSpeed = 50}); //move to intake three red things
    pros::delay(300);
    scraper_engaged = false;
    scraper.set_value(false); //scraper up

    chassis.turnToPoint(-14, -14.4, 500); 
    chassis.moveToPoint(-14, -14.4, 1700, {.maxSpeed = 50}); 
    autonCenterLower(nullptr);
    chassis.moveToPoint(-43, -58, 1000, {.forwards = false}); 
    chassis.turnToHeading(270, 700);
    chassis.moveToPoint(-28, -58, 1000, {.forwards = false}); 
    chassis.waitUntilDone();
    bunny_engaged = false; //bunny down
    bunny.set_value(false); 
    chassis.moveToPoint(-13, -58, 1000, {.forwards = false}); 
}

void skills(void* param){

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

