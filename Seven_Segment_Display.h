//============================================================================//
//  Name    : Seven_Segment_Display.h                                         //
//  Desc    : C++ Interface for a four-character, seven-segment display       //
//            in the TM1637 style                                             //
//  Dev     : Frankie.Chu (original implementation)                           //
//            Nate Cope   (adaptation and expansion)                          //
//  Version : 1.2                                                             //
//  Date    : Jan 2023                                                        //
//  Notes   : - All display updating should be done via tick(0)               //
//              to centralize message processing in one spot                  // 
//            - Many thanks to Frankie.Chu for his original example           //
//              example implementation                                        // 
//            - The clock pin broadly tells the display WHEN to receive data, // 
//              and the data pin broadly tells the display WHAT the data is.  //
//              We've developed an incremental way to update them in order to //
//              optimize for speed, where little bits of new messages are     //
//              sent every loop instead of a full update. Under this tactic,  // 
//              it's totally fine if displays share their data pins (since    //
//              they won't be told via their clock pin to pick up that data   //
//              if its not meant for them) but it DOES NOT WORK if displays   //
//              share their clock pins (since they have no way of knowing     //
//              what data is meant for them if they're all getting the same   //
//              timing signals                                                //
//============================================================================//

#ifndef SEVEN_SEGMENT_DISPLAY_H
#define SEVEN_SEGMENT_DISPLAY_H

// global includes
#include <inttypes.h>
#include <Arduino.h>

// A class to control a four-character, seven-segment display for a fencing control box
class Seven_Segment_Display
{
  public:

    //
    //  methods 
    //

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


    //
    //  constants 
    //

    // size of display constant
    static const uint8_t  DISPLAY_SIZE_   = 4; 

    // brightness constants for anyone to use  
    static const uint8_t  BRIGHT_DARKEST_ = 0; 
    static const uint8_t  BRIGHT_TYPICAL_ = 2;
    static const uint8_t  BRIGHTEST_      = 7;


  private:

    //
    //  methods 
    //

    // helper method to zero out display storage
    void clear_normal_display_message(); 

    // helper method to zero out override display storage
    void clear_override_display_message(); 

    // helper method - translates characters to their seven-segment display byte equivalent, if one exists  
    uint8_t get_display_code_for_character(char character);

    // helper method; checks for and prepares any new values for sending; ignores the request if redundant
    void stage_message_for_sending(uint8_t contents_to_display[]);

    // performs next step in sending of pending message; does redundancy checking 
    void step_incremental_display(); 

    // helper method; writes a single byte to the TM1637 style Seven-Segment Display
    void writeByte(int8_t wr_data);

    // helper method; sends the "prepare to receive command / data" signal to the TM1637 style Seven-Segment Display
    void send_signal_start();

    // helper method; sends the "finish receiving command / data" signal to the TM1637 style Seven-Segment Display
    void send_signal_stop();

    // DEPRECATED; one-shot method to send one value to a given index of the SSD
    void change_single_value(uint8_t index, uint8_t new_value);

    // DEPRECATED; one-shot method to send one four-value set to the SSD
    void change_whole_message(uint8_t contents_to_display[]); 

  
    //
    //  data members 
    //

    // storage for content of the "screen" 
    uint8_t normal_display_message_  [DISPLAY_SIZE_]      = {0x00,0x00,0x00,0x00};

    // storage for content of the "screen", priority message edition 
    uint8_t override_display_message_[DISPLAY_SIZE_]      = {0x00,0x00,0x00,0x00};

    // keeping track of what's already up there to avoid redundancy  
    uint8_t current_display_contents_ [DISPLAY_SIZE_]     = {0x00,0x00,0x00,0x00};

    // keep track of what we're currently trying to send to the display 
    uint8_t new_message_to_be_displayed_ [DISPLAY_SIZE_]  = {0x00,0x00,0x00,0x00};

    // when the current priority message was first displayed
    unsigned long override_birth_time_                    = 0; 

    // how long the current priority message should be displayed for in total  
    unsigned long override_lifespan_micros_               = 0; 

    // track the time we're told about to make stopping and starting simpler
    unsigned long most_recently_seen_external_time_       = 0;

    // SSD pin values 
    uint8_t clock_pin_;
    uint8_t data_pin_;

    // encoding for T1637 brightness 
    uint8_t brightness_bit_                               = BRIGHTNESS_BASE_ + BRIGHT_TYPICAL_;

    // keep track of whether a new message has arrived to begin sending once you get a chance 
    bool new_message_waiting_for_send_begin_              = false;  

    // keep track of which step of sending a message you've already completed  
    uint8_t incremental_display_step_                     = DISPLAY_SIZE_; 


    //
    //  constants 
    //

    // marker for whether the ":" is enabled or not
    // (apparently added to every character in the message) 
    const uint8_t CLOCK_POINTS_DATA_FLAG_ = 0x80;

    // TM1637 built-in constants for commands and brightness values 
    static const uint8_t BRIGHTNESS_BASE_ = 0x88; 
    static const uint8_t ADDR_AUTO_       = 0x40;
    static const uint8_t ADDR_FIXED_      = 0x44;
    static const uint8_t CMD_SET_ADDR_    = 0xc0;
};

#endif 
