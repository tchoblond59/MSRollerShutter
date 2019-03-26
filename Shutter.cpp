#include "Shutter.h"
#include <EEPROM.h>
Shutter::Shutter()
{

}

Shutter::~Shutter()
{
}

void Shutter::init()
{
	// Set relay pins in output mode
	// Make sure relays are off when starting up
	digitalWrite(SHUTTER_POWER_PIN, RELAY_OFF);
	pinMode(SHUTTER_POWER_PIN, OUTPUT);
	digitalWrite(SHUTTER_UPDOWN_PIN, RELAY_OFF);
	pinMode(SHUTTER_UPDOWN_PIN, OUTPUT);

	pinMode(DEBUG_LED, OUTPUT);
	digitalWrite(DEBUG_LED, LOW);

	m_oneWire = new OneWire(DS18B20_PIN);
	m_ds18b20.setOneWire(m_oneWire);

	m_current_position = readEeprom(EEPROM_POSITION);
	m_last_position = m_current_position;
	m_travel_time_up = readEeprom(EEPROM_TRAVELUP);
	m_travel_time_down = readEeprom(EEPROM_TRAVELDOWN);
	m_state = RollerState::STOPPED;
	m_calibration = false;
	m_last_move = millis();
	last_update_temperature = m_last_move;
	m_last_send = m_last_move;
	m_hilinkTemperature = 0;
	m_percent_target = -1;
	m_analog_read = 0;
	debug();
}

void Shutter::open()
{
	//stop();
	delay(25);
	m_state = RollerState::ACTIVE_UP;
	touchLastMove();
	// 0=RELAY_UP, 1= RELAY_DOWN
	digitalWrite(SHUTTER_UPDOWN_PIN, RELAY_UP);
	// Power ON motor 
	digitalWrite(SHUTTER_POWER_PIN, RELAY_ON);
	digitalWrite(DEBUG_LED, HIGH);
}

void Shutter::close()
{
	//stop();
	delay(25);
	m_state = RollerState::ACTIVE_DOWN;
	touchLastMove();
	// 0=RELAY_UP, 1= RELAY_DOWN
	digitalWrite(SHUTTER_UPDOWN_PIN, RELAY_DOWN);
	// Power ON motor 
	digitalWrite(SHUTTER_POWER_PIN, RELAY_ON);
	digitalWrite(DEBUG_LED, HIGH);
}

void Shutter::stop()
{
	// Power OFF motor 
	digitalWrite(SHUTTER_POWER_PIN, RELAY_OFF);
	digitalWrite(DEBUG_LED, LOW);
	m_state = RollerState::STOPPED;
	Serial.println("Motor Stopped");
	m_last_position = m_current_position;
	writeEeprom(EEPROM_POSITION, m_last_position);
	MyMessage msgShutterPosition(CHILD_ID_ROLLERSHUTTER, V_PERCENTAGE);    // Message for % shutter
	msgShutterPosition.set(m_current_position);
	send(msgShutterPosition);
}

void Shutter::update()
{
	if (m_calibration)//Calibration in progress shutter is opening
	{
		if (isTimeout())
		{
			m_calibration = false;
			stop();
		}
		if (m_state == RollerState::UP)//Wait for shutter to be completely open
		{
			m_travel_time_up = millis() - m_last_move;
			close();
		}
		else if (m_state == RollerState::DOWN)//Wait for close
		{
			m_travel_time_down = millis() - m_last_move;
			stop();
			m_calibration = false;
			storeTravelTime();
		}
	}
	else if(m_state == RollerState::ACTIVE_DOWN || m_state == RollerState::ACTIVE_UP)
	{
		mesureCurrent();
		Serial.print("Analog read: ");
		Serial.println(m_analog_read);

		calculatePercent();
		checkPosition();
		if (millis() - m_last_send >= REFRESH_POSITION_TIMEOUT)
		{
			m_last_send = millis();
			sendShutterPosition();
		}
	}
	refreshTemperature();
}

void Shutter::endStop()
{
	if (m_state == RollerState::ACTIVE_DOWN)
	{
		m_state = RollerState::DOWN;
		m_current_position = 100; //100% is completely down
	}
	else if (m_state == RollerState::ACTIVE_UP)
	{
		m_state = RollerState::UP;
		m_current_position = 0; //0% is completely closed
	}
}

void Shutter::calibration()
{
	m_calibration = true;
	open();
}

void Shutter::setTravelTimeDown(unsigned long time)
{
	m_travel_time_down = time;
}

void Shutter::setTravelTimeUp(unsigned long time)
{
	m_travel_time_up = time;
}

void Shutter::setPercent(int percent)
{
	m_percent_target = percent;
	if (m_percent_target < m_current_position)
	{
		open();
	}
	else if(m_percent_target > m_current_position)
	{
		close();
	}
}

int Shutter::getState()
{
	return m_state;
}

int Shutter::getPosition()
{
	return m_current_position;
}

