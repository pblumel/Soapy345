#include "SensorTracker.h"

#include <iostream>
#include <iomanip>


SensorTracker::~SensorTracker() {	// Print summary of sensor activity
	std::cout << std::endl;
	std::cout << "## SENSOR SUMMARY ##" << std::endl;
	if (sensors.empty()) {
		std::cout << "NO SENSORS FOUND" << std::endl;
	} else {
		for (auto sensor = sensors.begin(); sensor != sensors.end(); sensors.erase(sensor++)) {	// Erase each sensor to trigger its destructor and print history summary
			std::cout << "TXID " << std::setfill('0') << std::setw(3) << (sensor->first/10000) << "-" << std::setw(4) << (sensor->first%10000) << " ";
		}
	}
}


void SensorTracker::push(std::shared_ptr<SensorMessage> sensor_message) {
	if (!sensor_message) {	// Verify that message exists
		return;
	}

	auto sensor = sensors.find(sensor_message->getTXID());
	if (sensor != sensors.end()) {	// Sensor detected previously, update it
		// Output debug info to console
		std::cout << std::endl;
		std::cout << "## UPDATE SENSOR " << std::setfill('0') << std::setw(3) << (sensor_message->getTXID()/10000) << "-" << std::setw(4) << (sensor_message->getTXID()%10000) << " ##" << std::endl;

		// Update sensor state
		sensor->second.updateSensorState(sensor_message->getState());
	} else {
		// Output debug info to console
		std::cout << std::endl;
		std::cout << "## ADD SENSOR " << std::setfill('0') << std::setw(3) << (sensor_message->getTXID()/10000) << "-" << std::setw(4) << (sensor_message->getTXID()%10000) << " ##" << std::endl;

		// Add new sensor, using std::move for the history so we can have different ~SensorHistory() behavior
		sensors.insert(std::pair<unsigned long int, SensorHistory>(sensor_message->getTXID(), std::move(SensorHistory(sensor_message->getState()))));
	}
}
