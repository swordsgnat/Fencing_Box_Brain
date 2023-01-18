//============================================================================//
//  Name    : Buzzer.cpp                                                      //
//  Desc    : C++ Implementation for a buzzer                                 //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.2                                                             //
//  Date    : Jan 2023                                                        //
//  Notes   :                                                                 //
//============================================================================//

// TODO maybe lights should also control their own duration? or overload it so they can if they want to? 
// TODO a separate sound for holding the remote button as opposed to pushing it?
// TODO have every component set it own pin output mode and stuff, not the main brain code 
// TODO document how I'm wiring this weird somewhere - buzzer signal to ground (always scream); buzzer Vin to actual signal input = activate-high buzzer! 
// TODO something cool for timeout?

// interface include
#include "Buzzer.h"


// Constructor 
//    uint8_t control_pin - the Arduino pin control terminal of the buzzer  
Buzzer::Buzzer(uint8_t control_pin)
{
  // save and setup the control pin 
  this->BUZZER_PIN_         = control_pin; 
  pinMode(this->BUZZER_PIN_, OUTPUT); 

  // initialize ready to make noise 
  this->quiet_mode_enabled_ = false; 
}


// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating, and no time checks are done 
void Buzzer::tick(unsigned long current_time_micros)
{
  // currently no-op - tone() doesn't block, somehow!
}

// Emit a little "blip" to let the user know they've pressed a button correctly 
// Takes in a duration in microseconds (to keep consistent with other timing code) 
// NB: defaults to this->DEFAULT_CHIRP_DURATION_ (see .h file) 
void Buzzer::chirp(unsigned long chirp_duration_micros)
{
  if (!this->quiet_mode_enabled_)
  {
    tone(this->BUZZER_PIN_, this->CHIRP_NOTE_, chirp_duration_micros / this->MICROS_IN_MILLI_);
  }
}

// Emit a loud, noticable tone to let the fencers know a touch has been scored 
// Takes in a duration in microseconds (to keep consistent with other timing code) 
// NB: defaults to this->DEFAULT_BUZZ_DURATION_ (see .h file) 
void Buzzer::buzz(unsigned long buzz_duration_micros)
{
  if (!this->quiet_mode_enabled_)
  {
    tone(this->BUZZER_PIN_, this->BUZZ_NOTE_, buzz_duration_micros / this->MICROS_IN_MILLI_);
  }
}

// Stop emitting sound // TODO protection against calling this in a loop?
void Buzzer::silence()
{
  noTone(this->BUZZER_PIN_);
}

// play a cute little sequence to greet the world 
void Buzzer::play_startup_trill()
{
  // currently NO-OP
  if (!this->quiet_mode_enabled_)
  {
    // eventually
  }
}

// play something cool for 4-4 or 15-15
void Buzzer::play_labelle_trill()
{
  // currently NO-OP  
  if (!this->quiet_mode_enabled_)
  {
    // eventually
  }
}

// set whether the buzzer sounds at all 
void Buzzer::set_quiet_mode(bool enabled_or_not)
{
  this->quiet_mode_enabled_ = enabled_or_not;  
}
