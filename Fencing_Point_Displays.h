//============================================================================//
//  Desc    : C++ Interface for two displays showing the points scored by     //
//            each fencer.                                                    //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.0                                                             //
//  Date    : Nov 2022                                                        //
//  Notes   :                                                                 //
//                                                                            // 
//============================================================================//

#ifndef FENCING_POINT_DISPLAYS_H
#define FENCING_POINT_DISPLAYS_H

// local includes 
#include "Seven_Segment_Display.h"

// A class to control a fencing clock
class Fencing_Point_Displays
{
  public:

    // Constructor 
    //    uint8_t left_fencer_clock_pin  - the Arduino pin attached to the CLK pin of the left fencer's scoreboard 
    //    uint8_t left_fencer_data_pin   - the Arduino pin attached to the DATA pin the left fencer's scoreboard 
    //    uint8_t right_fencer_clock_pin - the Arduino pin attached to the CLK pin of the right fencer's scoreboard 
    //    uint8_t right_fencer_data_pin  - the Arduino pin attached to the DATA pin of the right fencer's scoreboard 
    Fencing_Point_Displays(uint8_t left_fencer_clock_pin, uint8_t left_fencer_data_pin, uint8_t right_fencer_clock_pin, uint8_t right_fencer_data_pin);

    // Destructor
    ~Fencing_Point_Displays();

    // Lets the object know how much time has passed. For the sake of streamlining 
    // the main code, this class should never check the time or call any sort of delay function,
    // but rely on this method to tell it how much time has passed, and update that way. 
    void tick(int elapsed_micros); 

    // Hopefully all self-explanatory 
    void set_scores(int left_fencer_score, int right_fencer_score);
    void increment_left_fencer_score(); 
    void increment_right_fencer_score(); 
    void decrement_left_fencer_score(); 
    void decrement_right_fencer_score(); 
    int  get_left_fencer_score();
    int  get_right_fencer_score(); 

    // the displays being used as scoreboards 
    //    I trusted you with public level access to these, okay? So don't abuse it. Be good. 
    //    Only touch them for what you actually need and can't get through the normal interface
    //    methods, like brightness and weird message displays and stuff. Okay?
    Seven_Segment_Display*  left_fencer_score_display_; 
    Seven_Segment_Display* right_fencer_score_display_; 

 
  private:

   int  left_fencer_score_; 
   int right_fencer_score_; 
};

#endif 
