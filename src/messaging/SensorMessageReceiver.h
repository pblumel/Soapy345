#ifndef SENSORMESSAGERECEIVER_H_
#define SENSORMESSAGERECEIVER_H_

#include <memory>

#include "SymbolLenTracker.h"
#include "ManchesterDecoder.h"
#include "CRC16.h"

#define SYNC_LEN 32-2	// Length of sync sequence with manchester encoding shortened due to two ignored sync bits
#define SYNC_LEVELS_FORMAT 0x55555556	// Sync sequence with manchester encoding
#define SYNC_LEVEL_MASK 0x3FFFFFFF	// First couple bits are inconsistent on some sensors

#define CHANNEL_BITS 4
#define HEADER_BITS 20
#define INIT_HEADER_BITS 12
#define DEVID_BITS 8
#define STD_TXID_BITS 20
#define STD_TXID_MASK ((0x1<<STD_TXID_BITS)-1)
#define VIVINT_TXID_BITS 32
#define SENSOR_STATE_BITS 8
#define CRC_BITS 16


enum Vendor {UNKNOWN, HONEYWELL, TWOGIG, VIVINT, VIVINT_INIT};	// Vivint sensors send messages over a different channel when they first power on (INIT)
static const Vendor vendor_channel_map[0x1<<CHANNEL_BITS] = {	// A map of channel numbers to the associated vendors
		UNKNOWN, UNKNOWN, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, VIVINT,
		HONEYWELL, TWOGIG, TWOGIG, UNKNOWN, UNKNOWN, VIVINT_INIT, UNKNOWN, UNKNOWN};


class SensorMessage {
	friend class SensorMessageReceiver;
public:
	SensorMessage(const unsigned char& channel): vendor(vendor_channel_map[channel]) {};
	Vendor getVendor() const {return vendor;}
	unsigned long int getHeader() const {return header;};
	unsigned char getDEVID() const {return devid;};
	unsigned long int getTXID() const {return txid;};
	unsigned char getState() const {return sensor_state;};
private:
	const Vendor vendor;
	unsigned long int header {};
	unsigned char devid {};
	unsigned long int txid {};
	unsigned char sensor_state {};
};


enum messageState {SYNC, CHANNEL, HEADER, TXID, SENSOR_STATE, CRC, DEVID};


class SensorMessageReceiver {
public:
	SensorMessageReceiver(const float& est_symbol_len) :
		symbol_len_tracker(SymbolLenTracker<unsigned int>(SYNC_LEN-1, est_symbol_len)) {};
					// -1 slot is required because 11b in the manchester sync sequence only takes 1 slot
	std::shared_ptr<SensorMessage> push(const bool& sample);
private:
	void resetToSync() {message_state = SYNC; symbol_len_tracker.resetSyncAvg(); sensor_message.reset();};
	bool symbol_state {};
	unsigned int rx_sync_sr {};
	SymbolLenTracker<unsigned int> symbol_len_tracker;	// Shortened window size due to ignored sync bits
	ManchesterDecoder manchester_decoder;
	CRC16 crc16;
	messageState message_state {SYNC};
	std::shared_ptr<SensorMessage> sensor_message;
};

#endif /* SENSORMESSAGERECEIVER_H_ */
