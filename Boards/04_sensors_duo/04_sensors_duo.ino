
/* 
 * Defaultly disabled. More details: https://docs.particle.io/reference/firmware/photon/#system-thread 
 */
//SYSTEM_THREAD(ENABLED);

/*
 * Defaultly disabled. If BLE setup is enabled, when the Duo is in the Listening Mode, it will de-initialize and re-initialize the BT stack.
 * Then it broadcasts as a BLE peripheral, which enables you to set up the Duo via BLE using the RedBear Duo App or customized
 * App by following the BLE setup protocol: https://github.com/redbear/Duo/blob/master/docs/listening_mode_setup_protocol.md#ble-peripheral 
 * 
 * NOTE: If enabled and upon/after the Duo enters/leaves the Listening Mode, the BLE functionality in your application will not work properly.
 */
//BLE_SETUP(ENABLED);

/*
 * SYSTEM_MODE:
 *     - AUTOMATIC: Automatically try to connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *     - SEMI_AUTOMATIC: Manually connect to Wi-Fi and the Particle Cloud, but automatically handle the cloud messages.
 *     - MANUAL: Manually connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *     
 * SYSTEM_MODE(AUTOMATIC) does not need to be called, because it is the default state. 
 * However the user can invoke this method to make the mode explicit.
 * Learn more about system modes: https://docs.particle.io/reference/firmware/photon/#system-modes .
 */
#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define ABSZERO 273.15
#define MAXANALOGREAD 4095.0

float temperature_NTC(float T0, float R0, float T1, float R1, float RV, float VA_VB) {
  T0+=ABSZERO;
  T1+=ABSZERO;
  float B= (T0 * T1)/ (T1-T0) * log(R0/R1);
  float RN=RV*VA_VB / (1-VA_VB);
  return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}

float pressureFormula(float bValue) {
  float P= bValue * 0.00305250305250305 - 1.25;
  return P;
}

void setup() {
  Serial.begin(9600);
}

#define aPin1 A0
#define aPin2 A5

void loop() {
  float T0=40;
  float R0=5830;
  float T1=150;
  float R1=316;
  float Vorwiderstand=1000;
  float temp;
  float press;
  int aValue=analogRead(aPin1);
  int bValue=analogRead(aPin2);
  float aVoltIn;
  float bVoltIn;
  //
  Serial.print("aValue: ");Serial.println(aValue);
  Serial.print("bValue: ");Serial.println(bValue);
  //
  aVoltIn = aValue*0.000805860805860806;
  bVoltIn = bValue*0.000805860805860806;
  //
  Serial.print("aVolt: ");Serial.print(aVoltIn, 1);Serial.println(" V");
  Serial.print("bVolt: ");Serial.print(bVoltIn, 1);Serial.println(" V");
  //
  temp=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue/MAXANALOGREAD);
  Serial.print("Temp: ");Serial.print(temp);Serial.println(" C");
  //
  if (bValue<410) {
    bValue = 410;
  }
  if (bValue>3689) {
    bValue = 3689;
  }
  press=pressureFormula(bValue);
  Serial.print("Druck: ");Serial.print(press, 2);Serial.println(" bar");
  //
  delay(1000);
}
