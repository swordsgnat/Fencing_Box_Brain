//============================================================================//
//  Desc    : C++ Interface for a Fencing Timer                               //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.1                                                             //
//  Date    : Dec 2022                                                        //
//  Notes   :                                                                 //
//                                                                            // 
//============================================================================//

#ifndef FENCING_CLOCK_H
#define FENCING_CLOCK_H

// local includes 
#include "Seven_Segment_Display.h"

// A class to control a fencing clock
class Fencing_Clock
{
  public:

    // Constructor 
    //    uint8_t clock_pin - the Arduino pin attached to the CLK pin of the clock display 
    //    uint8_t data_pin  - the Arduino pin attached to the DATA pin of the clock display
    Fencing_Clock(uint8_t clock_pin, uint8_t data_pin);

    // Destructor
    ~Fencing_Clock();

    // Lets the object know what the current time is. For the sake of streamlining 
    // the main code, this class should never check the time or call any sort of delay function,
    // but rely on this method to tell it what the time is, and update that way. 
    // if "0" is passed in specifically, we're just updating the display, and no time checks are done 
    void tick(unsigned long current_time_micros); 

    // Set the clock to be running
    void start();

    // Set the clock to be paused 
    void stop();

    // Start the clock if stopped, stop the clock if started 
    void toggle();

    // Return the remaining time left on the clock, in microseconds
    unsigned long get_remaining_micros(); 

    // set the remaining time on the clock
    //    unsigned long new_micros - what the clock should be set to, in microseconds  
    void set_time(unsigned long new_micros);

    // pointer to the display being used as a clock 
    //    I trusted you with public level access to this, okay? So don't abuse it. Be good. 
    //    Only touch it for what you actually need and can't get through the normal interface
    //    methods, like brightness and weird message displays and stuff. Okay?
    Seven_Segment_Display* clock_; 

    // time constants for public use  
    const static unsigned long MICROS_IN_SEC_          = 1000000; 
    const static unsigned long SECS_IN_MIN_            = 60; 
 
  private:

    // time constants only relevant to this 
    const unsigned long STARTING_MICROS_        = 3  * SECS_IN_MIN_ * MICROS_IN_SEC_; 
    const unsigned long MAX_MICROS_             = 70 * SECS_IN_MIN_ * MICROS_IN_SEC_; // due to unsigned long constraints  

    // track the clock's active status; we want to begin with the clock paused 
    boolean is_running_ = false;  

    // track how much is left on the timer 
    unsigned long current_clock_time_micros_ = this->STARTING_MICROS_; 

    // helper method; make conversion from unformatted microseconds to human-readable time string easy  
    String get_time_string_from_micros(unsigned long microsecs);

    // track the time since we started to do time math better 
    unsigned long time_of_most_recent_start_        = 0; 

    // track the time we're told about to make stopping and starting simpler
    unsigned long most_recently_seen_external_time_ = 0; 
};

#endif 
