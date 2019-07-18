/*
 * rgb.ino
 * 
 * Control a RGB led via three analog outputs.
 * 
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
  /* Nothing to do */
}

void RgbAnalog::sendConfig()
{
  sendMoreData( "R="+String(red));
  sendMoreData( "G="+String(green));
  sendMoreData( "B="+String(blue));
}

void RgbAnalog::setConfig()
{
  setRGB( getIntParam("R", 0), getIntParam("G", 0),getIntParam("B", 0));
}

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
