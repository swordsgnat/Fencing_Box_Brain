//============================================================================//
//  Desc    : C++ Interface for a four-character, seven-segment display       //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.1                                                             //
//  Date    : Dec 2022                                                        //
//  Notes   :                                                                 //
//                                                                            // 
//============================================================================//

#ifndef SEVEN_SEGMENT_DISPLAY_H
#define SEVEN_SEGMENT_DISPLAY_H

// global includes
#include <inttypes.h>
#include <Arduino.h>

// local includes 
#include "TM1637.h"


// A class to control a four-character, seven-segment display for a fencing control box
class Seven_Segment_Display
{
  public:

    // Constructor 
    //    uint8_t clock_pin - the Arduino pin attached to the CLK pin of the display 
    //    uint8_t data_pin  - the Arduino pin attached to the DATA pin of the display
    Seven_Segment_Display(uint8_t clock_pin, uint8_t data_pin);

    // Destructor
    ~Seven_Segment_Display();

    // Lets the object know what the current time is. For the sake of streamlining 
    // the main code, this class should never check the time or call any sort of delay function,
    // but rely on this method to tell it what the time is, and update that way. 
    // if "0" is passed in specifically, we're just updating the display, and no time checks are done 
    void tick(unsigned long elapsed_micros); 
    
    // Sets what is shown on the display, and some details about how it is shown. 
    //    std::string data                - the numbers or characters to be shown. The first character will display on the leftmost section, the next
    //                                      on the next leftmost, and so on. 
    //    boolean clock_points            - controls whether the ":" is displayed or not 
    //    boolean will_override           - if true, this message "takes priority" and shows instead of any other future non-override message for 
    //                                      a length of time equal to override_duration_micros, upon which the most recent non-priority message 
    //                                      displays again
    //    unsigned long loop_speed_micros - the length for which the given message has override priority. Meaningless if will_override is false.  
    //    boolean looping_enabled         - if true, the contents of data will "scroll" across the display instead of being shown statically. 
    //    boolean loop_direction_r_to_l   - controls direction of the scroll, right to left if true, left to right if false 
    //    unsigned long loop_speed_micros - controls the speed of the scrolling, in microseconds. 
    void set_display_contents(  String        data                                , 
                                boolean       clock_points              = false   ,
                                boolean       will_override             = false   ,
                                unsigned long override_duration_micros  = 1000000 ,
                                boolean       looping_enabled           = false   , 
                                boolean       loop_direction_r_to_l     = true    , 
                                unsigned long loop_speed_micros         = 1000000
                             );

    // Sets the brightness of the display
    void set_brightness(uint8_t level); 

    // size of display constant
    static const uint8_t DISPLAY_SIZE_ = 4; 


  private:

    // marker for whether the ":" is enabled or not
    // (apparently added to every character in the message) 
    const uint8_t CLOCK_POINTS_DATA_FLAG_ = 0x80;
  
    // pointer to the display object, using its included library 
    TM1637* controlled_display_; 

    // storage for content of the "screen" 
    uint8_t current_display_contents_ [DISPLAY_SIZE_]   = {0x00,0x00,0x00,0x00};

    // storage for content of the "screen", priority message edition 
    uint8_t override_display_contents_[DISPLAY_SIZE_]   = {0x00,0x00,0x00,0x00};

    // when the current priority message was first displayed
    unsigned long override_birth_time_      = 0; 

    // how long the current priority message should be displayed for in total  TODO delte?
    unsigned long override_lifespan_micros_ = 0; 

    // helper method to zero out display storage
    void clear_display_contents(); 

    // helper method to zero out override display storage
    void clear_override_display_contents(); 

    // helper method - translates characters to their seven-segment display byte equivalent, if one exists  
    uint8_t get_display_code_for_character(char character);

    // helper method, compares two uint8_t arrays for identical contents 
    bool check_uint8_t_arrays_for_same_contents(uint8_t first[], uint8_t second[]); 

    // keeping track of what's already up there to avoid redundancy  
    uint8_t most_recently_displayed_contents_ [DISPLAY_SIZE_]   = {0x00,0x00,0x00,0x00};

    // helper method, simplifies some calls and does redundancy checking 
    void send_to_display(uint8_t contents_to_display[]);

    // track the time we're told about to make stopping and starting simpler
    unsigned long most_recently_seen_external_time_ = 0; 
};

#endif 
