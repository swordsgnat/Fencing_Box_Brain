//===========================================================================//
//  Desc:    C++ Implementation for a RBG Ring Fencing Light Set             //
//  Dev:     Nate Cope,                                                      //
//  Date:    Nov 2022                                                        //
//  Notes:                                                                   //
//                                                                           // 
//===========================================================================//

// interface includes
#include "Fencing_Light_Displays.h"

// Constructor 
//    uint8_t left_fencer_light_control_pin   - the Arduino pin attached to the control pin of the left fencer's ring light
//    uint8_t right_fencer_light_control_pin  - the Arduino pin attached to the control pin of the right fencer's ring light
Fencing_Light_Displays::Fencing_Light_Displays(uint8_t left_fencer_light_control_pin, uint8_t right_fencer_light_control_pin)
{
  // initialize the individual light display pointers 
  this->left_fencer_light_  = new Fencing_Light( left_fencer_light_control_pin); 
  this->right_fencer_light_ = new Fencing_Light(right_fencer_light_control_pin);       
}

// Destructor 
Fencing_Light_Displays::~Fencing_Light_Displays()
{
  // delete the underlying Fencing_Lights we allocated memory for 
  delete this->left_fencer_light_ ; 
  delete this->right_fencer_light_;       
}

    

// Lets the object know how much time has passed-> For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it how much time has passed, and update that way-> 
void Fencing_Light_Displays::tick(int elapsed_micros)
{
  // currently no-op
  this->left_fencer_light_ ->tick(elapsed_micros);
  this->right_fencer_light_->tick(elapsed_micros);
}
    

// Hopefully all self-explanatory 
void Fencing_Light_Displays::display_left_on_target()
{
  this->left_fencer_light_->light_up_red();
}


void Fencing_Light_Displays::display_right_on_target()
{
  this->right_fencer_light_->light_up_green();
}

void Fencing_Light_Displays::display_left_off_target()
{
  this->left_fencer_light_->light_up_white();
}


void Fencing_Light_Displays::display_right_off_target()
{
  this->right_fencer_light_->light_up_white();
}


void Fencing_Light_Displays::display_left_short_circuit()
{
  this->left_fencer_light_->light_up_short_circuit_light();
}

void Fencing_Light_Displays::display_right_short_circuit()
{
  this->right_fencer_light_->light_up_short_circuit_light();
}

void Fencing_Light_Displays::reset_lights()
{
  this->left_fencer_light_ ->go_dark();
  this->right_fencer_light_->go_dark();
}

void Fencing_Light_Displays::set_brightness(uint8_t brightness)
{
  this->left_fencer_light_ ->set_brightness(brightness);
  this->right_fencer_light_->set_brightness(brightness);
}

void Fencing_Light_Displays::show_off_on_startup()
{
  // currently no-op
}

void Fencing_Light_Displays::show_off_on_labelle()
{
  // currently no-op
}
