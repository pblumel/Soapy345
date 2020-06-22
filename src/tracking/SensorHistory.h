#ifndef SRC_SENSORHISTORY_H_
#define SRC_SENSORHISTORY_H_


#include "../messaging/SensorMessageReceiver.h"

#include <string>
#include <vector>


static const std::string status_bit_states[SENSOR_STATE_BITS][3] = {
		{"CLOSED", "OPEN", "LOOP ONE"},	// False state, True state, Name
		{"NORM", "TRIP", "TAMPER"},
		{"CLOSED", "OPEN", "LOOP TWO"},
		{"FALSE", "TRUE", "UNKb4"},
		{"NORM", "LOW", "BATTERY"},
		{"EVENT", "PERIODIC", "TRIGGER"},
		{"FALSE", "TRUE", "PAIR"},
		{"FALSE", "TRUE", "UNKb0"}
};


class SensorHistory {
public:
	SensorHistory(const unsigned char& sensor_state);
	SensorHistory(SensorHistory&& obj);
	~SensorHistory();
	void updateSensorState(const unsigned char& sensor_state);
private:
	void printStatusDiff(const std::vector<std::pair<unsigned char, time_t>>::iterator& entry) const;
	std::vector<std::pair<unsigned char, time_t>> history;
	unsigned int rx_msg_count {1};
};


#endif /* SRC_SENSORHISTORY_H_ */
