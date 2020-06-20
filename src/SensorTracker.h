#ifndef SRC_SENSORTRACKER_H_
#define SRC_SENSORTRACKER_H_


#include "SensorHistory.h"

#include <map>


class SensorTracker {
public:
	~SensorTracker();
	void push(const unsigned long int& txid, const unsigned char& sensor_state);
private:
	std::map<const unsigned long int, SensorHistory> sensors;
};


#endif /* SRC_SENSORTRACKER_H_ */
