/*
 * rgb.ino
 * 
 * Control a RGB led via three analog outputs.
 * 
 * Supported functions:
 * ====================
 * 
 * Query data: No
 *   
 * Query config: Yes
 *   send intensity per color
 *     Keys: R, G, B
 *   
 * Set config: Yes
 *   set intensity per color
 *     Keys: R, G, B
 */

#ifdef RGB_SUPPORT

size_t RgbAnalog::setup(unsigned int eepromLocation)
{
  pinMode(RGB_RED_PORT, OUTPUT);
  pinMode(RGB_GREEN_PORT, OUTPUT);
  pinMode(RGB_BLUE_PORT, OUTPUT);

  digitalWrite(RGB_RED_PORT, LOW);
  digitalWrite(RGB_GREEN_PORT, LOW);
  digitalWrite(RGB_BLUE_PORT, LOW);

  setRGB( 0, 0, 0);

  return (size_t)0;
}

void RgbAnalog::timeout()
{
  /* nothing */  
}

const char *RgbAnalog::getName()
{
  return "RGB";
}

void RgbAnalog::getData()
{
  /* Nothing to do */
}

void RgbAnalog::sendData()
{
  /* Not supported */
}

void RgbAnalog::sendConfig()
{
  sendMoreDataStart();
  Serial.print("R=");
  Serial.print(red);
  Serial.print(" G=");
  Serial.print(green);
  Serial.print(" B=");
  Serial.print(blue);
  sendMoreDataEnd();
}

void RgbAnalog::setConfig()
{
  setRGB( getIntParam("R", 0), getIntParam("G", 0),getIntParam("B", 0));
}


/* ***************** PRIVATE *************************************************/


void RgbAnalog::setRGB( byte r, byte g, byte b)
{
  red = r;
  green = g;
  blue = b;
  
  analogWrite(RGB_RED_PORT, r);
  analogWrite(RGB_GREEN_PORT, g);
  analogWrite(RGB_BLUE_PORT, b);
}

#endif
