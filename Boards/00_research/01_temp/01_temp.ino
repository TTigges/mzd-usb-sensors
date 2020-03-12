
// NTC temperature calculation by "jurs" for German Arduino forum
#define ABSZERO 273.15
#define MAXANALOGREAD 1023.0

float temperature_NTC(float T0, float R0, float T1, float R1, float RV, float VA_VB)
// Ermittlung der Temperatur mittels NTC-Widerstand
// Version der Funktion bei unbekannter Materialkonstante B
// Erklärung der Parameter:
// T0           : Nenntemperatur des NTC-Widerstands in °C
// R0           : Nennwiderstand des NTC-Sensors in Ohm
// T1           : erhöhte Temperatur des NTC-Widerstands in °C
// R1           : Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
// Vorwiderstand: Vorwiderstand in Ohm  
// VA_VB        : Spannungsverhältnis "Spannung am NTC zu Betriebsspannung"
// Rückgabewert : Temperatur
{
 T0+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 T1+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 float B= (T0 * T1)/ (T1-T0) * log(R0/R1); // Materialkonstante B
 float RN=RV*VA_VB / (1-VA_VB); // aktueller Widerstand des NTC
 return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}


void setup()
{
 Serial.begin(9600);
}


#define ANALOGPIN A0

void loop()
{
 float T0=40;    // Nenntemperatur des NTC-Widerstands in °C
 float R0=5830; // Nennwiderstand des NTC-Sensors in Ohm
 float T1=150;   // erhöhte Temperatur des NTC-Widerstands in °C
 float R1=316;  // Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
 float Vorwiderstand=1000; // Vorwiderstand in Ohm  
 float temp;    
 int aValue=analogRead(ANALOGPIN);
 
 // Berechnen bei unbekannter Materialkonstante
 temp=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue/MAXANALOGREAD);
 Serial.print("NTC : ");Serial.print(temp);Serial.println(" C");

 delay(1000);
}