//============================================================================//
//  Name    : Fencing_Box_Brain.ino                                           //
//  Desc    : Arduino Code to Implement a Fencing Scoring Machine             //
//  Dev     : Wnew (base), Nate Cope (updates)                                //
//  Date    : Nov  2012 (base)                                                //
//  Updated : Sept 2015 (base), Sept 2022 - Jan 2023 (updates)                //
//  Version : 2.1                                                             //
//  Notes   : 1. [inherited] Basis of algorithm from digitalwestie on github. // 
//                           Thanks Mate                                      //
//            2. [inherited] Used uint8_t instead of unsigned long where      //
//                           possible to optimise                             //
//============================================================================//

// TODO short circuit lights
// TODO check every TODO in the other files
// TODO components mostly set their own pin states?
// TODO changing mode during running clock???
// TODO THIS code implements not singaling hits if there's no time on the clock
// TODO loop()'s order has to be figured out...
// TODO labelle checks, someday
// TODO explain the fucken var_ vs. var thing
// TODO why are the off / on target args unsigned longs??
// TODO "Name:" entry in each heading paragraph thing 
// TODO probably supposed to have the GNU general liscence in this file up top or something 
// TODO changing point values should stop the clock? maybe?

//============
// #defines
//============
#define DEBUG 0 // 1 == weapon testing, 2 = main loop timing

//============
// #includes
//============


// Local Includes
#include "Fencing_Clock.h"
#include "Fencing_Point_Displays.h"
#include "Fencing_Light_Displays.h"
#include "Buzzer.h"


//============
// Constants
//============

// Input Pins
const uint8_t REMOTE_INPUT_BUTTON_A_PIN_            = A0; // Button labelled "A" on the remote control
const uint8_t REMOTE_INPUT_BUTTON_B_PIN_            = A1; // Button labelled "B" on the remote control
const uint8_t REMOTE_INPUT_BUTTON_C_PIN_            = A2; // Button labelled "C" on the remote control
const uint8_t REMOTE_INPUT_BUTTON_D_PIN_            = 7; // Button labelled "D" on the remote control
const uint8_t MODE_SWITCH_BUTTON_PIN_               = 3; // Button for switching modes
const uint8_t CLOCK_TIME_INCREMENT_BUTTON_PIN_      = 4; // Button for increasing time on the score clock
const uint8_t CLOCK_TIME_DECREMENT_BUTTON_PIN_      = 5; // Button for decreasing time on the score clock
const uint8_t QUIET_MODE_BUTTON_PIN_                = 20; // Button for turning buzzer off

// Fencing Equipment Input Pins
const uint8_t LEFT_FENCER_WEAPON_PIN_               = A3;    // Analog
const uint8_t LEFT_FENCER_LAME_PIN_                 = A4;    // Analog (Epee return path)
const uint8_t RIGHT_FENCER_WEAPON_PIN_              = A5;    // Analog
const uint8_t RIGHT_FENCER_LAME_PIN_                = A6;   // Analog (Epee return path)

// Output Pins
const uint8_t LEFT_FENCER_SCORE_DISPLAY_CLK_PIN_    = 12;  // pin for communication with left fencer scoreboard
const uint8_t LEFT_FENCER_SCORE_DISPLAY_DATA_PIN_   = 11; // pin for communication with left fencer scoreboard
const uint8_t RIGHT_FENCER_SCORE_DISPLAY_CLK_PIN_   = LEFT_FENCER_SCORE_DISPLAY_CLK_PIN_;  // it's not a problem if they share!
const uint8_t RIGHT_FENCER_SCORE_DISPLAY_DATA_PIN_  = 10; // pin for communication with right fencer scoreboard
const uint8_t TIME_DISPLAY_CLK_PIN_                 = LEFT_FENCER_SCORE_DISPLAY_CLK_PIN_;  // it's not a problem if they share!
const uint8_t TIME_DISPLAY_DATA_PIN_                = 9; // pin for communication with timer
const uint8_t LEFT_FENCER_RING_LIGHT_CONTROL_PIN_   = 13; // pin for communication with left fencer scoring light
const uint8_t RIGHT_FENCER_RING_LIGHT_CONTROL_PIN_  = 8; // pin for communication with right fencer scoring light
const uint8_t BUZZER_CONTROL_PIN_                   = 6;  // pin for sending commands to buzzer module

// enums (currently just mode)
enum mode
{
  SABER,
  FOIL,
  EPEE
  //SABER_DUAL_HIT_PRACTICE,
  //FOIL_DUAL_HIT_PRACTICE,
  //EPEE_DUAL_HIT_PRACTICE,
  //SABER_CONTINUITY,
  //FOIL_CONTINUITY,
  //EPEE_CONTINUITY
  // if you add modes later, you gotta go change the mode wraparound in the handle_mode_switch_button() method
};

// Timing Constants
const unsigned long MICROS_IN_SEC                       = 1000000;                // conversion constant; "avoiding magic numbers"
const unsigned long BUZZER_DURATION_MICROS              = 1 * MICROS_IN_SEC;      // length of time the buzzer is kept on after a hit (microseconds)
const unsigned long LIGHT_DURATION_MICROS               = 3 * MICROS_IN_SEC;      // length of time the lights are kept on after a hit ((microseconds)
const unsigned long BAUDRATE                            = 57600;//9600;                  // baudrate of the serial debug interface
const unsigned long CLOCK_ADJUSTMENT_RATE_TICK_MICROS   = 0.333 * MICROS_IN_SEC;  // how fast the time increments / decrements when the corresponding button is held down
const unsigned long CLOCK_ADJUSTMENT_RATE_CHANGE_MICROS = 2 * MICROS_IN_SEC;      // how fast the time increments / decrements increase in size when the corresponding button is held down
const unsigned long CLOCK_ADJUSTMENT_LEVEL_MICROS_ []   = { 1  * MICROS_IN_SEC,
                                                            10 * MICROS_IN_SEC,   // rates for adding / subtracting time from the clock
                                                            30 * MICROS_IN_SEC,
                                                            60 * MICROS_IN_SEC
                                                          };
