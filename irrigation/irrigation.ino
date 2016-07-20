//set APN to Hologram.io rather than Particle
#include "cellular_hal.h" //required to set APN
STARTUP(cellular_credentials_set("apn.konekt.io", "", "", NULL));

//PIN DEFINITIONS
    //inputs (Analog A0-A5)
    const int hygro1Pin = A5
    const int hygro2Pin = A4
    const int anemoPin = A0

    //leds
    const int idleLED = D7;

    //outputs (Digital D0-D4, D7)
    const int valveRow1 = D0;
    const int valveRow2 = D1;
    const int valveRow3 = D2;
    const int valveSprinkler = D3;
    const int valvePump = D4;
    
    //related globals
    bool pumpRunning = false;

    // create FuelGauge
    FuelGauge fuel;

    /* Buttons Deprecated for virtual ones
    const int btnValve1 = B0;
    const int btnValve2 = B1;
    const int btnValve3 = B2;
    const int btnSprinkler = B3;
    const int btnPump = B4;
    */


//GLOBALS
    
    //hygro values
    double hygro1 = 0; //Variable stores the value direct from the analog pin
    double hygro2 = 0; //Variable stores the value direct from the analog pin

    //anemo globals - Anemometer requires 7-24v

    int anemoValue = 0; //Variable stores the value direct from the analog pin
    float anemoVoltage = 0; //Variable that stores the voltage (in Volts) from the anemometer being sent to the analog pin
    float windSpeed = 0; // Wind speed in meters per second (m/s)
    float voltageConversionConstant = .00322265625; //This constant maps the value provided from the analog read function, which ranges from 0 to 1023, to actual voltage, which ranges from 0V to 5V
    int anemoDelay = 3000; //Delay between sensor readings, measured in milliseconds (ms)

    float anemoVoltMin = 0.4; // Mininum output voltage from anemometer in mV.
    float windSpeedMin = 0; // Wind speed in meters/sec corresponding to minimum voltage
    
    float anemoVoltMax = 2.0; // Maximum output voltage from anemometer in mV.
    float windSpeedMax = 72.47; // Wind speed in meters/sec corresponding to maximum voltage, tolerance is 0.3m/s tolerant

    //loop and logic globals
    double loopCounter = 0;


//SUBSCRIPTIONS and GLOBAL FUNCTIONS

    //--- Begin IFTTT Subscription ---
    
    void IFTTTsentEvent(const char *event, const char *data)
    {
      Particle.publish("Received event from IFTTT", "Event: " + (String)event + ", Data: " + (String)data, 60, PRIVATE); 
      
      if ((String)event == "IFTTT-relay"){
      relayTest();
      }
      /*if (data)
      Serial.println(data);
      else
      Serial.println("NULL");
      */
    }
      
    void relayTest(){
        Particle.publish("Relays Fired", "!!", 60, PRIVATE);
    }

    // --- End IFTTT Subscription ---
    
    
    //make setting LED color, brightness, and times to flash easy.
    void setLED(int red, int green, int blue, int bright, int timesToBlink = 0){
        RGB.color(red,green,blue); //set color
    
        if (timesToBlink > 0){
            for (int count = 0; count < timesToBlink; count++) {
                RGB.brightness(255); //full
                delay(20);
                RGB.brightness(0); //off
                delay(20);
            }
        }
        
        RGB.brightness(bright); //set bright
    }
            


//SETUP

