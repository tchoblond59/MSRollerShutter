#include "Shutter.h"
#include "MyNodeDefinition.h"
#include <SPI.h>
#include <MySensors.h>

Shutter rollerShutter;

// the setup function runs once when you press reset or power the board
void setup() {
	rollerShutter.init();
  rollerShutter.sendShutterPosition();
}

// the loop function runs over and over again until power down or reset
void loop() {
	rollerShutter.update();
}


/* ======================================================================
Function: receive
Purpose : Mysensors incomming message
Comments:
====================================================================== */
void receive(const MyMessage &message) {

	if (message.isAck()) {}
	else {
		// Message received : Open shutters
		if (message.type == V_UP && message.sensor == CHILD_ID_ROLLERSHUTTER) {
			Serial.println("V_UP");
			rollerShutter.open();
		}

		// Message received : Close shutters
		if (message.type == V_DOWN && message.sensor == CHILD_ID_ROLLERSHUTTER) {
			Serial.println("V_DOWN");
			rollerShutter.close();
		}

		// Message received : Stop shutters motor
		if (message.type == V_STOP && message.sensor == CHILD_ID_ROLLERSHUTTER) {
			Serial.println("V_STOP");
			rollerShutter.stop();
		}

		// Message received : Set position of Rollershutter 
		if (message.type == V_PERCENTAGE && message.sensor == CHILD_ID_ROLLERSHUTTER) {
			Serial.print("SET PERCENT: ");
			Serial.println(message.getInt());
			rollerShutter.setPercent(message.getInt());
		}
		// Message received : Set position of Rollershutter 
		if (message.type == V_DIMMER && message.sensor == CHILD_ID_PERCENT) {
    
		}

		// Message received : Calibration requested 
		if (message.type == V_STATUS && message.sensor == CHILD_ID_AUTOCALIBRATION) {
			Serial.println("CALIBRATION");
			rollerShutter.calibration();
		}
		// Message received : Endstop command received 
		if (message.type == V_STATUS && message.sensor == CHILD_ID_ENDSTOP) {
			Serial.println("ENDSTOP");
			rollerShutter.endStop();
		}
		// Message received : temperature command received 
		if (message.type == V_TEMP && message.sensor == CHILD_ID_TEMPERATURE) {
			//Send Temperature
		}
	}
}
