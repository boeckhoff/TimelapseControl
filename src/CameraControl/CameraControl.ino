#include <usbhub.h>
#include <ptp.h>
#include <nkeventparser.h>
#include <nikon.h>
#include "qp_port.h"
#include <valuelist.h>
#include <nkvaluetitles.h>
#include <LiquidCrystal.h>
#include "DFR_Key.h"
#include <Thread.h>
#include <ThreadController.h>

bool forwardSlide = true;
bool serial = false;
int slideSteps = 0;

//-----------LCD Keypad-----------------------
//Pin assignments for SainSmart LCD Keypad Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 

DFR_Key keypad;

int localKey = 0;
int curKey = 1;
String keyString = "";
//---------------------------------------------

//Motor pins
int dir1PinA = 1;
int dir2PinA = 0;

//-----------Analog read-----------------------
unsigned int potPinDelay = 2;
unsigned int potPinDuration = 1;
unsigned int valSlideDelay = 0;
unsigned int valSlideDuration = 0;
unsigned int valNewSlideDelay = 0;
unsigned int valNewSlideDuration = 0;
unsigned int pinDelayRead = 0;
unsigned int pinDurationRead = 0;
float DELAYMIN = 200;
float DELAYMAX = 5000;
float DURATIONMIN = 1;
float DURATIONMAX = 300;
//---------------------------------------------

class NKEventDump : public NKEventHandlers
{
	public:
		virtual void OnEvent(const NKEvent *evt);
};

void NKEventDump::OnEvent(const NKEvent *evt)
{
	switch (evt->eventCode)
	{
		case PTP_EC_DevicePropChanged:
			//Fifo.Push(evt->wParam1);
			PrintHex<uint16_t>(evt->wParam1, 0x80);
			if(serial)
				Serial.println("");
			break;
		case PTP_EC_ObjectAdded:
			{
				//            PTPObjInfoParser     inf;
				//            Nk.GetObjectInfo(evt->dwParam, &inf);
			}
			break;
	};
}

class CamStateHandlers : public PTPStateHandlers
{
	enum CamStates { stInitial, stDisconnected, stConnected };
	CamStates stateConnected;

	uint32_t nextPollTime;

	public:
	CamStateHandlers() : stateConnected(stInitial), nextPollTime(0)
	{
	};

	virtual void OnDeviceDisconnectedState(PTP *ptp);
	virtual void OnDeviceInitializedState(PTP *ptp);
};

CamStateHandlers  CamStates;

USB                 Usb;
NikonDSLR           Nk(&Usb, &CamStates);
NKEventDump         Dmp;

QEvent            evtTick;

void CamStateHandlers::OnDeviceDisconnectedState(PTP *ptp)
{
	if (stateConnected == stConnected || stateConnected == stInitial)
	{
		stateConnected = stDisconnected;
		E_Notify(PSTR("Camera disconnected.\r\n"),0x80);

	}
}

void CamStateHandlers::OnDeviceInitializedState(PTP *ptp)
{
	if (stateConnected == stDisconnected || stateConnected == stInitial)
	{
		stateConnected = stConnected;
		E_Notify(PSTR("Camera connected.\r\n"),0x80);
		ptp->SetDevicePropValue(0xD1B0, (uint8_t)1);
		Nk.InitiateCapture();
	}

	uint32_t time_now = millis();

	if (time_now >= nextPollTime)
	{
		nextPollTime = time_now + 300;

		NKEventParser  prs(&Dmp);
		Nk.EventCheck(&prs);
	}
}


void controlMotor(int inByte) {
	switch (inByte) {

		//______________Motor 1______________

		case '1': // Motor 1 Forward
			digitalWrite(dir1PinA, HIGH);
			digitalWrite(dir2PinA, LOW);
			if(serial)
				Serial.println("Motor 1 Forward");
			break;

		case '2': // Motor 1 Stop (Freespin)
			digitalWrite(dir1PinA, LOW);
			digitalWrite(dir2PinA, LOW);
			if(serial)
				Serial.println("Motor 1 Stop"); 
			break;

		case '3': // Motor 1 Reverse
			digitalWrite(dir1PinA, LOW);
			digitalWrite(dir2PinA, HIGH);
			if(serial)
				Serial.println("Motor 1 Reverse"); 
			break;

		default:
			// turn all the connections off if an unmapped key is pressed:
			for (int thisPin = 2; thisPin < 11; thisPin++) {
				digitalWrite(thisPin, LOW);
			}
	}
}

void changeStatus(int motor, bool enable) {
	noInterrupts();
	if(enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Slide Enabled");
		slideSteps = 0;
	}
	if(!enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Slide Disabled");
		lcd.setCursor(2,1);
		lcd.print("Steps:");
		lcd.setCursor(8,1);
		lcd.print(slideSteps);
		slideSteps = 0;
	}
	interrupts();
}

// Threads Initialization
Thread slideStartThread = Thread();
Thread slideStopThread = Thread();
Thread slideThread = Thread();
Thread cameraThread = Thread();
Thread displayThread = Thread();
Thread pinReadThread = Thread();
Thread userInputThread = Thread();
ThreadController controller = ThreadController();


