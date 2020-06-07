#include "SensorHistory.h"

#include <iostream>
#include <ctime>


SensorHistory::SensorHistory(const Vendor& vendor, const unsigned char& sensor_state): vendor(vendor) {
	history.push_back(std::pair<unsigned char, time_t>(sensor_state, time(NULL)));

	// Output initial sensor state to the console
	std::cout << "SEEN " << rx_msg_count << " TIMES, NOW " << std::asctime(std::localtime(&history.back().second));
	unsigned int i = 1;
	for (auto bit_state : status_bit_states) {
		std::cout << bit_state[2] << ": "; // Output bit descriptor
		// Output state description
		std::cout << bit_state[sensor_state>>(SENSOR_STATE_BITS-i) & 0x1] << std::endl;

		i++;	// Next bit
	}
}


SensorHistory::SensorHistory(SensorHistory&& obj): vendor(obj.vendor), history(obj.history), rx_msg_count(obj.rx_msg_count) {
	obj.history.clear();
	obj.rx_msg_count = 0;
}


SensorHistory::~SensorHistory() {
	if (rx_msg_count) {
		std::cout << "SEEN " << rx_msg_count << " TIMES" << std::endl;
		for (auto entry = ++history.begin(); entry != history.end(); ++entry) {	// Print status diff starting with the second entry
			// Print timestamp for this entry
			std::cout << "# " << std::asctime(std::localtime(&entry->second));

			// Print status changes for entry
			printStatusDiff(entry);
		}

		std::cout << std::endl;
	}
}


void SensorHistory::updateSensorState(const unsigned char& sensor_state) {
	// Update counter and time for messages received from this sensor
	auto curTime = time(NULL);
	rx_msg_count++;
	std::cout << "SEEN " << rx_msg_count << " TIMES, NOW " << std::asctime(std::localtime(&curTime));

	// Skip updating sensor state if it has not changed
	if (history.back().first == sensor_state) {
		return;
	}

	// Add sensor status update to the history
	history.push_back(std::pair<unsigned char, time_t>(sensor_state, curTime));

	// Output changes to sensor state to the console
	printStatusDiff(--history.end());
}


/*
 * Given an iterator, prints a summary of differences between the current and previous states
 */
void SensorHistory::printStatusDiff(const std::vector<std::pair<unsigned char, time_t>>::iterator& entry) const {
	unsigned int i = 1;
	for (auto bit_state : status_bit_states) {
		bool former_bit = ((entry-1)->first>>(SENSOR_STATE_BITS-i)) & 0x1;
		bool cur_bit = (entry->first>>(SENSOR_STATE_BITS-i)) & 0x1;
		if (former_bit != cur_bit) {
			std::cout << bit_state[2] << ": ";	// Output bit descriptor
			// Output change description
			std::cout	<< bit_state[former_bit] << "->"
						<< bit_state[cur_bit] << std::endl;
		}

		i++;	// Next bit
	}
}
