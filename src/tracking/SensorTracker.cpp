#include "SensorTracker.h"

#include <iostream>
#include <iomanip>


void printTXID(const Vendor& vendor, const unsigned long int& txid) {
	if ((vendor == VIVINT_INIT) | (vendor == VIVINT)) {
		std::cout << std::setfill('0') << std::setw(4) << (txid>>STD_TXID_BITS) << "-";	// Print extra Vivint-specific leading TXID field
	}

	// Strip extra Vivint TXID field and print remaining fields as normal
	std::cout << std::setfill('0') << std::setw(3) << ((txid & STD_TXID_MASK)/10000);
	std::cout << "-" << std::setw(4) << ((txid & STD_TXID_MASK)%10000);
}


void printDevice(const unsigned char& devid) {
	switch (devid) {
	case 0x18:
		std::cout << "DW11";
		break;
	case 0x0f:
		std::cout << "DW21R";
		break;
	case 0x14:
		std::cout << "GB2";
		break;
	case 0x0a:
		std::cout << "PIR2";
		break;
	default:
		std::cout << "UNKNOWN";
	}
}


SensorTracker::~SensorTracker() {	// Print summary of sensor activity
	std::cout << std::endl;
	std::cout << "## SENSOR SUMMARY ##" << std::endl;
	if (sensors.empty()) {
		std::cout << "NO SENSORS FOUND" << std::endl;
	} else {
		for (auto sensor = sensors.begin(); sensor != sensors.end(); sensors.erase(sensor++)) {	// Erase each sensor to trigger its destructor and print history summary
			std::cout << "TXID ";
			printTXID(sensor->second.vendor, sensor->first);
			std::cout << " ";
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
		std::cout << std::endl << "## UPDATE SENSOR ";
		printTXID(sensor_message->getVendor(), sensor_message->getTXID());
		std::cout << " ##" << std::endl;

		// Update sensor state
		sensor->second.updateSensorState(sensor_message->getState());
	} else {
		// Output debug info to console
		std::cout << std::endl << "## ADD ";
		printDevice(sensor_message->getDEVID());
		std::cout << " SENSOR ";
		printTXID(sensor_message->getVendor(), sensor_message->getTXID());
		std::cout << " ##" << std::endl;

		// Add new sensor, using std::move for the history so we can have different ~SensorHistory() behavior
		sensors.insert(std::pair<unsigned long int, SensorHistory>(sensor_message->getTXID(), std::move(SensorHistory(sensor_message->getVendor(), sensor_message->getState()))));
	}
}