const unsigned long CLOCK_STANDARD_START_MICROS_        = 3 * 60 * MICROS_IN_SEC; // the time put on the clock when it's reset
const unsigned long REMOTE_BUTTON_MODE_2_HOLD_DURATION_ = 2 * MICROS_IN_SEC;      // the time a remote button must be held down to activate its second mode
const unsigned long DISPLAY_MODE_CHANGE_TEXT_LENGTH_    = 1 * MICROS_IN_SEC;      // the duration to display the name of the new mode

// Lockout & Depress Times
// the lockout time between hits for foil is 300ms +/-25ms
// the minimum amount of time the tip needs to be depressed for foil 14ms +/-1ms
// the lockout time between hits for epee is 45ms +/-5ms
// the minimum amount of time the tip needs to be depressed for epee 2ms
// the lockout time between hits for sabre is 170ms +/-10ms
// the minimum amount of time the tip needs to be depressed (in contact) for sabre 0.1ms <-> 1ms
// These values are stored as micro seconds for more accuracy, and
// we use the minimums so that we have the most edge on any processing lag
const unsigned long SABER_LOCKOUT_MICROS_  = 160000;
const unsigned long FOIL_LOCKOUT_MICROS_   = 275000;
const unsigned long EPEE_LOCKOUT_MICROS_   = 40000;
const unsigned long SABER_CONTACT_MICROS_  = 100;
const unsigned long FOIL_CONTACT_MICROS_   = 13000;
const unsigned long EPEE_CONTACT_MICROS_   = 2000;

// Analog read constants (may need tuning)
const unsigned long ANALOG_READ_ON_TARGET_THRESHOLD_LOW_            = 450;//400;
const unsigned long ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_           = 600;//600;
const unsigned long ANALOG_READ_ON_TARGET_THRESHOLD_LOW_SABER_      = 300;//400;
const unsigned long ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_SABER_     = 450;//600;
const unsigned long ANALOG_READ_OFF_TARGET_B_THRESHOLD_HIGH_        = 100;//100;
const unsigned long ANALOG_READ_OFF_TARGET_A_THRESHOLD_LOW_         = 900;//900;

// Debugging constants
const unsigned long CYCLES_PER_TIMING_EVENT_ = 1000; 

//=============================
// Data Members and Attributes
//=============================

// A/V components
Fencing_Point_Displays*  scoreboard_;
Fencing_Clock*           clock_;
Buzzer*                  buzzer_;
Fencing_Light_Displays*  lights_;

// button states
bool            remote_button_a_pressed_              = false;
bool            remote_button_b_pressed_              = false;
bool            remote_button_c_pressed_              = false;
bool            remote_button_d_pressed_              = false;
unsigned long   remote_button_a_time_of_depression_   = 0;
unsigned long   remote_button_b_time_of_depression_   = 0;
unsigned long   remote_button_c_time_of_depression_   = 0;
unsigned long   remote_button_d_time_of_depression_   = 0;
uint8_t         remote_button_a_last_tick_reacted_to_ = 0;
uint8_t         remote_button_b_last_tick_reacted_to_ = 0;
uint8_t         remote_button_c_last_tick_reacted_to_ = 0;
uint8_t         remote_button_d_last_tick_reacted_to_ = 0;
bool            mode_switch_button_pressed_           = false;
bool            clock_time_increment_button_pressed_  = false;
bool            clock_time_decrement_button_pressed_  = false;
bool            quiet_mode_button_pressed_            = false;

// main hit interpretation mode and setting
mode current_mode_ = mode::SABER;

// quiet mode and setting
bool quiet_mode_enabled_ = false;

// clock change timing
unsigned long clock_adjustment_button_time_of_depression_   = 0;
int8_t        last_clock_adjustment_tick_reacted_to_        = -1; 
uint8_t       most_recent_time_adjustment_level_            = 0;
bool          did_time_reset_                               = false; 

// values of weapon socket analog reads
unsigned long left_fencer_weapon_prong_  = 0;
unsigned long right_fencer_weapon_prong_ = 0;
unsigned long left_fencer_lame_prong_    = 0;
unsigned long right_fencer_lame_prong_   = 0;

// hit interpretation variables
unsigned long left_fencer_contact_start_time_         = 0;
unsigned long right_fencer_contact_start_time_        = 0;
bool          locked_out_                             = false;
unsigned long left_fencer_time_of_registered_hit_     = 0;
unsigned long right_fencer_time_of_registered_hit_    = 0;
bool          contact_reset_after_hit_signaled_       = true; //TODO switch on mode switch?
unsigned long time_of_lockout_                        = 0;
bool          left_fencer_contact_made_               = false;
bool          right_fencer_contact_made_              = false;
bool          left_fencer_hit_on_target_              = false;
bool          left_fencer_hit_off_target_             = false;
bool          right_fencer_hit_on_target_             = false;
bool          right_fencer_hit_off_target_            = false;

// Debugging variables
unsigned long timing_event_start_micros_              = 0;
unsigned long cycles_passed_                          = 0; 
unsigned long last_cycle_start_                       = 0; 
unsigned long worst_cycle_time_                       = 0;   
unsigned long second_worst_cycle_time_                = 0;   
unsigned long third_worst_cycle_time_                 = 0;   
           


