#ifndef SENSORMESSAGERECEIVER_H_
#define SENSORMESSAGERECEIVER_H_

#include "SymbolLenTracker.h"
#include "ManchesterDecoder.h"
#include "crc16.h"

#define SYNC_LEN 32-2	// Length of sync sequence with manchester encoding shortened due to two ignored sync bits
#define SYNC_LEVELS_FORMAT 0x55555556	// Sync sequence with manchester encoding
#define SYNC_LEVEL_MASK 0x3FFFFFFF	// First couple bits are inconsistent on some sensors

#define CHANNEL_BITS 4
#define TXID_BITS 20
#define SENSOR_STATE_BITS 8
#define CRC_BITS 16


enum Vendor {UNKNOWN, HONEYWELL, TWOGIG};
static const Vendor vendor_channel_map[0x1<<CHANNEL_BITS] = {	// A map of channel numbers to the associated vendors
		UNKNOWN, UNKNOWN, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
		HONEYWELL, TWOGIG, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};


class SensorMessage {
public:
	void setProcessed() {processed_state = true;};
	bool isProcessed() const {return processed_state;};
	void newMessage() {processed_state = false;};
	void idVendor(const unsigned char& channel) {vendor = vendor_channel_map[channel];};
	Vendor getVendor() const {return vendor;}
	void setTXID(const unsigned int& txid) {this->txid = txid;};
	unsigned int getTXID() const {return txid;};
	void setState(const unsigned char& sensor_state) {this->sensor_state = sensor_state;};
	unsigned char getState() const {return sensor_state;};
private:
	bool processed_state {true};
	Vendor vendor {UNKNOWN};
	unsigned int txid {};
	unsigned char sensor_state {};
};


enum messageState {SYNC, CHANNEL, TXID, SENSOR_STATE, CRC};


class SensorMessageReceiver {
public:
	SensorMessageReceiver(const unsigned int& est_symbol_len) :
		symbol_len_tracker(SymbolLenTracker<unsigned int>(SYNC_LEN-1, est_symbol_len)) {};
					// -1 slot is required because 11b in the manchester sync sequence only takes 1 slot
	SensorMessage* push(const bool& sample);
private:
	bool symbol_state {};
	unsigned int rx_sync_sr {};
	SymbolLenTracker<unsigned int> symbol_len_tracker;	// Shortened window size due to ignored sync bits
	ManchesterDecoder manchester_decoder;
	CRC16 crc16;
	messageState message_state {SYNC};
	SensorMessage sensor_message;
};

#endif /* SENSORMESSAGERECEIVER_H_ */
