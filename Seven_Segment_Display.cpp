//============================================================================//
//  Name    : Seven_Segment_Display.cpp                                       //
//  Desc    : C++ Implementation for a four-character, seven-segment display  //
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

// interface include
#include "Seven_Segment_Display.h"

// TODO theoretically changing brightness mid-stream could be a huge issue, 
//      since it could collide with a currently-sending message 
// TODO at some point, a major overhaul for beyond 4 character messages 

// Constructor 
//    uint8_t clock_pin - the Arduino pin attached to the CLK pin of the display 
//    uint8_t data_pin  - the Arduino pin attached to the DATA pin of the display
Seven_Segment_Display::Seven_Segment_Display(uint8_t clock_pin, uint8_t data_pin)
{
  // store the pins for later reference 
  this->clock_pin_ = clock_pin;
  this->data_pin_  = data_pin;

  // set the pin modes 
  pinMode(this->clock_pin_, OUTPUT);
  pinMode(this->data_pin_,  OUTPUT);

  // initialization stuff 
  this->set_brightness(this->BRIGHTEST_);
}

// Destructor 
Seven_Segment_Display::~Seven_Segment_Display()
{
  // no-op currently   
}

// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating the display, and no time checks are done 
void Seven_Segment_Display::tick(unsigned long current_time_micros)
{
  // track the new timestamp and potentially incrementally display a new message unless we're just updating 
  if (current_time_micros != 0)
  {
    this->most_recently_seen_external_time_ = current_time_micros; 

    // change the next display character if there's an active incrementally-sending message 
    this->step_incremental_display(); 
  }

  if (this->override_birth_time_ == 0) // i.e., there's no active high-priority message
  {
    // push the normal message to the display
    this->stage_message_for_sending(normal_display_message_); 
  }
  else // i.e., there's an active high-priority message 
  {
    // check for override expiration 
    if ((unsigned long)(this->most_recently_seen_external_time_ - this->override_birth_time_) > this->override_lifespan_micros_) // i.e. the high-priorty message just "died" (expired) 
    {
      // render the lifespan invalid so we don't go through these checks unnecessarily 
      this->override_lifespan_micros_ = 0; 

      // reset the start time, too 
      this->override_birth_time_      = 0; 

      // push the normal message to the display
      this->stage_message_for_sending(normal_display_message_); 
    }
    else // i.e., the override message hasn't expired 
    {
      // push the override message to the display
      this->stage_message_for_sending(override_display_message_);  
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
                                                  unsigned long loop_speed_micros         // default value 1,000,000 (one second)       currently no-op
                                                )
{   
  uint8_t message_length = data.length() < this->DISPLAY_SIZE_ ? data.length() : this->DISPLAY_SIZE_;  // TODO TODO what happens if data's too short???

  if (will_override) // if this is a priority message 
  {    
    // clear the message to prevent lingering characters if a short message follows a long one  
    this->clear_override_display_message();
    
    // update the priority message holder 
    for (uint8_t i = 0; i < message_length; i++)
    {
      // add in the clock points marker if need be 
      if (clock_points)
      {
        this->override_display_message_[i] = this->get_display_code_for_character(data[i]) + CLOCK_POINTS_DATA_FLAG_;
      }
      else
      {
        this->override_display_message_[i] = this->get_display_code_for_character(data[i]);
      }
    }

    // set the lifespan
    this->override_lifespan_micros_ = override_duration_micros; 

    // zero the age (it just got born!) 
    this->override_birth_time_      = this->most_recently_seen_external_time_;
   
  }
  else // just a normal message 
  {    
    // clear the message to prevent lingering characters if a short message follows a long one  
    this->clear_normal_display_message();
    
    // update the normal message holder 
    for (uint8_t i = 0; i < message_length; i++)
    {
      // add in the clock points marker if need be 
      if (clock_points)
      {
        this->normal_display_message_[i] = this->get_display_code_for_character(data[i]) + CLOCK_POINTS_DATA_FLAG_;
      }
      else
      {
        this->normal_display_message_[i] = this->get_display_code_for_character(data[i]);
      }
    }
  }

  // update everything through the one central update channel 
  // (with no time passage - we're just updating)
  this->tick(0); 
}

// Sets the brightness of the display
// TODO live adjusting test
void Seven_Segment_Display::set_brightness(uint8_t level)
{
  // make sure the input doesn't exceed logical limits 
  if      (level > this->BRIGHTEST_     ) level = this->BRIGHTEST_     ; 
  else if (level < this->BRIGHT_DARKEST_) level = this->BRIGHT_DARKEST_; 

  // calculate the correct brightness code 
  this->brightness_bit_ = this->BRIGHTNESS_BASE_ + level;

  // send that brightess level immediately 
  // TODO will cause problems with current incremental send strategy if 
  //      there's a half-sent message out there already
  this->send_signal_start();
  this->writeByte(this->brightness_bit_);
  this->send_signal_stop();
}


//
//  private methods 
//

// helper method to zero out display storage
void Seven_Segment_Display::clear_normal_display_message()
{
  for (uint8_t i = 0; i < sizeof(this->normal_display_message_)/sizeof(uint8_t); i++)
  {
    this->normal_display_message_[i] = 0x00; 
  }
}


// helper method to zero out override display storage
void Seven_Segment_Display::clear_override_display_message()
{
  for (uint8_t i = 0; i < sizeof(this->override_display_message_)/sizeof(uint8_t); i++)
  {
    this->override_display_message_[i] = 0x00; 
  }
}


// helper method - translates characters to their seven-segment display byte equivalent, if one exists  
// TODO: incomplete by a lot 
uint8_t Seven_Segment_Display::get_display_code_for_character(char character)
{
  uint8_t return_val = 0x00; 

  // retrieve the appropriate seven-segment display numeric code for the alpha-numeric potential character 
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

// helper method; checks for and prepares any new values for sending; ignores the request if redundant
void Seven_Segment_Display::stage_message_for_sending(uint8_t contents_to_display[])
{
  // look for and save any new parts of the potential new message 
  for (uint8_t i = 0; i < sizeof(this->new_message_to_be_displayed_)/sizeof(uint8_t); i++)
  {
    if (this->new_message_to_be_displayed_[i] != contents_to_display[i])
    {
      this->new_message_to_be_displayed_[i] = contents_to_display[i]; 
      
      // only mark that there's a new message to deal with if any differences are actually found
      this->new_message_waiting_for_send_begin_ = true; 
    }
  }
}

// performs next step in sending of pending message; does redundancy checking 
void Seven_Segment_Display::step_incremental_display()
{
  // this part's pretty hardcoded, but I'm TRYING to avoid as many magic numbers as possible 
  const uint8_t STEPS_IN_CHANGING_ONE_VALUE = 8; 

  // figure out where we are in the process 
  uint8_t current_index               = this->incremental_display_step_ / STEPS_IN_CHANGING_ONE_VALUE; 
  uint8_t current_step_for_this_index = this->incremental_display_step_ % STEPS_IN_CHANGING_ONE_VALUE; 

  // make sure new messages can get examined even if the old message is already finished sending; save time if we're already done otherwise  
  if (!(current_index < this->DISPLAY_SIZE_))       // if you're past an end, therefore finished sending a message and just waiting around...
  {
    if (this->new_message_waiting_for_send_begin_)  // if there's a new message waiting to be sent...
    {
      current_step_for_this_index = 0;              // make sure that it gets evaluated 
    }
    else
    {
      return;                                       // just leave to save time 
    }
  }

  // figure out and take the next step of the message updating process 
  switch (current_step_for_this_index)
  {
    case 0:

      // need to reset if there's a new message waiting, but
      // only here, when we're not in the middle of sending
      // a value 
      if (this->new_message_waiting_for_send_begin_)
      {
        current_index                             = 0;  
        this->incremental_display_step_           = 0; 
        this->new_message_waiting_for_send_begin_ = false; 
      }

      /*
      TODO TODO TODO 
      while ( 
              (this->new_message_to_be_displayed_[current_index] == this->current_display_contents_[current_index]) && // while a value's ALREADY being displayed...
              !(current_index < this->DISPLAY_SIZE_)                                                                   // AND we haven't violated the length of the arrays...
            ) 
      {
        // increment both the current index and the display step 
        current_index++; 
        this->incremental_display_step_ += STEPS_IN_CHANGING_ONE_VALUE; 
      }

      */

      
      break;  
    
    // steps to send the preamble command 
    case 1:
      this->send_signal_start(); 
      break;
    case 2:
      this->writeByte(this->ADDR_FIXED_); 
      break;
    case 3:
      this->send_signal_stop();
      break;

    // steps to send one value to the specified index 
    case 4:
      this->send_signal_start(); 
      break;
    case 5:
      this->writeByte(current_index | this->CMD_SET_ADDR_); 
      break;
    case 6:
      this->writeByte(this->new_message_to_be_displayed_[current_index]);
      break;
    case 7:
      this->send_signal_stop();
      // since this value is finished, note that it's been changed 
      this->current_display_contents_[current_index] = this->new_message_to_be_displayed_[current_index]; 
      break;
    

    // catch-all; shouldn't happen 
    default:
      bool we_have_an_uh_oh = true; // TODO actually raise an error or crash or something 
      // should not be able to reach this 
  }
  
  // move on to the next stage in the process 
  this->incremental_display_step_++; 
}


// helper method; writes a single byte to the TM1637 style Seven-Segment Display 
void Seven_Segment_Display::writeByte(int8_t wr_data)
{
  uint8_t i,count1;      

  // send one byte of data
  for(i = 0; i < 8 ;i++)       
  {
    digitalWrite(this->clock_pin_,LOW);      
    if (wr_data & 0x01) digitalWrite(this->data_pin_,HIGH); //LSB first
    else                digitalWrite(this->data_pin_,LOW);
    wr_data >>= 1;      
    digitalWrite(this->clock_pin_,HIGH);
  }  
  
  //wait for the ACK
  digitalWrite(this->clock_pin_,  LOW  ); 
  digitalWrite(this->data_pin_ ,  HIGH );
  digitalWrite(this->clock_pin_,  HIGH ); 


  // NB: this block can be commented out wholesale with seemingly no effect, but it only saves
  // like 16 microseconds off the worse cases, so I'm leaving it in right now to be on the safe
  // side
  pinMode(     this->data_pin_ ,  INPUT);
  while( digitalRead(this->data_pin_) )    
  {
    count1 +=1;
    if(count1 == 200) // TODO what is this janky delay??? 
    {      
      pinMode(this->data_pin_,OUTPUT);
      digitalWrite(this->data_pin_,LOW);
      count1 =0;
    }
    pinMode(this->data_pin_,INPUT);
  }
  pinMode(this->data_pin_,OUTPUT);

 
  // gotta set this pin here to make interlacing commands possible
  //    My theory is that if you don't and you're sharing data pins like
  //    we are, the next time data goes high you accidentally start a 
  //    start command!
  digitalWrite(this->clock_pin_,  LOW);
}


// helper method; sends the "prepare to receive command / data" signal to the TM1637 style Seven-Segment Display
void Seven_Segment_Display::send_signal_start()
{
  // begin sending start signal to SSD
  digitalWrite(this->clock_pin_, HIGH);
  digitalWrite(this->data_pin_ , HIGH); 
  digitalWrite(this->data_pin_ , LOW ); 
  digitalWrite(this->clock_pin_, LOW );    
  // end sending start signal to SSD    
}


// helper method; sends the "finish receiving command / data" signal to the TM1637 style Seven-Segment Display
void Seven_Segment_Display::send_signal_stop()
{
  // begin sending stop signal to SSD
  digitalWrite(this->clock_pin_,  LOW);
  digitalWrite(this->data_pin_ ,  LOW);
  digitalWrite(this->clock_pin_, HIGH);
  digitalWrite(this->data_pin_ , HIGH);   
  // end sending stop signal to SSD   
}


// DEPRECATED; one-shot method to send one value to a given index of the SSD
void Seven_Segment_Display::change_single_value(uint8_t index, uint8_t new_value)
{
  // don't try to set invalid indicies 
  if ( !(index < this->DISPLAY_SIZE_) )
  {
    return; 
  }

  this->send_signal_start();
  this->writeByte(this->ADDR_FIXED_);
  this->send_signal_stop();
  
  this->send_signal_start();       
  this->writeByte(index | this->CMD_SET_ADDR_);
  this->writeByte(new_value);
  this->send_signal_stop();
}


// DEPRECATED; one-shot method to send one four-value set to the SSD
void Seven_Segment_Display::change_whole_message(uint8_t contents_to_display[])
{
  this->send_signal_start();       
  this->writeByte(this->ADDR_AUTO_);
  this->send_signal_stop();
      
  this->send_signal_start();          
  this->writeByte(this->CMD_SET_ADDR_);
  for (uint8_t i = 0; i < this->DISPLAY_SIZE_; i++)
  {
    this->writeByte(contents_to_display[i]); 
  }
  this->send_signal_stop();
}
