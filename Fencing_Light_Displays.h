//===========================================================================//
//  Name    : Fencing_Light_Displays.h                                       //
//  Desc    : C++ Interface for two RGB rings indicating fencing hits        //
//  Dev     : Nate Cope,                                                     //
//  Date    : Dec 2022                                                       //
//  Version : 1.1                                                            //
//  Notes   :                                                                //
//===========================================================================//

#ifndef FENCING_LIGHT_DISPLAYS_H
#define FENCING_LIGHT_DISPLAYS_H

// local includes 
#include "Fencing_Light.h"

// A class to control a fencing clock
class Fencing_Light_Displays
{
  public:

    // Constructor 
    //    uint8_t left_fencer_light_control_pin   - the Arduino pin attached to the control pin of the left fencer's ring light
    //    uint8_t right_fencer_light_control_pin  - the Arduino pin attached to the control pin of the right fencer's ring light
    Fencing_Light_Displays(uint8_t left_fencer_light_control_pin, uint8_t right_fencer_light_control_pin);

    // Destructor 
    ~Fencing_Light_Displays();

    // Lets the object know what the current time is. For the sake of streamlining 
    // the main code, this class should never check the time or call any sort of delay function,
    // but rely on this method to tell it what the time is, and update that way. 
    // if "0" is passed in specifically, we're just updating the displays, and no time checks are done 
    void tick(unsigned long current_time_micros); 

    // Hopefully all self-explanatory 
    void display_left_on_target(); 
    void display_right_on_target(); 
    void display_left_off_target(); 
    void display_right_off_target(); 
    void display_left_short_circuit();
    void display_right_short_circuit();
    void reset_lights();
    void set_brightness(uint8_t brightness); 
    void show_off_on_startup();
    void show_off_on_labelle();
    
  private:
  
    // the individual light displays 
    Fencing_Light* left_fencer_light_; 
    Fencing_Light* right_fencer_light_; 
};

#endif 
