// Base custom code class, used for client communications.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

class CustomCode
{
public:
  #if EEPROM_FORMAT
  virtual void flash(void) {}
  #endif

  // called during setup()
  virtual void setup(void) {}
  
  // called during loop()
  virtual void loop(void) {}

  // set name for device
  virtual void setName(char *name) {}

  // send reply to client
  virtual void sendReply(char *instr) {}
};