void slide() {
	if(forwardSlide == true)
		controlMotor('1');
	else
		controlMotor('3');
	slideSteps++;

	if(serial)
		Serial.println("slide_b: " + String(millis()));

	uint32_t now = millis();		

	while((millis() - now) < valSlideDuration){};
	/*volatile int i,j,z;
	for (i=0; i<900; i++){
		 for (j=0; j<900; j++){
			  z=i*10;}}
			  */
	if(serial)
		Serial.println("slide_e: " + String(millis()));

	controlMotor('2');
	slideThread.setInterval(valSlideDelay);
}

void startSlide() {
	noInterrupts();	
	if(serial)
		Serial.println("startSlide_b: " + String(millis()));
	if(forwardSlide == true)
		controlMotor('1');
	else
		controlMotor('3');
	slideSteps++;

	slideStopThread.setInterval(valSlideDuration);
	//cameraThread.setInterval(valSlideDelay - 100);

	slideStartThread.enabled = false;
	slideStopThread.enabled = true;
	//cameraThread.enabled = true;

	if(serial) {
		Serial.println("SlideDur: " + String(valSlideDuration));
		Serial.println("startSlide_e: " + String(millis()));
	}

	interrupts();	
}

void stopSlide() {
	noInterrupts();	
	if(serial)
		Serial.println("stopSlide_b: " + String(millis()));
	controlMotor('2');

	slideStartThread.setInterval(valSlideDelay);

	slideStartThread.enabled = true;
	slideStopThread.enabled = false;

	if(serial)
		Serial.println("stopSlide_e: " + String(millis()));
	interrupts();	
}

void shutter() {
	if(serial)
		Serial.println("shutter_b: " + String(millis()));
	Nk.InitiateCapture();
	//Usb.Task();
	if(serial)
		Serial.println("shutter_e: " + String(millis()));
}

void updateDisplay() {
	if(serial)
		Serial.println("updateDisplay_b: " + String(millis()));
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Slide");
	lcd.setCursor(0, 1);

	if(forwardSlide == true)
		lcd.print("->");
	else
		lcd.print("<-");

	lcd.setCursor(6, 0);
	lcd.print(valNewSlideDuration);
	lcd.setCursor(6, 1);
	lcd.print(valSlideDuration);
	lcd.setCursor(12, 0);
	lcd.print(valNewSlideDelay);
	lcd.setCursor(12, 1);
	lcd.print(valSlideDelay);
	if(serial)
		Serial.println("updateDisplay_e: " + String(millis()));
}

void readPins() {
	if(serial)
		Serial.println("readPins_b: " + String(millis()));
	pinDelayRead = analogRead(potPinDelay); 
	pinDurationRead = analogRead(potPinDuration); 

	valNewSlideDelay = pinDelayRead*((DELAYMAX-DELAYMIN)/1023)+DELAYMIN;
	valNewSlideDuration = pinDurationRead*((DURATIONMAX-DURATIONMIN)/1023)+DURATIONMIN;
	if(serial)
		Serial.println("readPins_e: " + String(millis()));
}

void userInput() {
	if(serial)
		Serial.println("userInput_b: " + String(millis()));
	localKey = keypad.getKey();

	if(localKey != SAMPLE_WAIT) {
		if(localKey == 1 && curKey != 1) {
			//Change Direction
			forwardSlide = !forwardSlide;
			curKey = 1;
		}
		if(localKey == 5 && curKey != 5) {
			//Commit Delay & Duration
			valSlideDelay = valNewSlideDelay;
			valSlideDuration = valNewSlideDuration;
		}
		if(localKey == 3 && curKey != 3) {
			//Enable/Disable Slide
			slideThread.enabled = !slideThread.enabled;
			//slideStartThread.enabled = !slideStartThread.enabled;
			changeStatus(0, slideThread.enabled);
		}
		else
			curKey = localKey;
	}
	if(serial)
		Serial.println("userInput_e: " + String(millis()));
}


void setup() {
	if(serial)
		Serial.begin(9600);
	Usb.Init();

	//Define L298N Dual H-Bridge Motor Controller Pins
	pinMode(dir1PinA,OUTPUT);
	pinMode(dir2PinA,OUTPUT);
	digitalWrite(10, HIGH);
	digitalWrite(0, LOW);
	digitalWrite(1, LOW);

	//controlMotor('2');

	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("TimelapseControl");
	delay(1000);
	keypad.setRate(10);

	slideStartThread.enabled = false;
	slideStopThread.enabled = false;
	slideThread.enabled = false;
	cameraThread.enabled = false;

	//slideStartThread.onRun(startSlide);
	//slideStopThread.onRun(stopSlide);
	slideThread.onRun(slide);
	cameraThread.onRun(shutter);
	displayThread.onRun(updateDisplay);
	pinReadThread.onRun(readPins);
	userInputThread.onRun(userInput);

	pinReadThread.setInterval(200);
	displayThread.setInterval(200);
	userInputThread.setInterval(300);
	cameraThread.setInterval(10000);

	//controller.add(&slideStartThread);
	//controller.add(&slideStopThread);
	controller.add(&slideThread);
	controller.add(&cameraThread);
	controller.add(&displayThread);
	controller.add(&pinReadThread);
	controller.add(&userInputThread);
	cameraThread.enabled = true;
}

void loop() {
	controller.run();
	Usb.Task();
}