//================
// Configuration
//================
void setup()
{
  // set all the inputs pins to INPUT_PULLUP to avoid floating pin issues
  pinMode(REMOTE_INPUT_BUTTON_A_PIN_,           INPUT);
  pinMode(REMOTE_INPUT_BUTTON_B_PIN_,           INPUT);
  pinMode(REMOTE_INPUT_BUTTON_C_PIN_,           INPUT);
  pinMode(REMOTE_INPUT_BUTTON_D_PIN_,           INPUT);
  pinMode(MODE_SWITCH_BUTTON_PIN_,              INPUT_PULLUP);
  pinMode(CLOCK_TIME_INCREMENT_BUTTON_PIN_,     INPUT_PULLUP);
  pinMode(CLOCK_TIME_DECREMENT_BUTTON_PIN_,     INPUT_PULLUP);
  pinMode(QUIET_MODE_BUTTON_PIN_,               INPUT_PULLUP);
  pinMode(LEFT_FENCER_WEAPON_PIN_,              INPUT);
  pinMode(LEFT_FENCER_LAME_PIN_,                INPUT);
  pinMode(RIGHT_FENCER_WEAPON_PIN_,             INPUT);
  pinMode(RIGHT_FENCER_LAME_PIN_,               INPUT);

  // set all the output pins to be outputs
  pinMode(BUZZER_CONTROL_PIN_,                  OUTPUT);
  pinMode(LEFT_FENCER_SCORE_DISPLAY_CLK_PIN_,   OUTPUT);
  pinMode(LEFT_FENCER_SCORE_DISPLAY_DATA_PIN_,  OUTPUT);
  pinMode(RIGHT_FENCER_SCORE_DISPLAY_CLK_PIN_,  OUTPUT);
  pinMode(RIGHT_FENCER_SCORE_DISPLAY_DATA_PIN_, OUTPUT);
  pinMode(TIME_DISPLAY_CLK_PIN_,                OUTPUT);
  pinMode(TIME_DISPLAY_DATA_PIN_ ,              OUTPUT);
  pinMode(LEFT_FENCER_RING_LIGHT_CONTROL_PIN_,  OUTPUT);
  pinMode(RIGHT_FENCER_RING_LIGHT_CONTROL_PIN_, OUTPUT);

  scoreboard_ = new Fencing_Point_Displays( LEFT_FENCER_SCORE_DISPLAY_CLK_PIN_,
                                            LEFT_FENCER_SCORE_DISPLAY_DATA_PIN_,
                                            RIGHT_FENCER_SCORE_DISPLAY_CLK_PIN_,
                                            RIGHT_FENCER_SCORE_DISPLAY_DATA_PIN_
                                          );
  clock_      = new Fencing_Clock(TIME_DISPLAY_CLK_PIN_, TIME_DISPLAY_DATA_PIN_);
  buzzer_     = new Buzzer(BUZZER_CONTROL_PIN_);
  lights_     = new Fencing_Light_Displays(LEFT_FENCER_RING_LIGHT_CONTROL_PIN_, RIGHT_FENCER_RING_LIGHT_CONTROL_PIN_);

  if (DEBUG > 0)
  {
    // start serial
    Serial.begin(BAUDRATE);
  }

  // why not just be sure?
  reset_values();
  lights_->reset_lights();
}


//============
// Main Loop
//============
void loop()
{
  // use a while as a main loop because its 3-4% faster than loop() itself, apparently
  while (true) // run forever
  {
    // get elapsed time
    unsigned long current_time = micros(); 
 
    // update all major components on time elapsed
    scoreboard_ ->tick(current_time);   // Timing NB: this line is now like 0.03 milliseconds per average cycle (without timer or lights on)
    clock_      ->tick(current_time);   // Timing NB: this line is now like 0.60 milliseconds per average cycle (without timer or lights on)
    buzzer_     ->tick(current_time);   // Timing NB: this line doesn't do anything; makes sense as it's a no-op 
    lights_     ->tick(current_time);   // Timing NB: this line doesn't do anything; makes sense as it's a no-op 

    // check user inputs and act on them
    handle_remote_input(current_time);
    handle_clock_adjustment_buttons(current_time);  // Timing NB: all the button methods so far are a total of 0.06 ms per cycle. Not huge! 
    handle_mode_switch_button();
    //handle_quiet_mode_button();

    // get equipment inputs // TODO TODO Timing NB: these take like 450 microseconds all together!! digitalReads are way faster, can I do that instead??
    left_fencer_weapon_prong_  = analogRead(LEFT_FENCER_WEAPON_PIN_);
    right_fencer_weapon_prong_ = analogRead(RIGHT_FENCER_WEAPON_PIN_);
    left_fencer_lame_prong_    = analogRead(LEFT_FENCER_LAME_PIN_);
    right_fencer_lame_prong_   = analogRead(RIGHT_FENCER_LAME_PIN_);

    // weapons bugtesting 
    if (DEBUG == 1)
    {
      Serial.print("left_weap:");
      Serial.print(left_fencer_weapon_prong_);
      Serial.print(",");
      Serial.print("left_lame:");
      Serial.print(left_fencer_lame_prong_);
      Serial.print(",");
      Serial.print("right_weap:");
      Serial.print(right_fencer_weapon_prong_);
      Serial.print(",");
      Serial.print("right_lame:");
      Serial.println(right_fencer_lame_prong_);
    }

    // interpret equipment inputs (based on the current mode)// basically free, timewise [before hit registered)
    process_hits(current_time);

    // react to equipment inputs as necessary // basically free, timewise [before hit registered)
    signal_hits(current_time);


    // main loop timing investigation / bugtesting 
    if (DEBUG == 2)
    { 
      // do the average cycle counts 
      if (cycles_passed_ == 0)
      {
        timing_event_start_micros_ = current_time;
        Serial.println("Timing...");
      }
      else if (cycles_passed_ == CYCLES_PER_TIMING_EVENT_)
      {
        float millis_per_cycle    = ((float)current_time - (float)timing_event_start_micros_) / ((float)CYCLES_PER_TIMING_EVENT_ * 1000); // microsecs per millisec, being lazy 
        Serial.print("Milliseconds Per Cycle Average For Last ");
        Serial.print(CYCLES_PER_TIMING_EVENT_);
        Serial.print(" Cycles: ");
        Serial.print(millis_per_cycle);
        Serial.print("\tTotal Microseconds: ");
        Serial.print((current_time - timing_event_start_micros_));
        Serial.print("\tWorst Cycle: ");
        Serial.print(worst_cycle_time_);
        Serial.print("\tSecond Worst Cycle: ");
        Serial.print(second_worst_cycle_time_);
        Serial.print("\tThird Worst Cycle: ");
        Serial.print(third_worst_cycle_time_);
        Serial.println("");
   
        // zero the tracking variables 
        cycles_passed_                = 0; 
        timing_event_start_micros_    = micros(); // to try to eliminate the effects of these printouts 
        worst_cycle_time_             = 0; 
        second_worst_cycle_time_      = 0;
        third_worst_cycle_time_       = 0;
      }     

      // keep track of the worst cycle you've seen 
      unsigned long last_cycle_time = (current_time - last_cycle_start_);
      if (last_cycle_time > third_worst_cycle_time_) // it was worse than the third
      {
        if (last_cycle_time > second_worst_cycle_time_) // it was worse than the second and third 
        {
          if (last_cycle_time > worst_cycle_time_) // it was worse than the previous worst!
          {
            worst_cycle_time_ = last_cycle_time; 
          }
          else
          {
            second_worst_cycle_time_ = last_cycle_time; 
          }
        }
        else // it was worse than the third, but not the second or the first 
        {
          third_worst_cycle_time_ = last_cycle_time; 
        }
      }
      last_cycle_start_ = current_time; 

      // keep track of your overall progress 
      cycles_passed_++; 
    }
  }
}