void setup() {
    //take control of RGB
    RGB.control(true);

    //show we're in control
    setLED(255,255,0,255,100); //r,g,b,%,x - flashing red

    //inputs (Analog A0-A5)
    const int hygro1Pin = A5;
    const int hygro2Pin = A4;
    const int anemoPin = A0;

    //leds
    const int idleLED = D7;

    //outputs (Digital D0-D4, D7)
    const int valveRow1 = D0;
    const int valveRow2 = D1;
    const int valveRow3 = D2;
    const int valveSprinkler = D3;
    const int valvePump = D4;
    
    //init up indicator LED, not on though.
    pinMode(idleLED, OUTPUT);
    
    //set up sensor pins
    pinMode(hygro1Pin, INPUT_PULLDOWN);
    pinMode(hygro2Pin, INPUT_PULLDOWN);
    pinMode(hygro3Pin, INPUT_PULLDOWN);
    pinMode(hygro4Pin, INPUT_PULLDOWN);
    pinMode(hygro5Pin, INPUT_PULLDOWN);
    pinMode(anemoPin, INPUT_PULLDOWN);

    //set up relays
    pinMode(valveRow1, OUTPUT);
    pinMode(valveRow2, OUTPUT);
    pinMode(valveRow3, OUTPUT);
    pinMode(valveSprinkler, OUTPUT);
    pinMode(valvePump, OUTPUT);

    //init relays to the open state
    digitalWrite(valveRow1, HIGH);
    digitalWrite(valveRow2, HIGH);
    digitalWrite(valveRow3, HIGH);
    digitalWrite(valveSprinkler, HIGH);
    digitalWrite(valvePump, HIGH);

    Serial.begin(9600);
    
    Particle.variable("soil", soil);
    Particle.subscribe("IFTTT", IFTTTsentEvent);
    
    //DroneHome has booted. GPS will update every 20 minutes. Battery|
    //String strBootup = String::format("%d min updates. Power: %.2fv, %.2f\%.",delayMinutes,fuel.getVCell(),fuel.getSoC());
    String strBootup = String::format("Power: %.2fv, %.2f\%.",fuel.getVCell(),fuel.getSoC());
    
    //init complete!
    setLED(0,0,255,255,0); //r,g,b,%,x - full green
}

// LOOP

void loop() {

    loopCounter++;

    float hygro1 = analogRead(hygro1Pin);
    hygro1 = constrain(hygro1, 1192, 4095);
        //hygro1 = 2642;
    float hygro2 = analogRead(hygro2Pin);
    hygro2 = constrain(hygro2, 1192, 4095);
        //hygro2 = 2642;

    // Convert to percentages
    // Value read minus 1192 (low value), then divided into 2093 (range between 1192 and 4095) = percentage dry (p). Then 100 - (p) will be percent wet (moisture percentage).
    hygro1Percent = 100-(((hygro1-1192)/2903)*100);
    hygro2Percent = 100-(((hygro2-1192)/2903)*100);

    float hygroAvg = (hygro1Percent + hygro2Percent) / 2;

    anemoValue = analogRead(anemoPin); //Get a value between 0 and 1023 from the analog pin connected to the anemometer
    anemoVoltage = anemoValue * voltageConversionConstant; //Convert sensor value to actual voltage

    //Convert voltage value to wind speed using range of max and min voltages and wind speed for the anemometer
    if (anemoVoltage <= anemoVoltMin){
        windSpeed = 0; //Check if voltage is below minimum value. If so, set wind speed to zero.
    }
    else {
        windSpeed = (anemoVoltage - anemoVoltMin) * windSpeedMax / (anemoVoltMax - anemoVoltMin); //For voltages above minimum value, use the linear relationship to calculate wind speed.
    }

    //Publish Results

    if(loopCounter == 6*60*3){ //get to minutes, get to hours, number of hours.
        
        //Every 3 hours, publish hygroAverage
        Serial.print(soil);
        Serial.println("%");
        Serial.print(soil2);
        Serial.println("%");
        Particle.publish("SoilLog", (String)hygroAvg + "%");

        loopCounter = 0;    //reset loop counter
    }
    
    /*
    Particle.publish("Soil Log", 
    "soil:" + String::format("%.2f",soil) + "\%" + 
    ", soil2:" + String::format("%.2f",soil2) + "\%" + 
    " - soilAvg:" + String::format("%.2f",soilAvg) + "\%",
    60, PRIVATE
    );
    */
  
    Serial.print("Voltage: ");
    Serial.print(sensorVoltage);
    Serial.print("Wind speed MPH: ");
    Serial.println(windSpeed);
    Particle.publish("WindSpeed", (String)windSpeed);

    delay(10000);   // loop every 10 seconds
}