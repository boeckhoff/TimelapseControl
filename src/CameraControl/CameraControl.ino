#include <usbhub.h>
#include <ptp.h>
#include <nkeventparser.h>
#include <nikon.h>
#include "qp_port.h"
#include <valuelist.h>
#include <nkvaluetitles.h>
#include <LiquidCrystal.h>
#include "DFR_Key.h"

bool forwardSlide = true;
bool forwardPan = true;
bool runningSlide = false;
bool runningPan = false;
bool waitingSlide = true;
bool waitingPan = true;
bool slide = true;
bool slideDone = false;
bool panDone = false;
bool slideEnabled = false;
bool panEnabled = false;
int slideSteps = 0;
int panSteps = 0;
unsigned long prevSlideDelMillies = 0;
unsigned long prevPanDelMilies = 0;
unsigned long prevSlideDurMillies = 0;
unsigned long prevPanDurMillies = 0;
unsigned long prevDisMillies = 0;
//-----------LCD Keypad-----------------------

//Pin assignments for SainSmart LCD Keypad Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 

DFR_Key keypad;

int localKey = 0;
int curKey = 1;
String keyString = "";
//---------------------------------------------


//-----------Analog read----------------------
unsigned int potPinDelay = 1;
unsigned int potPinDuration = 2;
unsigned int valSlideDelay = 0;
unsigned int valPanDelay = 0;
unsigned int valSlideDuration = 0;
unsigned int valPanDuration = 0;
unsigned int valNewSlideDelay = 0;
unsigned int valNewPanDelay = 0;
unsigned int valNewSlideDuration = 0;
unsigned int valNewPanDuration = 0;
unsigned int pinDelayRead = 0;
unsigned int pinDurationRead = 0;
float DELAYMIN = 200;
float DELAYMAX = 5000;
float DURATIONMIN = 1;
float DURATIONMAX = 100;
//---------------------------------------------
// 1 -Motor 1 Left
// 2 -Motor 1 Stop
// 3 -Motor 1 Right

// Motor 1

int dir1PinA = 0;
int dir2PinA = 1;
int speedPinA = 11; // Needs to be a PWM pin to be able to control motor speed

//  Motor 2

int dir1PinB = 2;
int dir2PinB = 3;
int speedPinB = 12; // Needs to be a PWM pin to be able to control motor speed

/*

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
        delay(1000);
        lcd.begin(16, 2);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Analog Read");
        delay(1000);
	
    }

    uint32_t time_now = millis();

    if (time_now >= nextPollTime)
    {
	nextPollTime = time_now + 300;

	NKEventParser  prs(&Dmp);
	Nk.EventCheck(&prs);
    }
}
*/


void setup() {
    // initialize serial communication @ 9600 baud:
    //Serial.begin(9600);

    //Define L298N Dual H-Bridge Motor Controller Pins
    pinMode(dir1PinA,OUTPUT);
    pinMode(dir2PinA,OUTPUT);
    pinMode(speedPinA,OUTPUT);


    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TimelapseControl");
    delay(1000);
    keypad.setRate(10);
}

void controlMotor(int inByte) {
    switch (inByte) {

        //______________Motor 1______________

        case '1': // Motor 1 Forward
            analogWrite(speedPinA, 255);//Sets 150 variable via PWM 
            digitalWrite(dir1PinA, HIGH);
            digitalWrite(dir2PinA, LOW);
            Serial.println("Motor 1 Forward"); // Prints out “Motor 1 Forward” on the serial monitor
            Serial.println("   "); // Creates a blank line printed on the serial monitor
            break;

        case '2': // Motor 1 Stop (Freespin)
            analogWrite(speedPinA, 0);
            digitalWrite(dir1PinA, LOW);
            digitalWrite(dir2PinA, LOW);
            Serial.println("Motor 1 Stop"); 
	    Serial.println("   ");
            break;

        case '3': // Motor 1 Reverse
            analogWrite(speedPinA, 255);
            digitalWrite(dir1PinA, LOW);
            digitalWrite(dir2PinA, HIGH);
            Serial.println("Motor 1 Reverse"); 
	    Serial.println("   ");
            break;

	//______________Motor 2______________
	
	case '4': // Motor 2 Forward
	    analogWrite(speedPinB, 255);
	    digitalWrite(dir1PinB, HIGH);
	    digitalWrite(dir2PinB, LOW);
	    Serial.println("Motor 2 Forward");
	    Serial.println("   ");
	    break;

	case '5': // Motor 1 Stop (Freespin)
	    analogWrite(speedPinB, 0);
	    digitalWrite(dir1PinB, LOW);
	    digitalWrite(dir2PinB, LOW);
	    Serial.println("Motor 2 Stop");
	    Serial.println("   ");
	    break;

	case '6': // Motor 2 Reverse
	    analogWrite(speedPinB, 255);
	    digitalWrite(dir1PinB, LOW);
	    digitalWrite(dir2PinB, HIGH);
	    Serial.println("Motor 2 Reverse");
	    Serial.println("   ");
	    break;

        default:
            // turn all the connections off if an unmapped key is pressed:
            for (int thisPin = 2; thisPin < 11; thisPin++) {
                digitalWrite(thisPin, LOW);
            }
    }

}

