//===========================================================================//
//  Name    : Fencing_Light.cpp                                              //
//  Desc    : C++ Implementation for a RBG Ring Fencing Light                //
//  Dev     : Nate Cope,                                                     //
//  Date    : Jan 2023                                                       //
//  Version : 1.2                                                            //
//  Notes   : TODO a show-off on time running out, too??                     //
//            TODO cool animation methods and resulting tick method          // 
//===========================================================================//

// interface includes
#include "Fencing_Light.h"

// TODO TODO TODO redundancy color checks! 
// TODO short circuit light implementation logic is gonna need work 
//      to keep it essentially independant from what the other lights 
//      are doing
// TODO eventually probably a by-pixel redundancy check instead of by state??
// TODO light I think eventually needs to take over its own timing; 
//      accept a duration to display whatever light method gets called...
// TODO short circuit lights are a new color

// Constructor 
//    uint8_t control_pin - the Arduino pin control terminal of the fencing light  
Fencing_Light::Fencing_Light(uint8_t control_pin)
{
  // set the control pin 
  this->CONTROL_PIN_ = control_pin;

  // define the NeoPixel object, that last arg is some library thing we don't have to care about 
  this->led_ring_ = new Adafruit_NeoPixel(this->LED_COUNT_, this->CONTROL_PIN_, NEO_GRB + NEO_KHZ800);

  // INITIALIZE the NeoPixel object 
  this->led_ring_->begin();  

  // Set BRIGHTNESS to 1/5 the max of 255 as an arbitrary starting point 
  this->led_ring_->setBrightness(this->MAX_BRIGHTNESS_ / 5); 

  // if you call this line here, the whole thing hangs and won't respond
  //   and I don't know why. Fortunately, you don't need to. 
  // this->go_dark();            
}


// Destructor 
Fencing_Light::~Fencing_Light()
{
  // delete the underlying object we allocated memory for 
  delete this->led_ring_;      
}

// Lets the object know what the current time is. For the sake of streamlining 
// the main code, this class should never check the time or call any sort of delay function,
// but rely on this method to tell it what the time is, and update that way. 
// if "0" is passed in specifically, we're just updating the display, and no time checks are done 
void Fencing_Light::tick(unsigned long current_time_micros)
{
  // currently NO-OP
}


// Illuminate green to show an on-target hit! 
void Fencing_Light::light_up_green()
{
  if (this->current_display_state != this->display_state::ALL_GREEN)  // redundancy check 
  {
    set_all_leds_to_color(this->color::GREEN); 
    this->current_display_state != this->display_state::ALL_GREEN; 
  }
}


// Illuminate red to show an on-target hit! 
void Fencing_Light::light_up_red()
{
  if (this->current_display_state != this->display_state::ALL_RED)  // redundancy check 
  {
    set_all_leds_to_color(this->color::RED); 
    this->current_display_state = this->display_state::ALL_RED;
  }
}


// Illuminate green to show an off-target hit! 
void Fencing_Light::light_up_white()
{
  if (this->current_display_state != this->display_state::ALL_WHITE)   // redundancy check 
  {
    set_all_leds_to_color(this->color::WHITE);
    this->current_display_state = this->display_state::ALL_WHITE; 
  }
}


// Show the "touching own lame" signal  // TODO make them a new color so they're always visible 
void Fencing_Light::light_up_short_circuit_light()
{
  // redundancy check 
  if (!this->short_circuit_signal_on)
  {
    //  set some ARBITRARY pattern (currently a square of 1, 5, 9, 13) to white
    this->led_ring_->setPixelColor( 1,  this->get_color_code(this->color::WHITE) );      
    this->led_ring_->setPixelColor( 5,  this->get_color_code(this->color::WHITE) );   
    this->led_ring_->setPixelColor( 9,  this->get_color_code(this->color::WHITE) );      
    this->led_ring_->setPixelColor( 13, this->get_color_code(this->color::WHITE) );     
    
    //  Update ring to match set colors TODO TODO redundancy check 
    this->led_ring_->show();  
  
    this->short_circuit_signal_on = true; 
  }
}

// control how bright the signals are! max 255, min 0 
void Fencing_Light::set_brightness(uint8_t brightness)
{
  // make sure the input doesn't exceed logical limits 
  if      (brightness > MAX_BRIGHTNESS_) brightness = MAX_BRIGHTNESS_; 
  else if (brightness < MIN_BRIGHTNESS_) brightness = MIN_BRIGHTNESS_; 

  // update the internal parameter
  this->brightness_ = brightness;

  // set the object's characteristics 
  this->led_ring_->setBrightness(this->brightness_);
}


// stop showing any lights 
void Fencing_Light::go_dark()
{
  if (this->current_display_state != this->display_state::DARK)   // redundancy check 
  {
    set_all_leds_to_color(this->color::NONE); 
    this->current_display_state   = this->display_state::DARK;
    this->short_circuit_signal_on = false;  // TODO this is gonna need work; short circuit needs to be kinda independent...
  }          
}


// do something cool to greet the world!  
void Fencing_Light::show_off_on_startup()
{
  // currently no-op
}


// do something cool for 4-4 or 15-15!
void Fencing_Light::show_off_on_labelle()
{
  // currently no-op
}

//
//  private methods 
//

// helper method to make code clean. sets all the pixels in the ring
//  to the provided color (provided as an enum)
void Fencing_Light::set_all_leds_to_color(color color_enum_val)
{
  // For each pixel in strip...
  for (int i = 0; i < this->led_ring_->numPixels(); i++) 
  { 
    //  Set pixel's colors
    this->led_ring_->setPixelColor( i, this->get_color_code(color_enum_val) );         
  }

  //  Update ring to match set colors 
  this->led_ring_->show();                    
}


// helper method to make code clean, just converts a color enum value 
//  into an actual underying-class color value (some uint32_t) that 
//  it can understand  
uint32_t Fencing_Light::get_color_code(color color_enum_val)
{
    uint32_t return_val = 0; 
    
    switch (color_enum_val)
    {
      case this->color::RED:
        return_val = this->led_ring_->Color( this->MAX_BRIGHTNESS_,   
                                             this->MIN_BRIGHTNESS_,   
                                             this->MIN_BRIGHTNESS_
                                          );
        break;
      case this->color::GREEN:
        return_val = this->led_ring_->Color( this->MIN_BRIGHTNESS_,   
                                             this->MAX_BRIGHTNESS_,   
                                             this->MIN_BRIGHTNESS_
                                          );
        break;
      case this->color::WHITE:
        // half brightness because all three lights working together 
        //  makes it seem brighter anyway 
        return_val = this->led_ring_->Color( this->MAX_BRIGHTNESS_ / 2,   
                                             this->MAX_BRIGHTNESS_ / 2,    
                                             this->MAX_BRIGHTNESS_ / 2
                                          );
        break;
      default:  // just zero (dark) on anything else. color::NONE ends up here, importantly 
        return_val = this->led_ring_->Color( this->MIN_BRIGHTNESS_,   
                                             this->MIN_BRIGHTNESS_,   
                                             this->MIN_BRIGHTNESS_
                                          );
    }

    return return_val; 
}
