#ifndef SRC_SENSORTRACKER_H_
#define SRC_SENSORTRACKER_H_


#include "SensorHistory.h"

#include <map>


class SensorTracker {
public:
	~SensorTracker();
	void push(std::shared_ptr<SensorMessage> sensor_message);
private:
	std::map<const unsigned long int, SensorHistory> sensors;
};


#endif /* SRC_SENSORTRACKER_H_ */
