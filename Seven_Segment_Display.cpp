//============================================================================//
//  Desc    : C++ Implementation for a four-character, seven-segment display  //
//  Dev     : Nate Cope,                                                      //
//  Version : 1.1                                                             //
//  Date    : Dec 2022                                                        //
//  Notes   : - All display updating should be done via tick(0)               //
//            to centralize message processing in one spot                    // 
//============================================================================//

// interface include
#include "Seven_Segment_Display.h"

// TODO TODO TODO anyone storing a micro time value needs to be unsigned long, not int!!!!
// TODO could do a "only update display if there's a change" in the future if display is taking a lot of time or something IT SURE THE HECK IS 
// TODO should nominally check if I can change brightness mid-stream  

// Constructor 
//    uint8_t clock_pin - the Arduino pin attached to the CLK pin of the display 
//    uint8_t data_pin  - the Arduino pin attached to the DATA pin of the display
Seven_Segment_Display::Seven_Segment_Display(uint8_t clock_pin, uint8_t data_pin)
{
  // initialize the individual light display pointers 
  this->controlled_display_ = new TM1637(clock_pin, data_pin);  

  // initialization stuff 
  this->controlled_display_->set(BRIGHTEST);
  this->controlled_display_->init();
}

// Destructor 
Seven_Segment_Display::~Seven_Segment_Display()
{
  // delete the underlying display object we allocated memory for 
  delete this->controlled_display_ ;      
}

// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating the display, and no time checks are done 
void Seven_Segment_Display::tick(unsigned long current_time_micros)
{
  // track the new timestamp unless we're just updating 
  if (current_time_micros != 0)
  {
    this->most_recently_seen_external_time_ = current_time_micros; 
  }

  if (this->override_birth_time_ == 0) // i.e., there's no active high-priority message
  {
    // push the normal message to the display
    this->send_to_display(current_display_contents_); 
  }
  else // i.e., there's an active high-priority message 
  {
    if (current_time_micros == 0) // if this is just an update
    {
      // push the override message to the display
      this->send_to_display(override_display_contents_);  
    }
    // check for override expiration 
    else if ((unsigned long)(current_time_micros - this->override_birth_time_) > this->override_lifespan_micros_) // i.e. the high-priorty message just "died" (expired) 
    {
      // render the lifespan invalid so we don't go through these checks unnecessarily 
      this->override_lifespan_micros_ = 0; 

      // reset the start time, too 
      this->override_birth_time_      = 0; 

      // push the normal message to the display
      this->send_to_display(current_display_contents_); 
    }
    else // i.e., the override message hasn't expired 
    {
      // push the override message to the display
      this->send_to_display(override_display_contents_);  
    }   
  }
}

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
void Seven_Segment_Display::set_display_contents( String        data                    , 
                                                  boolean       clock_points            , // default value false
                                                  boolean       will_override           , // default value false
                                                  unsigned long override_duration_micros, // default value 1,000,000 (one second)
                                                  boolean       looping_enabled         , // default value false                        currently no-op
                                                  boolean       loop_direction_r_to_l   , // default value true                         currently no-op
                                                  unsigned long loop_speed_micros         // default value 1,000,000 (one second        currently no-op
                                                )
{
  uint8_t message_length = data.length() < this->DISPLAY_SIZE_ ? data.length() : this->DISPLAY_SIZE_;

  // TODO TODO TODO NEED check on redundancy here somewhere too I think 

  if (will_override) // if this is a priority message 
  {
    // zero out any old priority message info 
    this->clear_override_display_contents(); 
    
    // update the priority message holder 
    for (uint8_t i = 0; i < message_length; i++)
    {
      // add in the clock points marker if need be 
      if (clock_points)
      {
        this->override_display_contents_[i] = this->get_display_code_for_character(data[i]) + CLOCK_POINTS_DATA_FLAG_;
      }
      else
      {
        this->override_display_contents_[i] = this->get_display_code_for_character(data[i]);
      }
    }

    // set the lifespan
    this->override_lifespan_micros_ = override_duration_micros; 

    // zero the age (it just got born!) 
    this->override_birth_time_      = this->most_recently_seen_external_time_;
   
  }
  else // just a normal message 
  { 
    // zero out any old priority message info 
    this->clear_display_contents(); 
    
    // update the normal message holder 
    for (uint8_t i = 0; i < message_length; i++)
    {
      // add in the clock points marker if need be 
      if (clock_points)
      {
        this->current_display_contents_[i] = this->get_display_code_for_character(data[i]) + CLOCK_POINTS_DATA_FLAG_;
      }
      else
      {
        this->current_display_contents_[i] = this->get_display_code_for_character(data[i]);
      }
    }
  }

  // update everything through the one central update channel 
  // (with no time pasage - we're just updating)
  this->tick(0); 
}