//=================================================================================================================
// process_hits - determines hit, lockout, and timeout statuses based on current equipment inputs and current mode
//    parameter:  current_time - the time in microseconds passed since the last processing
//    output:   none
//================================================================================================================
void process_hits(unsigned long current_time)
{
  // first, check for hits!

  // if the left fencer already has a hit, no need to confirm it again
  if ((!locked_out_) && !(left_fencer_hit_on_target_ || left_fencer_hit_off_target_) )
  {
    // if the left fencer's registering a hit, then add that time to their tally (or start the tally if they weren't already hitting)
    if ( (current_mode_ != mode::EPEE && is_reading_on_target( left_fencer_weapon_prong_, right_fencer_lame_prong_))   ||
         (current_mode_ == mode::EPEE && is_reading_on_target( left_fencer_weapon_prong_, left_fencer_lame_prong_ ))   ||
         (current_mode_ == mode::FOIL && is_reading_off_target(left_fencer_weapon_prong_, right_fencer_lame_prong_))
       )
    {
      if (!left_fencer_contact_made_)
      {
        // note the contact 
        left_fencer_contact_made_ = true;

        // record when the contact started 
        left_fencer_contact_start_time_ = current_time;
      }
    }
    else
    {
      // if there's no contact, then reset the counters
      left_fencer_contact_made_       = false;
      left_fencer_contact_start_time_ = 0;
    }

    // if the left fencer is in contact and has exceeded the necessary contact time, mark a hit
    // TODO NB: there's a weird situation where foil can start on-target and slide to off-target and the on-target depressed time counts. Is that right?
    if ( left_fencer_contact_made_ &&
         (
          ((current_mode_ == mode::SABER) && ((unsigned long)(current_time - left_fencer_contact_start_time_) > SABER_CONTACT_MICROS_) ) ||
          ((current_mode_ == mode::FOIL)  && ((unsigned long)(current_time - left_fencer_contact_start_time_) > FOIL_CONTACT_MICROS_ ) ) ||
          ((current_mode_ == mode::EPEE)  && ((unsigned long)(current_time - left_fencer_contact_start_time_) > EPEE_CONTACT_MICROS_ ) )
         )
       )
    {
      // if you're foil, you gotta check if you're off target
      if ((current_mode_ == mode::FOIL) && is_reading_off_target(left_fencer_weapon_prong_, right_fencer_lame_prong_))
      {
        left_fencer_hit_off_target_          = true;
        left_fencer_time_of_registered_hit_  = current_time;
      }
      // every other weapon can only get here by being on-target, as off-targets don't exist TODO TODO still check on target as an error checking measure???
      else
      {
        left_fencer_hit_on_target_           = true;
        left_fencer_time_of_registered_hit_  = current_time;
      }
    }
  }

  // if the right fencer already has a hit, no need to confirm it again
  if ((!locked_out_) && !(right_fencer_hit_on_target_ || right_fencer_hit_off_target_) )
  {
    // if the right fencer's registering a hit, then add that time to their tally (or start the tally if they weren't already hitting)
    if ( (current_mode_ != mode::EPEE && is_reading_on_target( right_fencer_weapon_prong_, left_fencer_lame_prong_  ))  ||
         (current_mode_ == mode::EPEE && is_reading_on_target( right_fencer_weapon_prong_, right_fencer_lame_prong_ ))  ||
         (current_mode_ == mode::FOIL && is_reading_off_target(right_fencer_weapon_prong_, left_fencer_lame_prong_  ))
       )
    {
      if (!right_fencer_contact_made_)
      {
        // note the contact 
        right_fencer_contact_made_ = true;

        // record when the contact started 
        right_fencer_contact_start_time_ = current_time;
      }
    }
    else
    {
      // if there's no contact, then reset the counters
      right_fencer_contact_made_       = false;
      right_fencer_contact_start_time_ = 0;
    }

    // if the right fencer is in contact and has exceeded the necessary contact time, mark a hit
    // TODO NB: there's a weird situation where foil can start on-target and slide to off-target and the on-target depressed time counts. Is that right?
    if ( right_fencer_contact_made_ &&
          (
           ((current_mode_ == mode::SABER) && ((unsigned long)(current_time - right_fencer_contact_start_time_) > SABER_CONTACT_MICROS_) ) ||
           ((current_mode_ == mode::FOIL)  && ((unsigned long)(current_time - right_fencer_contact_start_time_) > FOIL_CONTACT_MICROS_ ) ) ||
           ((current_mode_ == mode::EPEE)  && ((unsigned long)(current_time - right_fencer_contact_start_time_) > EPEE_CONTACT_MICROS_ ) )
          )
       )
    {
      // if you're foil, you gotta check if you're off target 
      if ((current_mode_ == mode::FOIL) && is_reading_off_target(right_fencer_weapon_prong_, left_fencer_lame_prong_))
      {
        right_fencer_hit_off_target_         = true;
        right_fencer_time_of_registered_hit_ = current_time;
      }
      // every other weapon can only get here by being on-target, as off-targets don't exist TODO TODO still check on target as an error checking measure???
      else
      {
        right_fencer_hit_on_target_          = true;
        right_fencer_time_of_registered_hit_ = current_time;
      }
    }
  }


  // now, check for lockouts!

  // if we're already locked out, no need to check for lockout again
  if (!locked_out_)
  {
    // if the left fencer has a valid hit and has had enough time pass since they confirmed it (according to their weapon), lock out
    if ( ( left_fencer_hit_on_target_ || left_fencer_hit_off_target_ ) &&
        (
          ((current_mode_ == mode::SABER) && ((unsigned long)(current_time - left_fencer_time_of_registered_hit_) > SABER_LOCKOUT_MICROS_) ) ||
          ((current_mode_ == mode::FOIL)  && ((unsigned long)(current_time - left_fencer_time_of_registered_hit_) > FOIL_LOCKOUT_MICROS_)  ) ||
          ((current_mode_ == mode::EPEE)  && ((unsigned long)(current_time - left_fencer_time_of_registered_hit_) > EPEE_LOCKOUT_MICROS_)  )
        )
       )
    {
      locked_out_      = true;
      time_of_lockout_ = current_time; 
    }

    // if the right fencer has a valid hit and has had enough time pass since they confirmed it (according to their weapon), lock out
    if ( ( right_fencer_hit_on_target_ || right_fencer_hit_off_target_ ) &&
        (
          ((current_mode_ == mode::SABER) && ((unsigned long)(current_time - right_fencer_time_of_registered_hit_) > SABER_LOCKOUT_MICROS_) ) ||
          ((current_mode_ == mode::FOIL)  && ((unsigned long)(current_time - right_fencer_time_of_registered_hit_) > FOIL_LOCKOUT_MICROS_)  ) ||
          ((current_mode_ == mode::EPEE)  && ((unsigned long)(current_time - right_fencer_time_of_registered_hit_) > EPEE_LOCKOUT_MICROS_)  )
        )
       )
    {
      locked_out_      = true;
      time_of_lockout_ = current_time; 
    }
  }
}


