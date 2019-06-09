#define ABSZERO 273.15
#define MAXANALOGREAD 1023.0

float temperature_NTC(float T0, float R0, float T1, float R1, float RV, float VA_VB)
{
 T0+=ABSZERO;
 T1+=ABSZERO;
 float B= (T0 * T1)/ (T1-T0) * log(R0/R1);
 float RN=RV*VA_VB / (1-VA_VB);
 return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}

float pressureFormula(float bValue)
{
 float P= bValue * 0.0122189638318671 - 1.25; // (5V²/(1023*2)) - (5V/4V) // 4V = range (4,5V bei 10 bar - 0,5V bei 0 bar) ?
 return P;
}

void setup()
{
 Serial.begin(9600);
}

#define aPin1 A1
#define aPin2 A2

void loop()
{
 float T0=40;
 float R0=5830;
 float T1=150;
 float R1=316;
 float Vorwiderstand=1000;
 float temp;
 float voltIn;
 float press;
 int aValue=analogRead(aPin1);
 int bValue=analogRead(aPin2);

 temp=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue/MAXANALOGREAD);
 Serial.print("Temp: ");Serial.print(temp);Serial.println(" C");
 
 //Serial.print("bValue: ");Serial.println(bValue);
 //voltIn = bValue*0.00488758553;
 //Serial.print("Volt: ");Serial.print(voltIn, 1);Serial.println(" V");
 if (bValue<103) {
 	bValue = 103;
 }
 if (bValue>920) {
 	bValue = 920;
 }
 //Serial.print("bValue: ");Serial.println(bValue);
 
 press=pressureFormula(bValue);
 Serial.print("Druck: ");Serial.print(press, 2);Serial.println(" bar");

 delay(1000);
}
