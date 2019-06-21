#include <Arduino.h>
#include <U8x8lib.h>

U8X8_SH1106_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 2, /* data=*/ 0, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display

void setup_OLED(void)
{
  u8x8.begin();
  u8x8.setPowerSave(0);

  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"oil and pres");
}