//==============================================================================================================================
// is_reading_off_target - determines whether the provided analog values constitute an off-target contact
//    parameter:  unsigned long fencer_A_weapon_prong - the analog read value of one fencer's weapon line
//    parameter:  unsigned long fencer_B_weapon_prong - the analog read value of the other fencer's one fencer's weapon line
//    output:   bool, true if off-target contact is made, false otherwise
//==============================================================================================================================
bool is_reading_off_target(unsigned long fencer_A_weapon_prong, unsigned long fencer_B_lame_prong)
{
  bool result = false;

  if ((ANALOG_READ_OFF_TARGET_A_THRESHOLD_LOW_ < fencer_A_weapon_prong) && (fencer_B_lame_prong < ANALOG_READ_OFF_TARGET_B_THRESHOLD_HIGH_))
  {
    result = true;
  }

  return result;
}


//==============================================================================================================================
// is_reading_on_target - determines whether the provided analog values constitute an on-target contact
//    parameter:  fencer_A_weapon_prong - the analog read value of one fencer's weapon line
//    parameter:  fencer_B_weapon_prong - the analog read value of the other fencer's one fencer's weapon line
//    output:   true if on-target contact is made, false otherwise
//==============================================================================================================================
bool is_reading_on_target(unsigned long fencer_A_weapon_prong, unsigned long fencer_B_lame_prong)
{
  bool result = false;

  if (current_mode_ == mode::SABER)
  {
    if ( ( ( ANALOG_READ_ON_TARGET_THRESHOLD_LOW_SABER_ < fencer_A_weapon_prong ) && ( fencer_A_weapon_prong < ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_SABER_) ) &&
         ( ( ANALOG_READ_ON_TARGET_THRESHOLD_LOW_SABER_ < fencer_B_lame_prong )   && ( fencer_B_lame_prong   < ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_SABER_) )
       )
    {
      result = true;
    }
  }
  else 
  {
    if ( ( ( ANALOG_READ_ON_TARGET_THRESHOLD_LOW_ < fencer_A_weapon_prong ) && ( fencer_A_weapon_prong < ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_) ) &&
         ( ( ANALOG_READ_ON_TARGET_THRESHOLD_LOW_ < fencer_B_lame_prong )   && ( fencer_B_lame_prong   < ANALOG_READ_ON_TARGET_THRESHOLD_HIGH_) )
       )
    {
      result = true;
    }
  }


  return result;
}


//============================================================================================
// signal_hits - sets A/V outputs and controls resetting between points. Will not reset until
//         a zero-contact reading is made (no on OR off-target sensed contact)
//    parameter:  current_time - the time in microseconds passed since the last processing
//    output:   none
//============================================================================================
void signal_hits(unsigned long current_time)
{
  // if there's not at least one contact-less reading, refuse to signal another hit (prevents continued shrieking on no change)
  if (left_fencer_contact_made_ == false && right_fencer_contact_made_ == false)
  {
    contact_reset_after_hit_signaled_ = true;
  }

  // TODO TODO encapsulate this in an "if (contact_reset_after_hit_signaled_)" too, to avoid the lights change???
  // if a fencer's gotten a hit, light up that light (hits can't get awarded if locked_out, so no need to check)
  if (left_fencer_hit_on_target_)   lights_->display_left_on_target();
  if (left_fencer_hit_off_target_)  lights_->display_left_off_target();
  if (right_fencer_hit_on_target_)  lights_->display_right_on_target();
  if (right_fencer_hit_off_target_) lights_->display_right_off_target();

  // if nothing new can happen, we're good to start signalling!
  if (locked_out_)
  {
    // only sound the buzzer if this isn't a constant-hitting situation
    if (contact_reset_after_hit_signaled_)
    {

      // stop the clock
      clock_->stop();
      
      // sound the buzzer to START signaling a hit
      buzzer_->start_shrieking();

      // if buzzer delay has been achieved, quiet the buzzer
      if ((unsigned long) (current_time - time_of_lockout_) > BUZZER_DURATION_MICROS)
      {
        // stop the buzzer
        buzzer_->stop_shrieking();
      }
    }

    // if light delay has been achieved, reset the lights
    if ((unsigned long) (current_time - time_of_lockout_) > LIGHT_DURATION_MICROS)
    {
      // reset the lights
      lights_->reset_lights();
    }

    // if both buzzer and light delays have been achieved...
    if ( ((unsigned long) (current_time - time_of_lockout_) > LIGHT_DURATION_MICROS ) && 
         ((unsigned long) (current_time - time_of_lockout_) > BUZZER_DURATION_MICROS) 
       )
    {
      // reset from this hit
      reset_values();

      // commit to not buzzing another hit until we get at least one reading where no fencer is making contact
      contact_reset_after_hit_signaled_ = false;
    }
  }
}

