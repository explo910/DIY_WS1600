// Wetterstation WS1600 - DIY
// ----------------------------
// Sensors Used:
// -------------
// Temp/Humid Sensor: AM2302
// Rain Sensor WS 1600 - TX26
// Wind Sensor WS 1600 - TX23
// ----------------------------


// ----------------------------
// Testet with Arduino Uno
// ----------------------------

#include "DHT.h" // Import ADAFruit DHT Lib - https://github.com/adafruit/DHT-sensor-library


//Config Pins
#define DHTPIN 4
#define WINDPIN 5
#define RAINPIN 2

#define SERIAL HIGH

//Init DHT22
DHT dht(DHTPIN, "DHT22"); //Initialize DHT Lib
#define BIT_MS 1250 //Define time of one Bit of TX23 in ms

//Global Variables
float rainfall = 0;
string winddirection = "";


boolean stat; // data input status
boolean GoodData; // dataquality Actual Run
int GoodCounterCheck;

word good = 0;
word bad = 0;
int percentage = 0;

// Setup routine
void setup() {
	if (SERIAL == HIGH) {
		Serial.begin(9600); 		//Turn on Serial for Debugging
	}
	
	pinMode(WINDPIN, OUTPUT);	// Set Windpin to Output
	pinMode(RAINPIN, INPUT); 	// Init RAINPIN
	
	//Attach Intterupt to RAINPIN
	attachInterrupt(digitalPinToInterrupt(RAINPIN), RegenSensor, FALLING);
	dht.begin(); //Initialize DHT22 (AM2302)
}


// Main loop
void loop() {
	delay (3500); //Start with Delay to get TX23 to initialize
	WindSensor(); 
	TempHumidSens();
	
	if (SERIAL == HIGH) {
	
	}
}