void changeStatus(int motor, bool enable) {
	if(motor == 0 && enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Slide Enabled");
		slideSteps = 0;
	}
	if(motor == 1 && enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Pan Enabled");
		panSteps = 0;
	}
	if(motor == 0 && !enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Slide Disabled");
		lcd.setCursor(2,1);
		lcd.print("Steps:");
		lcd.setCursor(8,1);
		lcd.print(slideSteps);
		slideSteps = 0;
	}
	if(motor == 1 && !enable) {
		lcd.clear();
		lcd.setCursor(2,0);
		lcd.print("Pan Disabled");
		lcd.setCursor(2,1);
		lcd.print("Steps:");
		lcd.setCursor(8,1);
		lcd.print(panSteps);
		panSteps = 0;
	}
	delay(1000);
}

void loop() {
    pinDelayRead = analogRead(potPinDelay); 
    pinDurationRead = analogRead(potPinDuration); 

    if(slide == true) {
		valNewSlideDelay = pinDelayRead*((DELAYMAX-DELAYMIN)/1023)+DELAYMIN;
		valNewSlideDuration = pinDurationRead*((DURATIONMAX-DURATIONMIN)/1023)+DURATIONMIN;
    }
    else {
		valNewPanDelay = pinDelayRead*((DELAYMAX-DELAYMIN)/1023)+DELAYMIN;
		valNewPanDuration = pinDurationRead*((DURATIONMAX-DURATIONMIN)/1023)+DURATIONMIN;
    }

    localKey = keypad.getKey();

	if(localKey != SAMPLE_WAIT) {
		if(localKey == 1 && curKey != 1 && slide) {
			forwardSlide = !forwardSlide;
			curKey = 1;
		}
		if(localKey == 1 && curKey != 1 && !slide) {
			forwardPan = !forwardPan;
			curKey = 1;
		}
		if(localKey == 2 && curKey != 2) {
			slide = !slide;	
			curKey = 2;
		}
		if(localKey == 5 && curKey != 5) {
			if(slide) {
				valSlideDelay = valNewSlideDelay;
				valSlideDuration = valNewSlideDuration;
			}
			else {
				valPanDelay = valNewPanDelay;
				valPanDuration = valNewPanDuration;
			}
		}
		if(localKey == 3 && curKey != 3) {
			slideEnabled = !slideEnabled;
			changeStatus(0, slideEnabled);
		}
		if(localKey == 4 && curKey != 4) {
			panEnabled = !panEnabled;
			changeStatus(1, panEnabled);
		}
		else
			curKey = localKey;
	}

    unsigned long currentMillis = millis();
    if(currentMillis - prevDisMillies > 200) {
        lcd.clear();
		lcd.setCursor(0, 0);
		if(slide == true) {
			lcd.print("Slide");
		}
		else {
			lcd.print("Pan");
		}
        lcd.setCursor(0, 1);
	if(slide == true) {
	    if(forwardSlide == true)
		lcd.print("->");
	    else
		lcd.print("<-");
	}
	else {
	    if(forwardPan == true)
		lcd.print("->");
	    else
		lcd.print("<-");
	}

	if(slide) {
	    lcd.setCursor(6, 0);
	    lcd.print(valNewSlideDuration);
	    lcd.setCursor(6, 1);
	    lcd.print(valSlideDuration);
	    lcd.setCursor(12, 0);
	    lcd.print(valNewSlideDelay);
	    lcd.setCursor(12, 1);
	    lcd.print(valSlideDelay);
	}
	else {
	    lcd.setCursor(6, 0);
	    lcd.print(valNewPanDuration);
	    lcd.setCursor(6, 1);
	    lcd.print(valPanDuration);
	    lcd.setCursor(12, 0);
	    lcd.print(valNewPanDelay);
	    lcd.setCursor(12, 1);
	    lcd.print(valPanDelay);
	}
        prevDisMillies = currentMillis;
    }
    Serial.println("doing task");

    //Usb.Task();
    Serial.println("done task");

    slideDone = false;
    panDone = false;

    if(waitingSlide == true && currentMillis - prevSlideDelMillies > valSlideDelay && slideEnabled) {
        if(forwardSlide == true)
            controlMotor('1');
        else
            controlMotor('3');
        prevSlideDurMillies = currentMillis;
        waitingSlide = false;
		slideDone = true;
		slideSteps++;
    }

    if(waitingSlide == false && currentMillis - prevSlideDurMillies > valSlideDuration && !slideDone) {
        controlMotor('2');
        prevSlideDelMillies = currentMillis;
        waitingSlide = true;
	slideDone = true;
    }

    if(waitingPan == true && currentMillis - prevPanDelMilies > valPanDelay && panEnabled) {
        if(forwardPan == true)
            controlMotor('4');
        else
            controlMotor('6');
        prevPanDurMillies = currentMillis;
        waitingPan = false;
		panDone = true;
		panSteps++;
    }

    if(waitingPan == false && currentMillis - prevPanDurMillies > valPanDuration && !panDone) {
        controlMotor('5');
        prevPanDelMilies = currentMillis;
        waitingPan = true;
	panDone = true;
    }
}