//===============================================
// reset_values - prepares system for next point
//    output:   none
//===============================================
void reset_values()
{
  locked_out_                           = false;

  right_fencer_hit_on_target_           = false;
  right_fencer_hit_off_target_          = false;
  left_fencer_hit_on_target_            = false;
  left_fencer_hit_off_target_           = false;

  left_fencer_time_of_registered_hit_   = 0;
  right_fencer_time_of_registered_hit_  = 0;

  time_of_lockout_                      = 0;
}


//========================================================================================================
// handle_clock_adjustment_buttons - implements clock change buttons and "hold longer go faster" feature
//    parameter:  current_time - the time in microseconds passed since the last processing
//    output:   none
//========================================================================================================
void handle_clock_adjustment_buttons(unsigned long current_time)
{
  bool clock_time_increment_button_just_pressed = false;
  bool clock_time_decrement_button_just_pressed = false;

  //
  // read in the button states and set the corresponding button statuses
  //

  // get and set the status(es) of the clock_time_increment_button
  if (digitalRead(CLOCK_TIME_INCREMENT_BUTTON_PIN_) == LOW)
  {
    if (clock_time_increment_button_pressed_ == false)
    {
      clock_time_increment_button_just_pressed = true;
    }
    else
    {
      clock_time_increment_button_just_pressed = false;
    }
    clock_time_increment_button_pressed_ = true;
  }
  else
  {
    clock_time_increment_button_pressed_ = false;
  }

  // get and set the status(es) of the clock_time_decrement_button
  if (digitalRead(CLOCK_TIME_DECREMENT_BUTTON_PIN_) == LOW)
  {
    if (clock_time_decrement_button_pressed_ == false)
    {
      clock_time_decrement_button_just_pressed = true;
    }
    else
    {
      clock_time_decrement_button_just_pressed = false;
    }
    clock_time_decrement_button_pressed_ = true;
  }
  else
  {
    clock_time_decrement_button_pressed_ = false;
  }

  //
  // fire the button actions correspondingly
  //

  // If either adjustment was just pressed, reset the relevant counting variables 
  if (clock_time_increment_button_just_pressed || clock_time_decrement_button_just_pressed)
  {
    clock_adjustment_button_time_of_depression_   =  current_time;
    last_clock_adjustment_tick_reacted_to_        = -1;  
    most_recent_time_adjustment_level_            =  0; 

    // might as well put this here, get a nice two-for-one
    buzzer_->chirp();
  }

  // if both the buttons are pressed at the same time, use that for a reset (then skip until they're both unpressed) 
  if (clock_time_increment_button_pressed_ && clock_time_decrement_button_pressed_)
  {
    // reset the clock to standard
    clock_->set_time(CLOCK_STANDARD_START_MICROS_);
    did_time_reset_ = true; 
  }
  // if either's (xor, effectively) pressed, figure out how much time to add/subtract and then add/subtract it
  else if ((clock_time_increment_button_pressed_ || clock_time_decrement_button_pressed_) && !did_time_reset_)
  {
    // lazy implicit math.floor() by using integer (technically long) math
    int8_t current_tick = (unsigned long) (current_time - clock_adjustment_button_time_of_depression_) / CLOCK_ADJUSTMENT_RATE_TICK_MICROS;

    // only fire an time adjustment once per tick
    if (current_tick > last_clock_adjustment_tick_reacted_to_)
    {
      // save the last tick number so we know not to react to it again [ticks are instances of reaction, not every instance of time passage]
      last_clock_adjustment_tick_reacted_to_ = current_tick;

      // figure out what size the increment/decrement of our clock time's gonna be
      uint8_t level = (unsigned long) (current_time - clock_adjustment_button_time_of_depression_) / CLOCK_ADJUSTMENT_RATE_CHANGE_MICROS;
      uint8_t max_clock_adjustment_level_index = sizeof(CLOCK_ADJUSTMENT_LEVEL_MICROS_) / sizeof(CLOCK_ADJUSTMENT_LEVEL_MICROS_[0]);
      if (level > max_clock_adjustment_level_index) // don't let it max out
      {
        level =  max_clock_adjustment_level_index;
      }

      // if it's a new level, chirp to let the people know 
      if (level > most_recent_time_adjustment_level_)
      {
        most_recent_time_adjustment_level_ = level; 
        buzzer_->chirp();
      }

      // start by taking the current time on the clock and rounding it (well, math.floor()ing it by abusing int math) to the nearest even "level" multiple
      unsigned long new_time = (clock_->get_remaining_micros() / CLOCK_ADJUSTMENT_LEVEL_MICROS_[level]) * CLOCK_ADJUSTMENT_LEVEL_MICROS_[level];

      // if we're incrementing, add; if we're decrementing, subtract. Obviously. God.
      if (clock_time_increment_button_pressed_)
      {
        new_time += CLOCK_ADJUSTMENT_LEVEL_MICROS_[level];
      }
      else // meaning clock_time_decrement_button_pressed_ was true
      {
        // decrement, but if that'd run it past zero just zero it instead
        if (CLOCK_ADJUSTMENT_LEVEL_MICROS_[level] > new_time)
        {
          new_time = 0;
        }
        else
        {
          new_time -= CLOCK_ADJUSTMENT_LEVEL_MICROS_[level];
        }
      }

      // actually send the adjustment to the clock
      clock_->set_time(new_time);
    }
  }
  // neither was pressed 
  else if (!clock_time_increment_button_pressed_ && !clock_time_decrement_button_pressed_) 
  {
    // stop skipping over everything 
    did_time_reset_ = false; 
  } 
}

