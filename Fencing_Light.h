//===========================================================================//
//  Name    : Fencing_Light.h                                                //
//  Desc    : C++ Interface for a RBG Ring Fencing Light                     //
//  Dev     : Nate Cope,                                                     //
//  Date    : Jan 2023                                                       //
//  Version : 1.2                                                            //
//  Notes   : TODO a show-off on time running out, too??                     //
//            TODO cool animation methods and resulting tick method          // 
//===========================================================================//

#ifndef FENCING_LIGHT_H
#define FENCING_LIGHT_H

// global includes
#include <inttypes.h>
#include <Arduino.h>

// local includes
#include "Adafruit_NeoPixel.h"

// A class to control a ring light for a fencing scoring machine 
class Fencing_Light
{
  public:

    // Constructor 
    //    uint8_t control_pin - the Arduino pin control terminal of the fencing light  
    Fencing_Light(uint8_t control_pin);

    // Destructor 
    ~Fencing_Light();

    // Lets the object know what the current time is. For the sake of streamlining 
    // the main code, this class should never check the time or call any sort of delay function,
    // but rely on this method to tell it what the time is, and update that way. 
    // if "0" is passed in specifically, we're just updating the display, and no time checks are done 
    void tick(unsigned long current_time_micros); 

    // Illuminate green to show an on-target hit! 
    void light_up_green();

    // Illuminate red to show an on-target hit! 
    void light_up_red();

    // Illuminate green to show an off-target hit! 
    void light_up_white();

    // Show the "touching own lame" signal  
    void light_up_short_circuit_light();

    // control how bright the signals are! 
    void set_brightness(uint8_t brightness);

    // stop showing any lights 
    void go_dark();

    // do something cool to greet the world!  
    void show_off_on_startup();

    // do something cool for 4-4 or 15-15!
    void show_off_on_labelle();


  private: 

    //
    //  Constants and constant-like things 
    //

    // pin controlling the LED ring, not actually a const because it gets mad at assignment 
    uint8_t CONTROL_PIN_;

    // number of LEDs in the ring, inherent in the component 
    const uint8_t LED_COUNT_ = 16; 

    // max brightess value constant; set by underlying library
    const uint8_t MAX_BRIGHTNESS_ = 255; 

    // min brightness value constant; set by underlying library 
    const uint8_t MIN_BRIGHTNESS_ = 0; 

    // readable reference!
    enum color
    {
      RED,
      GREEN,
      WHITE,
      NONE
    };

    // readable references relating to redundancy reduction
    enum display_state
    {
      ALL_RED,
      ALL_GREEN,
      ALL_WHITE,
      DARK
    };
    display_state current_display_state   = display_state::DARK; 
    bool          short_circuit_signal_on = false; 


    //
    // Data members 
    //
    
    // the main sub-library object 
    Adafruit_NeoPixel* led_ring_;
    
    // ring light intensity 
    uint8_t brightness_; 


    // 
    //  instance methods 
    //

    // helper method to make code clean. sets all the pixels in the ring
    //  to the provided color (provided as an enum)
    void set_all_leds_to_color(color color_enum_value);

    // helper method to make code clean, just converts a color enum value 
    //  into an actual underying-class color value (some uint32_t) that 
    //  it can understand  
    uint32_t get_color_code(color color_enum_val);
};

#endif 
