#ifndef Shutter_h
#define Shutter_h
#include <Arduino.h>
#include "MyNodeDefinition.h"
//#include <C:\Users\julie\OneDrive\Documents\Arduino\libraries\MySensors\core\MySensorsCore.h> //If someone can explain me why i have to do this shit...
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <timer.h>

// ********************* PIN/RELAYS DEFINES *******************************************
#define SHUTTER_UPDOWN_PIN        A1                    // Default pin for relay SPDT, you can change it with : initShutter(SHUTTER_POWER_PIN, SHUTTER_UPDOWN_PIN)
#define SHUTTER_POWER_PIN         A2                    // Default pin for relay SPST : Normally open, power off rollershutter motor

#define RELAY_ON                  1                     // ON state for power relay
#define RELAY_OFF                 0
#define RELAY_UP                  0                     // UP state for UPDOWN relay
#define RELAY_DOWN                1                     // UP state for UPDOWN relay

// ********************* EEPROM DEFINES **********************************************
#define EEPROM_OFFSET_SKETCH      512                   //  Address start where we store our data (before is mysensors stuff)
#define EEPROM_POSITION           EEPROM_OFFSET_SKETCH  //  Address of shutter last position
#define EEPROM_TRAVELUP           EEPROM_POSITION+2   //  Address of travel time for Up. 16bit value
#define EEPROM_TRAVELDOWN         EEPROM_POSITION+4   //  Address of travel time for Down. 16bit value

// ********************* CALIBRATION DEFINES *****************************************
#define START_POSITION            0                     // Shutter start position in % if no parameter in memory

#define TRAVELTIME_UP             40000                 // in ms, time measured between 0-100% for Up. it's initialization value. if autocalibration used, these will be overriden
#define TRAVELTIME_DOWN           40000                 // in ms, time measured between 0-100% for Down
#define TRAVELTIME_ERRORMS        1000                  // in ms,  max ms of error during travel, before reporting it as an error

#define CALIBRATION_SAMPLES       1                     // nb of round during calibration, then average
#define CALIBRATION_TIMEOUT       55000                 // in ms, timeout between 0 to 100% or vice versa, during calibration
#define REFRESH_POSITION_TIMEOUT  2000					// in ms time between each send of position when moving
#define AUTO_REFRESH              1800000         //in ms time between each send of position when its not moving keep high value.

enum RollerState
{
	STOPPED, UP, DOWN, ACTIVE_UP, ACTIVE_DOWN
};

class Shutter
{
public:
	Shutter();
	~Shutter();
	void init();
	void open();
	void close();
	void stop();
	void update();
	void endStop();
	void calibration();
	void setTravelTimeDown(unsigned long time);
	void setTravelTimeUp(unsigned long time);
	void setPercent(int percent);
	int getState();
	int getPosition();
	void debug();
	void writeEeprom(uint16_t pos, uint16_t value);
	uint16_t readEeprom(uint16_t pos);
	void refreshTemperature();
	void sendTemperature();
	void sendShutterPosition();

private:
	uint8_t m_state;
	uint16_t m_travel_time_up;
	uint16_t m_travel_time_down;
	int m_current_position;
	int m_last_position;
	bool m_calibration;
	unsigned long m_last_move;//Store last time shutter move in ms
	unsigned long m_last_send;//Store last time shutter move in ms
  unsigned long m_last_refresh_position;//Store last time we send the shutter position
	DallasTemperature m_ds18b20;
	OneWire *m_oneWire;
	float m_hilinkTemperature;
	unsigned long last_update_temperature;
	int m_percent_target;
	int m_analog_read;


	void touchLastMove();
	void storeTravelTime();
	unsigned long getCurrentTravelTime();
	uint16_t calculatePercent();
	void checkPosition();
	bool isTimeout();
	void mesureCurrent();

};

#endif
