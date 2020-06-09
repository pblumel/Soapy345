#include "SensorTracker.h"

#include <iostream>
#include <iomanip>


SensorTracker::~SensorTracker() {	// Print summary of sensor activity
	std::cout << std::endl;
	std::cout << "## SENSOR SUMMARY ##" << std::endl;
	if (sensors.empty()) {
		std::cout << "NO SENSORS FOUND" << std::endl;
	} else {
		for (auto sensor : sensors) {
			std::cout << "TXID " << std::setfill('0') << std::setw(3) << (sensor.first/10000) << "-" << std::setw(4) << (sensor.first%10000)
					<< " SEEN " << sensor.second.rx_msg_count << " TIMES, LAST AT " << std::asctime(std::localtime(&sensor.second.last_seen));
		}
	}
}


void SensorTracker::push(const unsigned int& txid, const unsigned char& sensor_state) {
	auto sensor = sensors.find(txid);
	if (sensor != sensors.end()) {	// Sensor detected previously
		// Update counter and time for messages received from this sensor
		sensor->second.last_seen = time(NULL);
		sensor->second.rx_msg_count++;

		// Skip updating sensor state if it has not changed
		if (sensor->second.sensor_state == sensor_state) {
			return;
		}

		// Output debug info to console
		std::cout << std::endl;
		std::cout << "## UPDATE TXID " << std::setfill('0') << std::setw(3) << (txid/10000) << "-" << std::setw(4) << (txid%10000) << " ##" << std::endl;
		std::cout << "SEEN " << sensor->second.rx_msg_count << " TIMES, NOW " << std::asctime(std::localtime(&sensor->second.last_seen));

		// Output changes to sensor state to the console
		unsigned int i = 1;
		for (auto bit_state : status_bit_states) {
			bool former_bit = (sensor->second.sensor_state>>(SENSOR_STATE_BITS-i)) & 0x1;
			bool cur_bit = (sensor_state>>(SENSOR_STATE_BITS-i)) & 0x1;
			if (former_bit != cur_bit) {
				std::cout << bit_state[2] << ": ";	// Output bit descriptor
				// Output change description
				std::cout	<< bit_state[former_bit] << "->"
							<< bit_state[cur_bit] << std::endl;
			}

			i++;
		}

		// Update sensor state
		sensor->second.sensor_state = sensor_state;
	} else {
		// Add new sensor
		sensorData temp;
		temp.sensor_state = sensor_state;
		temp.last_seen = time(NULL);
		sensors.insert(std::pair<unsigned int, sensorData>(txid, temp));

		// Output debug info to console
		std::cout << std::endl;
		std::cout << "## ADD TXID " << std::setfill('0') << std::setw(3) << (txid/10000) << "-" << std::setw(4) << (txid%10000) << " ##" << std::endl;

		// Output initial sensor state to the console
		unsigned int i = 1;
		for (auto bit_state : status_bit_states) {
			std::cout << bit_state[2] << ": "; // Output bit descriptor
			// Output state description
			std::cout << bit_state[sensor_state>>(SENSOR_STATE_BITS-i) & 0x1] << std::endl;

			i++;
		}
	}
}
