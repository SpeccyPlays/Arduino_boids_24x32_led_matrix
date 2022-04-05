/* Boids - flocking on 24x32 daisy chained LED matrixes
by Benjamin Gibbs
date started 2022/02/09

boidsv

## setup info ##
uses Max7219 LED 3 lots of 8 x 8 modules daisy chained
Use external power as the displays will use a lot of current
CLK - PIN 13
CS - PIN 10
DIN - PIN 11
module 0 = bottom left; top right is 11 in this instance
col 0 is at the bottom
MSB is on the left of the module

## aims ##
Improve method of displaying graphics on 3 daisy chained 8x32 MAX7219 led matrixes

## Updates - newest first ##



*/

//define commands for matrixes and module + screen constants
#include <SPI.h>
//#include <noDelay.h>
// op codes for MAX7219
#define NO_OP 0x00
#define DECODE_MODE 9
#define INTENSITY 10
#define SCAN_LIMIT 11
#define SHUTDOWN 12
#define DISPLAY_TEST 15
//CS pin
#define CSPIN 10
//joystick pins
#define JOYXPIN A0
#define JOYYPIN A1
#define JOYSWITCH 8
//sound
#define BZPIN 6
//module and screen details
const byte ROWWIDTH = 8;
const byte COLHEIGHT = 8;
const byte MODULES2 = 16;
const byte MODULES3 = 24;
const byte SCREENWIDTH = 32;
const byte SCREENHEIGHT = 24;
const byte LEVELWIDTH = 240;
const byte LEVELHEIGHT = 144;
const uint8_t numOfDevices = 12; //amount of modules all together
byte screenMemory [numOfDevices][8]{0};
byte xScreenStart = 0;
byte yScreenStart = 0;
// this allows me to select which module to display objects on
byte moduleLookupMatrix[4][5]{
	{32, 15, 14, 13, 12},
	{28, 11, 10, 9, 8},
	{24, 7, 6, 5, 4},
	{20, 3, 2, 1, 0}
};
struct boidSingle {
	byte x;
	byte y;
	byte velocity;
	int16_t angle;
};
boidSingle boidArray[50];
uint16_t globalAverageX = 0;
uint16_t globalAverageY = 0;
uint8_t amountOfBoids = sizeof(boidArray) / sizeof(boidArray[0]);
void boidSetup(boidSingle *array);
void showBoids(boidSingle *array);
void firstRule(boidSingle *array);
void secondRule(byte &x, byte &y, int16_t &angle, boidSingle *array);
uint16_t sendBuffer[numOfDevices]{0};	 //so we can send information to all modules in one stream
//joystick stuff
uint16_t joyX = 500;
uint16_t joyY = 500;
byte joyOffset = 50;
//noDelay resetTimer(1000);
void setup() {
	pinMode(CSPIN, OUTPUT);
	pinMode(JOYXPIN, INPUT);
	pinMode(JOYYPIN, INPUT);
	pinMode(JOYSWITCH, INPUT_PULLUP);
	Serial.begin(9600);
	SPI.begin();
	updateAll(INTENSITY, 0);
	updateAll(DISPLAY_TEST, 0);
	updateAll(DECODE_MODE, 0);
	updateAll(SCAN_LIMIT, 7);
	updateAll(SHUTDOWN, 1);
	sendScreenMemory();
	boidSetup(boidArray);
	firstRule(boidArray);
//	resetTimer.start();
}
void loop() {
//	checkJoyInput();
//	if (resetTimer.update()){
//		particleSetup(particleArray);
//		resetTimer.start();
//	}
//	else {
	showBoids(boidArray);
	sendScreenMemory();
	delay(50);
	clearScreenMemory();
	firstRule(boidArray);
//	Serial.println(globalAverageX);
//	Serial.println(globalAverageY);
//	}
}
void findAngleBetweenPoints(byte &x, byte &y, int16_t &angle){
// find dot product
//i hate maths
	byte dotProduct = (x * globalAverageX) + (y * globalAverageY);

	double magnitudeOld = sqrt(sq(x) + sq(y));
	double magnitudeNew = sqrt(sq(globalAverageX) + sq(globalAverageY));
	double a = dotProduct / (magnitudeOld * magnitudeNew);
	angle = (a * 180) / 3.14; //this should be cos(a) but what's there works better
//	Serial.println(angle);
}
void firstRule(boidSingle *array){
	for (byte i = 0; i < amountOfBoids; i++){
		globalAverageX += array[i].x;
		globalAverageY += array[i].y;
	}
	globalAverageX = (globalAverageX / (amountOfBoids ));
	globalAverageY = (globalAverageY / (amountOfBoids ));
}
void secondRule(byte &x, byte &y, int16_t &angle, boidSingle *array){
	for (byte i = 0; i < amountOfBoids; i++){
		if ((y + 2) == array[i].y | (x + 2) == array[i].x){
			y - 1;
			x - 1;
			array[i].velocity = 1;
			angle = array[i].angle;
		}
	}
}
void showBoids(boidSingle *array){
	for (byte i = 0; i < amountOfBoids; i++){
		plotPoint(array[i].x, array[i].y);
		array[i].velocity ++;
		findAngleBetweenPoints(array[i].x, array[i].y, array[i].angle);
		array[i].y += array[i].velocity * sin(array[i].angle);
		array[i].x += array[i].velocity * cos(array[i].angle);	
		if (array[i].x <= 0){
			array[i].x = SCREENWIDTH;
			array[i].velocity = 1;
		}
		if (array[i].x >= SCREENWIDTH + 1){
			array[i].x = 1;
			array[i].velocity = 1;
		}
		if (array[i].y <= 0){
			array[i].y = SCREENHEIGHT;
			array[i].velocity = 1; 
		}
		if (array[i].y >= SCREENHEIGHT + 1){
			array[i].y = 1;
			array[i].velocity = 1;
		}
		secondRule(array[i].x, array[i].y, array[i].angle, boidArray);
//		delay(500);
	}
}
void boidReset(byte &x, byte &y, byte &velocity, int16_t &angle){
	y = random(0, SCREENHEIGHT);
	x =  random(0, SCREENWIDTH - 1);
	velocity = random(1, 3);
	angle = random(0, 359);
}
void boidSetup(boidSingle *array){
	for (byte i = 0; i < amountOfBoids; i++){
		boidReset(array[i].x, array[i].y, array[i].velocity, array[i].angle);

	}
}
void updateAll(uint16_t cmd, uint8_t data){
//used for sending operation codes
	uint16_t x = (cmd << 8) | data;
	SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
	digitalWrite(CSPIN, LOW);
	for (byte i = 0; i < numOfDevices; i++){
	SPI.transfer16(x);
		}
	digitalWrite(CSPIN, HIGH);
	SPI.endTransaction();
}
void sendScreenMemory(){
	for (byte j = 0; j < COLHEIGHT; j++){
		for (byte i = 0; i < numOfDevices; i++){
			sendBuffer[i] = (j + 1) << 8 | screenMemory[i][j];
		}
		SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
		digitalWrite(CSPIN, LOW);
		for (byte k = 0; k < numOfDevices; k++){
			SPI.transfer16(sendBuffer[k]);
		}
		digitalWrite(CSPIN, HIGH);
		SPI.endTransaction();	
	}
}
void clearScreenMemory(){
//wipe the screen memory array
	for (byte i = 0; i < numOfDevices; i++){
		for (byte j = 0; j < ROWWIDTH; j++){
			screenMemory[i][j] = 0;
		}
	}
}
void checkJoyInput(){
	uint16_t x = analogRead(JOYXPIN);
	uint16_t y = analogRead(JOYYPIN);
	byte switchState = digitalRead(JOYSWITCH);
	if ( x < (joyX - joyOffset)){
		if (xScreenStart > 0){
			xScreenStart --;
		}
	}
	if (x > (joyX + joyOffset)){
		if ((xScreenStart + SCREENWIDTH) < LEVELWIDTH){
			xScreenStart ++;
		}
	}
	if ( y < (joyY - joyOffset)){
		if ((yScreenStart + SCREENHEIGHT) < LEVELHEIGHT){
			yScreenStart ++;
		}
	}
	if (y > (joyY + joyOffset)){
		if (yScreenStart != 0){
			yScreenStart --;
		}
	}
}
void plotPoint(byte x, byte y){
	byte moduleX = 0;
	byte moduleY = 0;
	byte colCounter = 0;
	byte rowCounter = 0;
//uses x and y values against a lookup array to determine which module to start on
	if (x >= (xScreenStart - ROWWIDTH) && x < xScreenStart){
		moduleX = 0;
		rowCounter = (xScreenStart - ROWWIDTH);
	}
	else if (x >= xScreenStart && x < (xScreenStart + ROWWIDTH)){
		moduleX = 1;
		rowCounter = xScreenStart;
	}
	else if (x >= (xScreenStart + ROWWIDTH) && x < (xScreenStart + MODULES2)){
		moduleX = 2;
		rowCounter = (xScreenStart + ROWWIDTH);
	}
	else if (x >= (xScreenStart + MODULES2) && x < (xScreenStart + MODULES3)){
		moduleX = 3;
		rowCounter = (xScreenStart + MODULES2);
	}
	else if (x >= (xScreenStart + MODULES3) && x < (xScreenStart + SCREENWIDTH)){
		moduleX = 4;
		rowCounter = (xScreenStart + MODULES3);
	}
	else {
		moduleX = 10;
	}
	if (y < yScreenStart){
		moduleY = 10;
	}	
	else if (y >= yScreenStart && y < (yScreenStart + COLHEIGHT)){
		moduleY = 3;
		colCounter = (yScreenStart + COLHEIGHT);
	}
	else if (y >= (yScreenStart + COLHEIGHT) && y < (yScreenStart + MODULES2)){
		moduleY = 2;
		colCounter = (yScreenStart + MODULES2);
	}
	else if (y >= (yScreenStart + MODULES2) && y < (yScreenStart + SCREENHEIGHT)){
		moduleY = 1;
		colCounter = (yScreenStart + SCREENHEIGHT); //changed from = screenheight only
	}
	else if (y >= (yScreenStart + SCREENHEIGHT) && y < (yScreenStart + SCREENHEIGHT + COLHEIGHT)){
		moduleY = 0;
		colCounter = (yScreenStart + SCREENHEIGHT + COLHEIGHT);
		}
	else {
		moduleY = 10;
	}
	//added the below so not to waste trying to plot something that wont show
	if (!(moduleX == 10 || moduleY == 10)){
		displayPlottedPoint(moduleLookupMatrix[moduleY][moduleX], x, y, colCounter, rowCounter);
	}		
}
void displayPlottedPoint(byte startModule, byte x, byte y, byte colCounter, byte rowCounter){
	bitWrite (screenMemory[startModule][8 - (colCounter - y)], 0 + (x - rowCounter), 1);
}
