/** speedometer-config.js ************************************************************** *\
|* =========================                                                             *|
|* Speedometer Configuration - Used to configure position of Speedometer values.         *|
|* =========================                                                             *|
|* Main Speedometer Value: [0, 0, 0] - Large, Front, & Center.                           *|
|* [ 0/1/2:(0 For Main Column, 1 For Bottom Rows, 2+ To Hide), Row Number, Position ]    *|
|* Main Column Positions: 4 Values (1-4 From Top to Bottom)                              *|
|* Bottom Rows Positions: 5 Values Per Row (1-5 From Left to Right)                      *|
|* Examples:                                                                             *|
|* [0, 1, 4] = [Main, Column, 4th position (Bottom of the Column)]                       *|
|* [1, 3, 1] = [Bottom, 3rd Row, First Position (Left Side)]                             *|
|* [1, 1, 5] = [Bottom, 1st Row, Last Position (Right Side)]                             *|
|* To Hide a Value = [2, 1, 0] (2 As The First Number)                                   *|
|* To Change Bottom Row Push Command Knob ("Select")                                     *|
|* Note: Only numbers inside [] brackets determine position, order in this list DOES NOT *|
\* ************************************************************************************* */
var spdBottomRows = 3;   //Number of Bottom Rows
var spdTbl = {
  vehSpeed:   [0, 0, 0], //Vehicle Speed
  topSpeed:   [0, 1, 1], //Top Speed
  avgSpeed:   [0, 1, 2], //Average Speed
  gpsSpeed:   [0, 1, 3], //GPS Speed
  engSpeed:   [0, 1, 4], //Engine Speed
  outTemp:    [1, 1, 1], //Outside Temperature
  inTemp:     [2, 1, 2], //Intake Temperature
  coolTemp:   [1, 1, 2], //Coolant Temperature
  oilTemp:    [1, 1, 3], //Oil Temperature
  oilPres:    [1, 1, 4], //Oil Pressure
  tpmsFlTemp: [1, 2, 1], //TPMS Front Left Temperature -
  tpmsFrTemp: [1, 2, 2], //TPMS Front Right Temperature
  tpmsRlTemp: [1, 2, 3], //TPMS Rear Left Temperature
  tpmsRrTemp: [1, 2, 4], //TPMS Rear Right Temperature
  tpmsFlPres: [1, 3, 1], //TPMS Front Left Pressure -
  tpmsFrPres: [1, 3, 2], //TPMS Front Right Pressure
  tpmsRlPres: [1, 3, 3], //TPMS Rear Left Pressure
  tpmsRrPres: [1, 3, 4], //TPMS Rear Right Pressure
  trpTime:    [2, 1, 0], //Trip Time
  trpIdle:    [2, 1, 0], //Idle Time
  trpDist:    [2, 1, 0], //Trip Distance
  fuelLvl:    [2, 2, 0], //Fuel Gauge Level
  gpsHead:    [2, 2, 0], //GPS Heading
  gpsAlt:     [2, 2, 0], //Altitude
  gpsAltMM:   [2, 2, 0], //Altitude Min/Max
  trpFuel:    [2, 2, 0], //Trip Fuel Economy
  gearPos:    [2, 3, 0], //Gear Position
  gearLvr:    [2, 3, 0], //Transmission Lever Position
  engTop:     [2, 3, 0], //Engine Top Speed
  avgFuel:    [2, 3, 0], //Average Fuel Economy
  engLoad:    [2, 4, 0], //Engine Load
  gpsLat:     [2, 4, 0], //GPS Latitude
  gpsLon:     [2, 4, 0], //GPS Longitude
  totFuel:    [2, 4, 0], //Total Fuel Economy
  trpEngIdle: [2, 4, 0], //Engine Idle Time
  batSOC:     [2, 4, 0], //Battery Charge State (i-stop)
};

/* ************************************************** */
/* Set this to true to use your values below ******** */
/* If this is false the following values are not used */
var overRideSpeed=false;
/* ************************************************** */
/* ****************** Start OverRide Variables ****** */
var SORV = {
  // Set the language for the speedometer
  // Available EN, ES, DE, PL, SK, TR, FR, IT
  language: "DE",

  // Used for metric/US english conversion flag (C/F, KPH/MPH, Meter/Feet, L per 100km/MPG)
  // Set isMPH: true for MPH, Feet, MPG
  // Set isMPH: false for KPH, Meter
  isMPH: false,

  // Set This to true to start with the Bar Speedometer Mod
  // False to use the analog speedometer
  barSpeedometerMod: true,

  // Set true to enable multicontroller and other mod features in classic mode
  // If false then use classic speedometer without Mods
  speedMod: true,

  // Set to true to start the classic speedometer in analog mode
  // False to start in digital mode
  startAnalog: true,

  // Set it true for the StatusBar Speedometer
  // False if you don't want the small speedometer in statusbar
  StatusBarSpeedometer: true,

  // Set to true for Outside Temperature & Fuel Efficiency in the statusbar
  // False for Compass & Altitude
  sbTemp: false,

  // Set true if you want the original speedometer background image as in version 4.2 or below
  // False for no background
  // If "true" the opacity above will be ignored
  original_background_image: false,

  // Set the opacity of black background color for speedometer, to reduce the visibility of custom MZD background images
  // Possible values 0.0 (full transparent) until 1.0 (complete black background)
  black_background_opacity: 0.0,

  // Set unit for fuel efficiency to km/L
  // False for L/100km
  fuelEffunit_kml: false,

  // Set this to true for Fahrenheit
  // False for Celsius
  tempIsF: false,

  // Set this to true for psi
  // False for bar
  pressIsPsi: false,

  // For the Speed Bar false for Current Vehicle Speed
  // Set This to true if you want the Colored Bar to measure engine speed
  engineSpeedBar: false,

  // Set This to true to hide the Speed Bar
  // False shows he bar
  hideSpeedBar: false,

  // Set this to true to enable counter animation on the speed number
  // False to disable speed counter animation
  // The animation causes the digital number to lag by 1 second
  speedAnimation: false,

  // Set this to the color of the Analog SpeedoMeter
  // Valid Colors are (Capitalized and in quotes): Red, Blue, Green, Yellow, Pink, Orange, Purple, silver
  analogColor: "Red",

  // Set to the color theme for Bar SpeedoMeter
  // Theme will be a number 0-5 (0 is default white)
  barTheme: 0,

  // Set suffix appended to gauge value
  // default is "%" to show available fuel percentage
  fuelGaugeValueSuffix: "%",

  // Set multiplier to get human readable output fuel value from its internal reading
  // default is 100 to show remaining percentage
  // set to fuel tank capacity in liters/gallons etc. and change aforementioned [fuelGaugeValueSuffix] to "L" etc.
  fuelGaugeFactor: 100,
};
