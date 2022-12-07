//============================================================================//
//  Desc    : C++ Interface for two displays showing the points scored by     //   
//            each fencer                                                     //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.0                                                             //
//  Date    : Nov 2022                                                        //
//  Notes   : - All display updating should be done via tick(0)               //
//            to centralize message processing in one spot                    // 
//============================================================================//

// interface include
#include "Fencing_Point_Displays.h"

// Constructor 
//    uint8_t left_fencer_clock_pin  - the Arduino pin attached to the CLK pin of the left fencer's scoreboard 
//    uint8_t left_fencer_data_pin   - the Arduino pin attached to the DATA pin the left fencer's scoreboard 
//    uint8_t right_fencer_clock_pin - the Arduino pin attached to the CLK pin of the right fencer's scoreboard 
//    uint8_t right_fencer_data_pin  - the Arduino pin attached to the DATA pin of the right fencer's scoreboard 
Fencing_Point_Displays::Fencing_Point_Displays(uint8_t left_fencer_clock_pin, uint8_t left_fencer_data_pin, uint8_t right_fencer_clock_pin, uint8_t right_fencer_data_pin)
{

  // initialize the SSD pointers 
  this-> left_fencer_score_display_ = new Seven_Segment_Display( left_fencer_clock_pin,  left_fencer_data_pin);
  this->right_fencer_score_display_ = new Seven_Segment_Display(right_fencer_clock_pin, right_fencer_data_pin);
  
  // zero the scores 
  this-> left_fencer_score_ = 0;  
  this->right_fencer_score_ = 0; 

  // make sure everything's displayed properly
  this->tick(0);
}


// Destructor
Fencing_Point_Displays::~Fencing_Point_Displays()
{
  delete this-> left_fencer_score_display_;
  delete this->right_fencer_score_display_;
}


// Lets the object know how much time has passed. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it how much time has passed, and update that way. 
void Fencing_Point_Displays::tick(int elapsed_micros)
{
  String score_string = ""; 

  // set and pad a string for proper display / to easily do string conversion 
  score_string += this->left_fencer_score_;
  while (score_string.length() < Seven_Segment_Display::DISPLAY_SIZE_) score_string = " " + score_string;
  
  // set the contents (abuse automatic string concatenation promotion behavior) 
  this-> left_fencer_score_display_->set_display_contents(score_string); 

  // set and pad a string for proper display / to easily do string conversion 
  score_string = ""; 
  score_string += this->right_fencer_score_;
  while (score_string.length() < Seven_Segment_Display::DISPLAY_SIZE_) score_string = " " + score_string;

  // set the contents (abuse automatic string concatenation promotion behavior) 
  this->right_fencer_score_display_->set_display_contents(score_string); 
  
  // tell the underlying SSDs how much time has passed so it will update
  // it's also important for overriding messages and stuff 
  this-> left_fencer_score_display_->tick(elapsed_micros);
  this->right_fencer_score_display_->tick(elapsed_micros);
}


// Hopefully all self-explanatory 
void Fencing_Point_Displays::set_scores(int left_fencer_score, int right_fencer_score)
{
  this-> left_fencer_score_ =  left_fencer_score;  
  this->right_fencer_score_ = right_fencer_score;

  // update the display 
  this->tick(0); 
}

void Fencing_Point_Displays::increment_left_fencer_score()
{
  this-> left_fencer_score_ += 1; 
    
  // update the display 
  this->tick(0); 
}

void Fencing_Point_Displays::increment_right_fencer_score()
{
  this->right_fencer_score_ += 1;   

  // update the display 
  this->tick(0); 
}

void Fencing_Point_Displays::decrement_left_fencer_score()
{
  this->left_fencer_score_ -= 1; 
  if (this->left_fencer_score_ < 0) this->left_fencer_score_ = 0; 
  
  // update the display 
  this->tick(0); 
}

void Fencing_Point_Displays::decrement_right_fencer_score()
{
  this->right_fencer_score_ -= 1; 
  if (this->right_fencer_score_ < 0) this->right_fencer_score_ = 0; 
  
  // update the display 
  this->tick(0); 
}

int  Fencing_Point_Displays::get_left_fencer_score()
{
  return this->left_fencer_score_;
}

int  Fencing_Point_Displays::get_right_fencer_score() 
{
  return this->right_fencer_score_;
}