//==============================================================================================================================
//  handle_remote_input - checks for signal from the small remote and fires actions accordingly.
//              A button's first mode is fired when the button is *released* without the second mode being activated
//              A button's second mode is fired every REMOTE_BUTTON_MODE_2_HOLD_DURATION_ the button is held down for
//                Button A:
//                Mode One: Increase left fencer score by one
//                Mode Two: Decrease left fencer score by one
//                Button B:
//                Mode One: Increase right fencer score by one
//                Mode Two: Decrease right fencer score by one
//                Button C:
//                Mode One: Start / stop the fencing clock
//                Mode Two: Reset the clock to CLOCK_STANDARD_START_MICROS_
//                Button D:
//                Mode One: Toggle the quiet mode
//                Mode Two: Reset the score to 0-0
//    parameter:  current_time - the time in microseconds passed since the last processing
//    output:   none
//===============================================================================================================================
void handle_remote_input(unsigned long current_time)
{
  // for a future debug level TODO 
  //Serial.print("current_time:"); Serial.print(digitalRead(current_time)); Serial.print(", ");
  //Serial.print("REMOTE_INPUT_BUTTON_A_PIN_:"); Serial.print(digitalRead(REMOTE_INPUT_BUTTON_A_PIN_)); Serial.print(", ");
  //Serial.print("REMOTE_INPUT_BUTTON_B_PIN_:"); Serial.print(digitalRead(REMOTE_INPUT_BUTTON_B_PIN_)); Serial.print(", ");
  //Serial.print("REMOTE_INPUT_BUTTON_C_PIN_:"); Serial.print(digitalRead(REMOTE_INPUT_BUTTON_C_PIN_)); Serial.print(", ");
  //Serial.print("REMOTE_INPUT_BUTTON_D_PIN_:"); Serial.print(digitalRead(REMOTE_INPUT_BUTTON_D_PIN_)); Serial.print(", ");
  //Serial.println();

  uint8_t current_tick = 0; 
  
  // handle remote button a
  
  if (digitalRead(REMOTE_INPUT_BUTTON_A_PIN_) == HIGH) // if the button is reading as pressed
  {
    if (remote_button_a_pressed_ == false) // meaning it was JUST pressed 
    {
      remote_button_a_time_of_depression_ = current_time; 
    }
    remote_button_a_pressed_ = true;

    // if the pressed time has completed a new hold duration, fire the button's second mode
    current_tick = (unsigned long)(current_time - remote_button_a_time_of_depression_) / REMOTE_BUTTON_MODE_2_HOLD_DURATION_; 
    if (remote_button_a_last_tick_reacted_to_ < current_tick) // cheeky math.floor thanks to int math
    {
      // save the new tick value so you don't react to it twice
      remote_button_a_last_tick_reacted_to_ = current_tick;
      // fire the button's second mode
      buzzer_     ->chirp();
      scoreboard_ ->decrement_left_fencer_score();  // current decided alt action: left fencer -1
    } 
  }
  else
  {
    if (remote_button_a_pressed_ && remote_button_a_last_tick_reacted_to_ == 0) // if the button was just released and the second mode wasn't activated
    {
      // fire the button's first mode
      buzzer_     ->chirp();
      scoreboard_ ->increment_left_fencer_score(); // current decided action: left fencer +1
    }

    // reset the button variables
    remote_button_a_pressed_              = false;
    remote_button_a_time_of_depression_   = 0;
    remote_button_a_last_tick_reacted_to_ = 0;
  }


  // handle remote button b
  
  if (digitalRead(REMOTE_INPUT_BUTTON_B_PIN_) == HIGH) // if the button is reading as pressed
  {
    if (remote_button_b_pressed_ == false) // meaning it was JUST pressed 
    {
      remote_button_b_time_of_depression_ = current_time; 
    }
    remote_button_b_pressed_ = true;

    // if the pressed time has completed a new hold duration, fire the button's second mode
    current_tick = (unsigned long)(current_time - remote_button_b_time_of_depression_) / REMOTE_BUTTON_MODE_2_HOLD_DURATION_; 
    if (remote_button_b_last_tick_reacted_to_ < current_tick) // cheeky math.floor thanks to int math
    {
      // save the new tick value so you don't react to it twice
      remote_button_b_last_tick_reacted_to_ = current_tick;
      // fire the button's second mode
      buzzer_     ->chirp();
      scoreboard_ ->decrement_right_fencer_score(); // current decided alt action: right fencer -1
    } 
  }
  else
  {
    if (remote_button_b_pressed_ && remote_button_b_last_tick_reacted_to_ == 0) // if the button was just released and the second mode wasn't activated
    {
      // fire the button's first mode
      buzzer_     ->chirp();
      scoreboard_ ->increment_right_fencer_score(); // current decided action: right fencer +1
    }

    // reset the button variables
    remote_button_b_pressed_              = false;
    remote_button_b_time_of_depression_   = 0;
    remote_button_b_last_tick_reacted_to_ = 0;
  }


  // handle remote button c
  
  if (digitalRead(REMOTE_INPUT_BUTTON_C_PIN_) == HIGH) // if the button is reading as pressed
  {
    if (remote_button_c_pressed_ == false) // meaning it was JUST pressed 
    {
      remote_button_c_time_of_depression_ = current_time; 
    }
    remote_button_c_pressed_ = true;

    // if the pressed time has completed a new hold duration, fire the button's second mode
    current_tick = (unsigned long)(current_time - remote_button_c_time_of_depression_) / REMOTE_BUTTON_MODE_2_HOLD_DURATION_; 
    if (remote_button_c_last_tick_reacted_to_ < current_tick) // cheeky math.floor thanks to int math
    {
      // save the new tick value so you don't react to it twice
      remote_button_c_last_tick_reacted_to_ = current_tick;
      // fire the button's second mode
      buzzer_ ->chirp();
      clock_  ->set_time(CLOCK_STANDARD_START_MICROS_);  // current decided alt action: reset the clock
    } 
  }
  else
  {
    if (remote_button_c_pressed_ && remote_button_c_last_tick_reacted_to_ == 0) // if the button was just released and the second mode wasn't activated
    {
      // fire the button's first mode
      buzzer_     ->chirp();
      clock_      ->toggle();  // current decided action: start/stop the clock
    }

    // reset the button variables
    remote_button_c_pressed_              = false;
    remote_button_c_time_of_depression_   = 0;
    remote_button_c_last_tick_reacted_to_ = 0;
  }


  // handle remote button d
  
  if (digitalRead(REMOTE_INPUT_BUTTON_D_PIN_) == HIGH) // if the button is reading as pressed
  {
    if (remote_button_d_pressed_ == false) // meaning it was JUST pressed 
    {
      remote_button_d_time_of_depression_ = current_time; 
    }
    remote_button_d_pressed_ = true;

    // if the pressed time has completed a new hold duration, fire the button's second mode
    current_tick = (unsigned long)(current_time - remote_button_d_time_of_depression_) / REMOTE_BUTTON_MODE_2_HOLD_DURATION_; 
    if (remote_button_d_last_tick_reacted_to_ < current_tick) // cheeky math.floor thanks to int math
    {
      // save the new tick value so you don't react to it twice
      remote_button_d_last_tick_reacted_to_ = current_tick;
      // fire the button's second mode
      buzzer_     ->chirp();
      scoreboard_ ->set_scores(0, 0); // current decided alt action: reset the scores
    } 
  }
  else
  {
    if (remote_button_d_pressed_ && remote_button_d_last_tick_reacted_to_ == 0) // if the button was just released and the second mode wasn't activated
    {
      // fire the button's first mode
      quiet_mode_enabled_ = !quiet_mode_enabled_;       // current decided action: toggle the quiet mode

      // always chirp so you know if you pressed it 
      buzzer_->set_quiet_mode(false);
      buzzer_->chirp();

      // set the actual mode 
      buzzer_->set_quiet_mode(quiet_mode_enabled_);
    }

    // reset the button variables
    remote_button_d_pressed_              = false;
    remote_button_d_time_of_depression_   = 0;
    remote_button_d_last_tick_reacted_to_ = 0;
  }
}