void WindSensor() {
	word startframe;
	word dataword;
	word dataword2;
	word dataword3;
	word dataword4;// what we print to PC
	word dataword5;
	word HeaderWord;
	word dwws0;
	word dwws1;
	word dwws2;
	word calcCheckSum;

	dataword = 0; // Prepare data word
	dataword2 = 0; // Prepare data word
	dataword3 = 0; // Prepare data word
	dataword4 = 0; // Prepare data word
	dataword5 = 0; // Prepare data word
	dwws0 = 0;
	dwws1 = 0;
	dwws2 = 0;
	calcCheckSum = 0;
	startframe = 0;
	GoodCounterCheck = 0;
	GoodData = HIGH;
	pinMode(WINDPIN,OUTPUT);
	digitalWrite(WINDPIN, LOW);
	delay(250);
	pinMode(WINDPIN, INPUT);
		
	stat = digitalRead(WINDPIN);	// get the sensor input
	while(stat == HIGH && GoodCounterCheck <= 10) {			// wait for falling edge (trigger for data sending)
		stat = digitalRead(WINDPIN);
		GoodCounterCheck = GoodCounterCheck +1;
	}
	if (GoodCounterCheck < 10) {
		GoodCounterCheck = 0;
		while(stat == HIGH && GoodCounterCheck <= 10) {			// wait for falling edge (trigger for data sending)
			stat = digitalRead(WINDPIN);
			GoodCounterCheck = GoodCounterCheck +1;
		}
		if (GoodCounterCheck < 10) {
			GoodCounterCheck = 0;
			while(stat == LOW && GoodCounterCheck <= 10) {			// wait for start frame
			stat = digitalRead(WINDPIN);
			GoodCounterCheck = GoodCounterCheck +1;
			}
		}
		else {
			GoodData = LOW;
		}
	}
	else {
		GoodData = LOW;
	}

	if (GoodData == HIGH) {
	for (int i=0; i<16; i++) { //to get exact position for start frame
		delayMicroseconds(BIT_MS);
	}
	for (int i=0; i<5; i++) {
		stat = digitalRead(WINDPIN);
		startframe = startframe + (stat << i);
		delayMicroseconds(BIT_MS); // Wait for next bit
	}
	
	if ((startframe & 31) == 27) {
		for (int i=0; i < 4; i++) { //Read Wind direction
		stat = digitalRead(WINDPIN);
		dataword = dataword + (stat << i);
		delayMicroseconds(BIT_MS); // Wait for next bit
		}
		for (int i=0; i < 11; i++) { //Read Wind Speed
		stat = digitalRead(WINDPIN);
		dataword2 = dataword2 + (stat << i);
		if (i < 4)
		{
		dwws0 = dwws0 + (stat << i);
		}
		if ((i > 3) && (i < 8)) {
		dwws1 = dwws1 + (stat << (i-4));
		}
		if ((i > 7)) {
		dwws2 = dwws2 + (stat << (i-8));
		}
		delayMicroseconds(BIT_MS); // Wait for next bit
		}
		for (int i=0; i < 4; i++) { //Read Checksum
		stat = digitalRead(WINDPIN);
		dataword3 = dataword3 + (stat << i);
		delayMicroseconds(BIT_MS); // Wait for next bit
		}
		for (int i=0; i < 4; i++) { //ReadInvertet Wind Direction
		stat = digitalRead(WINDPIN);
		stat = !stat;
		dataword4 = dataword4 + (stat << i);
		delayMicroseconds(BIT_MS); // Wait for next bit
		}
		for (int i=0; i < 11; i++) { //Read Invertet Wind Speed
		stat = digitalRead(WINDPIN);
		stat = !stat;
		dataword5 = dataword5 + (stat << i);
		delayMicroseconds(BIT_MS); // Wait for next bit
		}
	}
	else {
		Serial.println("StartFrame Invalid");
		Serial.println(startframe, BIN);
		GoodData = LOW;
	}
	}
	else {
		Serial.println("No Reaction from Sensor - Disconnected ?");
		GoodData = LOW;
	}
	//Calculate Checksum
	calcCheckSum = dataword + dwws0 + dwws1 + dwws2;
	calcCheckSum = calcCheckSum << 12;
	calcCheckSum = calcCheckSum >> 12; 
	if ((calcCheckSum != dataword3) || (dataword != dataword4) || (dataword2 != dataword5)) {
		GoodData = LOW;
		Serial.println("Checksum Wrong");
	}

	if (GoodData == HIGH) {
		Serial.print("Value: ");
		Serial.print(dataword); // Print out Dataword as readable ASCII string to PC
		Serial.print(" - Reversed Value: ");
		Serial.println(dataword4);
		//Serial.println(dataword3);
		Serial.print("Value: ");
		dataword2 = dataword2 * 0.18 * 2;
		Serial.print(dataword2);
		Serial.print(" - Reversed Value: ");
		dataword5 = dataword5 * 0.18 * 2;
		Serial.println(dataword5);
		Serial.print("Windspeed: ")
		Serial.print(dataword2 * 3.6)
		Serial.println(" km/h")
		good = good + 1;
	}
	else {
		bad = bad + 1;
	}

	Serial.print("Regenmenge: ");
	Serial.print(rainfall);
	Serial.println("mm");
	Serial.print("DataQuality - Good: ");
	Serial.print(good);
	Serial.print(" - Bad: ");
	Serial.println(bad);
	Serial.println("--------------------------------------------------------------");
	pinMode(WINDPIN,OUTPUT);
}


void RegenSensor() {
	//VERY hackish debouncing
	
	int Helper1 = 0; // I D K What this is ^^
	int Helper2 = 1; // Or this
	int ControlCounter = 0; //To ensure it doesnt loop long/endless
	bool RainPinState; //Variable to temporary Store Pin State - if used Directly it makes Problems sometimes
	
	while (Helper2 == 1 && ControlCounter < 10) {

		Helper1 = Helper1 + 1;
		delay(2);
		RainPinState = digitalRead(RAINPIN);

		if (RainPinState == LOW) {
		Helper2 = 1;
		}
		else {
			Helper2 = 0;
		}

		if (Helper1 >= 10000) {
			Helper2 = 0;
			Helper1 = 0;
		}
		ControlCounter = ControlCounter + 1;
	}
	
	if (Helper1 >= 5) {
		rainfall = rainfall + 0.5;
	}
}

void TempHumidSens() {
	float h = dht.readHumidity();	//Luftfeuchte auslesen
	float t = dht.readTemperature();	//Temperatur auslesen
	
	// Prüfen ob eine gültige Zahl zurückgegeben wird. Wenn NaN (not a number) zurückgegeben wird, dann Fehler ausgeben.
	if (isnan(t) || isnan(h)) {
		Serial.println("DHT22 konnte nicht ausgelesen werden");
	} 
	else {
		Serial.print("Luftfeuchte: "); 
		Serial.print(h);
		Serial.print(" %\t");
		Serial.print("Temperatur: "); 
		Serial.print(t);
		Serial.println(" C");
	}
}