// Sets the brightness of the display
void Seven_Segment_Display::set_brightness(uint8_t level)
{
  // make sure the input doesn't exceed logical limits 
  if      (level > BRIGHTEST     ) level = BRIGHTEST     ; 
  else if (level < BRIGHT_DARKEST) level = BRIGHT_DARKEST; 

  // send that brightess level 
  this->controlled_display_->set(level);

  // update everything through the one central update channel 
  // (with no time pasage - we're just updating)
  // this is so the changes take effect
  this->tick(0); 
}


//
//  private methods 
//

// helper method to zero out display storage
void Seven_Segment_Display::clear_display_contents()
{
  for (uint8_t i = 0; i < sizeof(this->current_display_contents_)/sizeof(uint8_t); i++)
  {
    this->current_display_contents_[i] = 0x00; 
  }
}


// helper method to zero out override display storage
void Seven_Segment_Display::clear_override_display_contents()
{
  for (uint8_t i = 0; i < sizeof(this->override_display_contents_)/sizeof(uint8_t); i++)
  {
    this->override_display_contents_[i] = 0x00; 
  }
}


// helper method - translates characters to their seven-segment display byte equivalent, if one exists  
uint8_t Seven_Segment_Display::get_display_code_for_character(char character)
{
  uint8_t return_val = 0x00; 

  // 
  switch (character)
  {
    case '0':
    case 'o':
    case 'O':
      return_val = 0x3f;
      break;
    case '1':
    case 'i':
    case 'I':
      return_val = 0x06;
      break;
    case '2':
      return_val = 0x5b;
      break;
    case '3':
      return_val = 0x4f;
      break;
    case '4':
      return_val = 0x66;
      break;
    case '5':
    case 'S':
    case 's':
      return_val = 0x6d;
      break;
    case '6':
      return_val = 0x7d;
      break;
    case '7':
      return_val = 0x07;
      break;
    case '8':
    case 'B':
      return_val = 0x7f;
      break;
    case '9':
      return_val = 0x6f;
      break;
    case 'a':
    case 'A':
    case 'R':
      return_val = 0x77;
      break;
    case 'b':
      return_val = 0x7c;
      break;
    case 'c':
    case 'C':
      return_val = 0x39;
      break;
    case 'd':
    case 'D':
      return_val = 0x5e;
      break;
    case 'e':
    case 'E':
      return_val = 0x79;
      break;
    case 'f':
    case 'F':
      return_val = 0x71;
      break;
    case 'l':
    case 'L':
      return_val = 0b0111000;
      break;
    case 'p':
    case 'P':
      return_val = 0b1110011;
      break;
    default:
      return_val = 0x00;
  }

  return return_val; 
}


// helper method, compares two uint8_t arrays for identical contents (TODO only checks up to display size, TODO just use std::vectors...)
bool Seven_Segment_Display::check_uint8_t_arrays_for_same_contents(uint8_t first[], uint8_t second[])
{
  bool return_val = true; 

  for(uint8_t i = 0; i < this->DISPLAY_SIZE_; i++)
  {
    if(first[i] != second[i]) 
    {
      return_val = false; 
      break;
    }
  }

  return return_val;
}


// helper method, simplifies some calls and does redundancy checking 
void Seven_Segment_Display::send_to_display(uint8_t contents_to_display[])
{
  // only bother updating if the contents of the display would be new 
  if( !this->check_uint8_t_arrays_for_same_contents(contents_to_display, this->most_recently_displayed_contents_) )
  {
    // actually send the contents to the display 
    this->controlled_display_->direct_display(contents_to_display);

    // update the most recently displayed data 
    for(uint8_t i = 0; i < this->DISPLAY_SIZE_; i++)
    {
      this->most_recently_displayed_contents_[i] = contents_to_display[i]; 
    }
  }
}
