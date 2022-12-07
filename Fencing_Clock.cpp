//============================================================================//
//  Desc    : C++ Implementation for a four-character, seven-segment display  //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.0                                                             //
//  Date    : Nov 2022                                                        //
//  Notes   : - All display updating should be done via tick(0)               //
//            to centralize message processing in one spot                    // 
//============================================================================//

// interface include
#include "Fencing_Clock.h"

// Constructor 
//    uint8_t clock_pin - the Arduino pin attached to the CLK pin of the clock display 
//    uint8_t data_pin  - the Arduino pin attached to the DATA pin of the clock display
Fencing_Clock::Fencing_Clock(uint8_t clock_pin, uint8_t data_pin)
{
  this->clock_ = new Seven_Segment_Display(clock_pin, data_pin);
  
  this->is_running_ = false; 
  
  this->current_clock_time_micros_ = this->STARTING_MICROS_; 

  // make sure everything's displayed properly
  this->tick(0);
}

// Destructor
Fencing_Clock::~Fencing_Clock()
{
  delete this->clock_; 
}


// Lets the object know how much time has passed. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it how much time has passed, and update that way. 
void Fencing_Clock::tick(int elapsed_micros)
{
  if (this->is_running_) 
  {
    // if we're out of time, automatically stop running and finish zeroing the time 
    if (elapsed_micros >= this->current_clock_time_micros_) 
    {
      this->is_running_                 = false; 
      this->current_clock_time_micros_  = 0;  
    }
    else
    {
      this->current_clock_time_micros_ -= elapsed_micros;  
    }
  } 

  // do the actual displaying 
  this->clock_->set_display_contents(this->get_time_string_from_micros(this->current_clock_time_micros_), true); 
  
  // tell the underlying SSDs how much time has passed so it will update
  // it's also important for overriding messages and stuff 
  this->clock_->tick(elapsed_micros);
}


// Set the clock to be running
void Fencing_Clock::start()
{
  this->is_running_ = true; 
}


// Set the clock to be paused 
void Fencing_Clock::stop()
{
  this->is_running_ = false; 
}


// Start the clock if stopped, stop the clock if started 
void Fencing_Clock::toggle()
{
  this->is_running_ = !this->is_running_;
}


// Return the remaining time left on the clock, in microseconds
unsigned long Fencing_Clock::get_remaining_micros()
{
  return current_clock_time_micros_;
}


// set the remaining time on the clock
//    unsigned long new_micros - what the clock should be set to, in microseconds  
void Fencing_Clock::set_time(unsigned long new_micros)
{
  // don't adjust on the fly! 
  if(this->is_running_)
  {
    this->stop();  
  }
  
  // check your inputs so you don't overflow the timer  
  if (new_micros > this->MAX_MICROS_) new_micros = this->MAX_MICROS_;

  // set the tracking data member 
  this->current_clock_time_micros_ = new_micros; 

  // call tick with no time just to update the display 
  this->tick(0); 
}


//
//  private methods
//

// helper method; make conversion from unformatted microseconds to human-readable time string easy  
String Fencing_Clock::get_time_string_from_micros(unsigned long microsecs)
{
  String return_string    = ""; 

  // check your inputs so you don't overflow the timer  
  if (microsecs > this->MAX_MICROS_) microsecs = this->MAX_MICROS_;

  // abuse int math to get whole number of ten-mins-chunks
  uint8_t tens_of_minutes = microsecs / (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_ * 10); 
  if (tens_of_minutes != 0) 
  {
    return_string        += tens_of_minutes;  
  }
  else
  {
    return_string        += " ";
  }
    
  // modulo gets the remainder (micros minus all the ten minutes) which we can then further convert (and int math drops off decimals)
  uint8_t minutes         = (microsecs % (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_ * 10)) / (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_); 
  if (minutes == 0 && return_string == " ") // one blank, for if the minutes tens place was zero 
  {
    return_string        += " ";  
  }
  else
  {
    return_string        += minutes;      
  }


  // modulo gets the remainder (micros minus all the minutes) which we can then further convert (and int math drops off decimals)
  uint8_t tens_of_seconds = (microsecs % (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_)) / (this->MICROS_IN_SEC_ * 10); 
  if (tens_of_seconds == 0 && return_string == "  ") // two blanks, for if both minute numbers were 0 
  {
    return_string        += " "; 
  }
  else
  {
    return_string        += tens_of_seconds;  
  }

  // modulo gets the remainder (micros minus all the ten-second-chunks) which we can then further convert (and int math drops off decimals)
  return_string          += (microsecs % (this->MICROS_IN_SEC_ * 10)) / (this->MICROS_IN_SEC_); 

  // send it back
  return return_string;
}
