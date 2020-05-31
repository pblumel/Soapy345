#ifndef SRC_SENSORMESSAGE_H_
#define SRC_SENSORMESSAGE_H_


#define NUM_CHANNELS 16


enum vendors {UNKNOWN, HONEYWELL, TWOGIG};
static const vendors vendor_channel_map[NUM_CHANNELS] = {	// A map of channel numbers to the associated vendors
		UNKNOWN, UNKNOWN, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
		HONEYWELL, TWOGIG, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};


class SensorMessage {
public:
	void setProcessed() {processed_state = true;};
	bool isProcessed() const {return processed_state;};
	void newMessage() {processed_state = false;};
	void idVendor(const unsigned char& channel) {vendor = vendor_channel_map[channel];};
	vendors getVendor() const {return vendor;}
	void setTXID(const unsigned int& txid) {this->txid = txid;};
	unsigned int getTXID() const {return txid;};
	void setState(const unsigned char& sensor_state) {this->sensor_state = sensor_state;};
	unsigned char getState() const {return sensor_state;};
private:
	bool processed_state {true};
	vendors vendor {UNKNOWN};
	unsigned int txid {};
	unsigned char sensor_state {};
};


#endif /* SRC_SENSORMESSAGE_H_ */
