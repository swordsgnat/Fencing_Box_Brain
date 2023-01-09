//============================================================================//
//  Name    : Fencing_Clock.cpp                                               //
//  Desc    : C++ Implementation for a timeer to track a fencing match        //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.2                                                             //
//  Date    : Jan 2023                                                        //
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
  // make the underlying SSD object 
  this->clock_ = new Seven_Segment_Display(clock_pin, data_pin);
  
  // make sure everything's displayed properly
  this->tick(0);
}

// Destructor
Fencing_Clock::~Fencing_Clock()
{
  delete this->clock_; 
}


// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating the display, and no time checks are done 
void Fencing_Clock::tick(unsigned long current_time_micros)
{

  // if we're not just updating...
  if (current_time_micros != 0) 
  {
    // track the new timestamp
    this->most_recently_seen_external_time_ = current_time_micros; 
  } 

  // either way, we're gonna need the remaining time 
  unsigned long remaining_micros = this->get_remaining_micros(); 

  // if we're running but out of time, we should stop running
  if (remaining_micros == 0 && this->is_running_)
  {
      this->stop(); // handles the time setting itself 
  }

  // use int math abuse to see how many whole seconds are left on the clock 
  unsigned long remaining_whole_seconds = remaining_micros / this->MICROS_IN_SEC_; 

  // redundancy check - don't bother figuring out what to display for the time 
  //  if the number of whole seconds hasn't changed 
  if (this->last_sent_number_of_whole_seconds_ != remaining_whole_seconds)
  {  
    // do the actual displaying 
    this->clock_->set_display_contents(this->get_time_string_from_micros(remaining_micros), true); 

    // update our understanding of the last time sent 
    this->last_sent_number_of_whole_seconds_ = remaining_whole_seconds; 
  }
  
  // tell the underlying SSDs how much time has passed so it will update
  // it's also important for overriding messages and stuff 
  this->clock_->tick(current_time_micros);
}


// Set the clock to be running
void Fencing_Clock::start()
{
  if (!this->is_running_)   // we only need to worry if we're not already running 
  {
      this->is_running_                 = true; 
      this->time_of_most_recent_start_  = this->most_recently_seen_external_time_; 
  }
}


// Set the clock to be paused 
void Fencing_Clock::stop()
{
  if (this->is_running_) // we only need to worry if we're already running 
  {
    // do the last calculation before stopping the time 
    unsigned long new_time = this->get_remaining_micros();

    // stop the counting of time 
    this->is_running_ = false;
    
    // set the internal micros count using our own method 
    this->set_time(new_time);

    // zero the start tracker (NOT the most recent time seen!) 
    this->time_of_most_recent_start_ = 0;  
  }
}


// Start the clock if stopped, stop the clock if started 
void Fencing_Clock::toggle()
{
  if (this->is_running_)
  {
    this->stop();     
  }
  else
  {
    this->start();  
  }
}


// Return the remaining time left on the clock, in microseconds
unsigned long Fencing_Clock::get_remaining_micros()
{
  unsigned long remaining_micros = 0; 

  if (this->is_running_)
  {
    // calculate the time passed since the most recent start command 
    unsigned long elapsed_time = (unsigned long)(this->most_recently_seen_external_time_ - this->time_of_most_recent_start_); 
    
    if (elapsed_time >= this->current_clock_time_micros_) // dodge overflow issues if we're out of time 
    {
      remaining_micros = 0;   
    }
    else // otherwise calculate the remaining time 
    {
      remaining_micros = this->current_clock_time_micros_- elapsed_time;
    }
  }
  else // i.e., we're not running 
  {
    remaining_micros = this->current_clock_time_micros_; 
  }
  
  return remaining_micros; 
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
  // start off with a blank string the size of our display
  String return_string = "    "; // TODO kind of a magic number; should really use Seven_Segment_Display::DISPLAY_SIZE_ I guess
                                 //      this is just the fastest way I've found so far 

  // check your inputs so you don't overflow the timer  
  if (microsecs > this->MAX_MICROS_) microsecs = this->MAX_MICROS_;

  // abuse int math to get whole number of ten-mins-chunks
  uint8_t tens_of_minutes = microsecs / (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_ * 10); 
  if (tens_of_minutes != 0) 
  {
    return_string.setCharAt(0, '0' + tens_of_minutes);  // [0] is the first digit in the time string and therefore the tens-of-minutes place 
  }
  
  // modulo gets the remainder (micros minus all the ten minutes) which we can then further convert (and int math drops off decimals)
  uint8_t minutes         = (microsecs % (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_ * 10)) / (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_); 
  if (!(minutes == 0 && tens_of_minutes == 0)) // if they're both zero, you want to leave it blank (no digits so far)
  {
    return_string.setCharAt(1, '0' + minutes);  // [1] is the second digit in the time string and therefore the minutes place 
  }

  // modulo gets the remainder (micros minus all the minutes) which we can then further convert (and int math drops off decimals)
  uint8_t tens_of_seconds = (microsecs % (this->MICROS_IN_SEC_ * this->SECS_IN_MIN_)) / (this->MICROS_IN_SEC_ * 10); 
  if (!(tens_of_seconds == 0 && minutes == 0 && tens_of_minutes == 0)) // if they're all zero, you want to leave it blank (no digits so far)
  {
    return_string.setCharAt(2, '0' + tens_of_seconds);  // [2] is the third digit in the time string and therefore the tens-of-seconds place 
  }

  // modulo gets the remainder (micros minus all the ten-second-chunks) which we can then further convert (and int math drops off decimals)
  // [3] is the fourth digit in the time string and therefore the seconds place 
  return_string.setCharAt(3, '0' + ((microsecs % (this->MICROS_IN_SEC_ * 10)) / (this->MICROS_IN_SEC_)) ); 

  // send it back
  return return_string;
}
