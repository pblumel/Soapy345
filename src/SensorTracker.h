#ifndef SRC_SENSORTRACKER_H_
#define SRC_SENSORTRACKER_H_


#include "SensorMessageReceiver.h"

#include <map>
#include <string>
#include <ctime>


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


struct sensorData {
	unsigned char sensor_state;
	unsigned int rx_msg_count {1};
	time_t last_seen;
};


class SensorTracker {
public:
	~SensorTracker();
	void push(const unsigned int& txid, const unsigned char& sensor_state);
private:
	std::map<unsigned int, sensorData> sensors;
};


#endif /* SRC_SENSORTRACKER_H_ */