//======================================================================================
// handle_mode_switch_button - implements mode change button, with wrap-around feature!
//    output:   none
//    TODO: how are we indicating what mode we're currently in??
//======================================================================================
void handle_mode_switch_button()
{
  bool mode_switch_button_just_pressed = false;

  //
  // get and set the status(es) of the mode switch button
  //
  if (digitalRead(MODE_SWITCH_BUTTON_PIN_) == LOW)
  {
    if (mode_switch_button_pressed_ == false)
    {
      mode_switch_button_just_pressed = true;
    }
    else
    {
      mode_switch_button_just_pressed = false;
    }
    mode_switch_button_pressed_ = true;
  }
  else
  {
    mode_switch_button_pressed_ = false;
  }

  //
  // fire the button actions correspondingly
  //
  if (mode_switch_button_just_pressed)
  {
    // chirp to let the user know they pressed it right!
    buzzer_->chirp();

    // increment the mode - unless you're at the end of the mode list, in which case wrap around.
    switch (current_mode_)
    {
      case mode::SABER:
        current_mode_ = mode::FOIL;
        clock_      ->clock_                     ->set_display_contents("FOIL", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ -> left_fencer_score_display_->set_display_contents("FOIL", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ ->right_fencer_score_display_->set_display_contents("FOIL", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );

        // commit to not buzzing another hit until we get at least one reading where no fencer is making contact
        /////contact_reset_after_hit_signaled_ = false; // TODO TODO commented out rn because it makes bugtesting easier! also check if this is right! (like, make it include lights too?) 
        break;
      case mode::FOIL:
        current_mode_ = mode::EPEE;
        clock_      ->clock_                     ->set_display_contents("EPEE", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ -> left_fencer_score_display_->set_display_contents("EPEE", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ ->right_fencer_score_display_->set_display_contents("EPEE", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        break;
      case mode::EPEE:
        current_mode_ = mode::SABER;
        clock_      ->clock_                     ->set_display_contents("SA", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ -> left_fencer_score_display_->set_display_contents("SA", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        scoreboard_ ->right_fencer_score_display_->set_display_contents("SA", false, true, DISPLAY_MODE_CHANGE_TEXT_LENGTH_ );
        break;
      default:
        current_mode_ = mode::SABER;
    }
  }
}


//=======================================================================================
// handle_quiet_mode_button - implements button to turn box chirps and buzzes on and off
//    output:   none
//    TODO: how are we indicating what quiet mode we're currently in??
//=======================================================================================
void handle_quiet_mode_button()
{
  bool quiet_mode_button_just_pressed_ = false;

  //
  // get and set the status(es) of the quiet mode button
  //
  if (digitalRead(QUIET_MODE_BUTTON_PIN_) == HIGH)
  {
    if (quiet_mode_button_pressed_ == false)
    {
      quiet_mode_button_just_pressed_ = true;
    }
    else
    {
      quiet_mode_button_just_pressed_ = false;
    }
    quiet_mode_button_pressed_ = true;
  }
  else
  {
    quiet_mode_button_pressed_ = false;
  }

  //
  // fire the button actions correspondingly
  //
  if (quiet_mode_button_just_pressed_)
  {
    // toggle the boolean data member
    quiet_mode_enabled_ = !quiet_mode_enabled_;

    // tell the buzzer the new setting
    buzzer_->set_quiet_mode(quiet_mode_enabled_);

    // chirp to let the user know they pressed it right! (if you're chirping!)
    buzzer_->chirp();
  }
}