void Shutter::debug()
{
	Serial.println(F("========= Node EEprom loading =========="));
	Serial.print("Current Position    : "); Serial.println(m_current_position);
	Serial.print("Up Traveltime Ref   : "); Serial.println(m_travel_time_up);
	Serial.print("Down Traveltime Ref : "); Serial.println(m_travel_time_down);
	Serial.print("Timeout Traveltime Ref : "); Serial.println(CALIBRATION_TIMEOUT);
	Serial.print("Traveltime now "); Serial.println(getCurrentTravelTime());
	Serial.print("Traveltime up % "); Serial.println(getCurrentTravelTime() * 100 / m_travel_time_up);
	Serial.print("Traveltime down % "); Serial.println(getCurrentTravelTime() * 100 / m_travel_time_up);
	Serial.print("Last position"); Serial.println(m_last_position);
	
	Serial.println(F("========================================"));
}

void Shutter::writeEeprom(uint16_t pos, uint16_t value)
{
	// function for saving the values to the internal EEPROM
	// value = the value to be stored (as int)
	// pos = the first byte position to store the value in
	// only two bytes can be stored with this function (max 32.767)
	EEPROM.update(pos, ((uint16_t)value >> 8));
	pos++;
	EEPROM.update(pos, (value & 0xff));
}

uint16_t Shutter::readEeprom(uint16_t pos)
{
	// function for reading the values from the internal EEPROM
	// pos = the first byte position to read the value from
	uint16_t hiByte;
	uint16_t loByte;

	hiByte = EEPROM.read(pos) << 8;
	pos++;
	loByte = EEPROM.read(pos);
	return (hiByte | loByte);
}

void Shutter::refreshTemperature()
{
	if (millis() - last_update_temperature > 30000)
	{	
		last_update_temperature = millis();
		float oldTemp = m_hilinkTemperature;
		m_ds18b20.requestTemperatures();
		m_hilinkTemperature = m_ds18b20.getTempCByIndex(0);
		// Check if reading was successful
		if (m_hilinkTemperature != DEVICE_DISCONNECTED_C)
		{
			Serial.println("READ TEMPERATURE");
			if (abs(m_hilinkTemperature - oldTemp) >= TEMP_TRANSMIT_THRESHOLD)
			{
				sendTemperature();				
			}
		}
		else
		{
			Serial.println("Error: Could not read temperature data");
		}
	}
}

void Shutter::sendTemperature()
{
	MyMessage msgTemperature(CHILD_ID_TEMPERATURE, V_TEMP);                // Message for onboard hilink temperature
	msgTemperature.set(m_hilinkTemperature, 2);
	send(msgTemperature);
}

void Shutter::touchLastMove()
{
	m_last_move = millis();
}

void Shutter::storeTravelTime()
{
	writeEeprom(EEPROM_TRAVELDOWN, m_travel_time_down);
	writeEeprom(EEPROM_TRAVELUP, m_travel_time_up);
}

unsigned long Shutter::getCurrentTravelTime()
{
	return millis() - m_last_move;
}

uint16_t Shutter::calculatePercent()
{
	if (m_state == RollerState::ACTIVE_UP)
	{
		m_current_position = m_last_position - getCurrentTravelTime() * 100 / m_travel_time_up;
		//Serial.println("CALCUL OUVERTURE EN COURS");
		//Serial.println(m_current_position);
	}
	else if (m_state == RollerState::ACTIVE_DOWN)
	{
		m_current_position = m_last_position + getCurrentTravelTime() * 100 / m_travel_time_down;
		//Serial.println("CALCUL FERMETURE EN COURS");
		//Serial.println(m_current_position);
	}
	return m_current_position;
}

void Shutter::checkPosition()
{
	if (m_current_position <= 0 && m_state == RollerState::ACTIVE_UP)
	{
		Serial.println("POSITION OUVERTE ATTEINTE");
		stop();
		m_state == RollerState::UP;
		m_current_position = 0;
		m_last_position = m_current_position;
	}
	else if(m_current_position >= 100 && m_state == RollerState::ACTIVE_DOWN)
	{
		Serial.println("POSITION FERMEE ATTEINTE");
		stop();
		m_state == RollerState::DOWN;
		m_current_position = 100;
		m_last_position = m_current_position;
	}
	else if (m_percent_target != -1)//User wants precise position
	{
		if (m_state == RollerState::ACTIVE_UP && m_current_position <= m_percent_target)
		{
			stop();
			m_state == RollerState::STOPPED;
			m_last_position = m_current_position;
			m_percent_target = -1;
		}
		else if (m_state == RollerState::ACTIVE_DOWN && m_current_position >= m_percent_target)
		{
			stop();
			m_state == RollerState::STOPPED;
			m_last_position = m_current_position;
			m_percent_target = -1;
		}
	}
}

bool Shutter::isTimeout()
{
	if (getCurrentTravelTime() > CALIBRATION_TIMEOUT)
		return true;
	else
		return false;
}

void Shutter::mesureCurrent()
{
	m_analog_read = analogRead(ACS712_SENSOR);
}

void Shutter::sendShutterPosition()
{
	MyMessage msgShutterPosition(CHILD_ID_ROLLERSHUTTER, V_PERCENTAGE);    // Message for % shutter
	msgShutterPosition.set(getPosition());
	send(msgShutterPosition);
}