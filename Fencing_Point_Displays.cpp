//============================================================================//
//  Desc    : C++ Interface for two displays showing the points scored by     //   
//            each fencer                                                     //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.2                                                             //
//  Date    : Jan 2023                                                        //
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

  // make sure everything's displayed properly
  this->handle_score_change(); 
}


// Destructor
Fencing_Point_Displays::~Fencing_Point_Displays()
{
  delete this-> left_fencer_score_display_;
  delete this->right_fencer_score_display_;
}


// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating the displays, and no time checks are done 
// (this one's just a pass-through, effectively) 
void Fencing_Point_Displays::tick(unsigned long current_time_micros)
{
  // tell the underlying SSDs how much time has passed so it will update
  // it's also important for overriding messages and stuff 
  this-> left_fencer_score_display_->tick(current_time_micros);
  this->right_fencer_score_display_->tick(current_time_micros);
}


// Hopefully all self-explanatory 
void Fencing_Point_Displays::set_scores(int left_fencer_score, int right_fencer_score)
{
  this-> left_fencer_score_ =  left_fencer_score;  
  this->right_fencer_score_ = right_fencer_score;

  // update the displays if need be 
  this->handle_score_change(); 
}


void Fencing_Point_Displays::increment_left_fencer_score()
{
  this-> left_fencer_score_ += 1; 
    
  // update the displays if need be 
  this->handle_score_change(); 
}


void Fencing_Point_Displays::increment_right_fencer_score()
{
  this->right_fencer_score_ += 1;   

  // update the displays if need be 
  this->handle_score_change(); 
}


void Fencing_Point_Displays::decrement_left_fencer_score()
{
  this->left_fencer_score_ -= 1; 
  if (this->left_fencer_score_ < 0) this->left_fencer_score_ = 0; 
  
  // update the displays if need be 
  this->handle_score_change(); 
}


void Fencing_Point_Displays::decrement_right_fencer_score()
{
  this->right_fencer_score_ -= 1; 
  if (this->right_fencer_score_ < 0) this->right_fencer_score_ = 0; 

  // update the displays if need be 
  this->handle_score_change(); 
}

int  Fencing_Point_Displays::get_left_fencer_score()
{
  return this->left_fencer_score_;
}


int  Fencing_Point_Displays::get_right_fencer_score() 
{
  return this->right_fencer_score_;
}


// helper method to pad out score strings and update the underlying SSDs as to the new message to display
// only if the scores have actually changed. 
void Fencing_Point_Displays::handle_score_change()
{
  // start off with a blank string the size of our display
  String score_string = "    "; // TODO kind of a magic number; should really use Seven_Segment_Display::DISPLAY_SIZE_ I guess
                                //      this is just the fastest way I've found so far 

  uint8_t thousands, hundreds, tens, ones; 
  
  // don't wanna work if we don't have to 
  if (this->left_fencer_score_ != this->previous_left_fencer_score_)
  {
    // update the previously seen score 
    this->previous_left_fencer_score_ = this->left_fencer_score_; 

    thousands = this->left_fencer_score_ / 1000;          // abusing int math truncation here 
    if(thousands != 0)                                    // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(0, '0' + thousands);         // [0] on our score string is the thousands place; abusing "'0' +" to get the right char val 
    }

    hundreds = (this->left_fencer_score_ % 1000) / 100;   // abusing int math truncation here 
    if(!(hundreds == 0 && thousands == 0))                // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(1, '0' + hundreds);          // [1] on our score string is the hundreds place; abusing "'0' +" to get the right char val 
    }

    tens = (this->left_fencer_score_ % 100) / 10;         // abusing int math truncation here 
    if(!(tens == 0 && hundreds == 0 && thousands == 0))   // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(2, '0' + tens);              // [2] on our score string is the tens place; abusing "'0' +" to get the right char val 
    }

    ones = (this->left_fencer_score_ % 10);               // abusing int math truncation here 
    score_string.setCharAt(3, '0' + ones);                // [3] on our score string is the ones place; abusing "'0' +" to get the right char val 
    
    // set the contents 
    this->left_fencer_score_display_->set_display_contents(score_string); 
  }

  // rinse and repeat for the right fencer 
  score_string = "    ";
  if (this->right_fencer_score_ != this->previous_right_fencer_score_)
  {
    // update the previously seen score 
    this->previous_right_fencer_score_ = this->right_fencer_score_; 

    thousands = this->right_fencer_score_ / 1000;         // abusing int math truncation here 
    if(thousands != 0)                                    // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(0, '0' + thousands);         // [0] on our score string is the thousands place; abusing "'0' +" to get the right char val 
    }

    hundreds = (this->right_fencer_score_ % 1000) / 100;  // abusing int math truncation here 
    if(!(hundreds == 0 && thousands == 0))                // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(1, '0' + hundreds);          // [1] on our score string is the hundreds place; abusing "'0' +" to get the right char val 
    }

    tens = (this->right_fencer_score_ % 100) / 10;        // abusing int math truncation here 
    if(!(tens == 0 && hundreds == 0 && thousands == 0))   // don't print if you're gonna be a leading zero 
    {
      score_string.setCharAt(2, '0' + tens);              // [2] on our score string is the tens place; abusing "'0' +" to get the right char val 
    }

    ones = (this->right_fencer_score_ % 10);              // abusing int math truncation here 
    score_string.setCharAt(3, '0' + ones);                // [3] on our score string is the ones place; abusing "'0' +" to get the right char val 

    
    // set the contents 
    this->right_fencer_score_display_->set_display_contents(score_string); 
  }

  // update the display 
    // TODO technically don't need to do this - it'll catch the update on the very next tick? But is there any harm?
    // I guess it helps in the case of a sudden super long delay in ticks?
  this->tick(0); 
}
