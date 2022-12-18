//============================================================================//
//  Desc    : C++ Implementation for a buzzer                                 //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.1                                                             //
//  Date    : Dec 2022                                                        //
//  Notes   :                                                                 //
//                                                                            // 
//============================================================================//


// TODO this controls shriek length for points, not main brain code
// TODO also shriek is cute but let's just make it point indicator or something 
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
  this->BUZZER_PIN_ = control_pin; 
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
void Buzzer::chirp()
{
  if (!this->quiet_mode_enabled_)
  {
    tone(this->BUZZER_PIN_, this->CHIRP_NOTE_, this->CHIRP_LIFESPAN_MILLIS_);
  }
}

// Start emitting a loud, noticable tone to let the fencers know a touch has been scored 
void Buzzer::start_shrieking()
{
  if (!this->quiet_mode_enabled_)
  {
    tone(this->BUZZER_PIN_, this->SHRIEK_NOTE_, this->BUZZER_DURATION_MICROS_);
  }
}

// Stop emitting the "scored" tone
void Buzzer::stop_shrieking()
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
