#ifndef SENSORMESSAGERECEIVER_H_
#define SENSORMESSAGERECEIVER_H_

#include "SymbolLenTracker.h"
#include "ManchesterDecoder.h"
#include "crc16.h"
#include "SensorMessage.h"

#define SYNC_LEN 32-2	// Length of sync sequence with manchester encoding shortened due to two ignored sync bits
#define SYNC_LEVELS_FORMAT 0x55555556	// Sync sequence with manchester encoding
#define SYNC_LEVEL_MASK 0x3FFFFFFF	// First couple bits are inconsistent on some sensors

#define CHANNEL_BITS 4
#define TXID_BITS 20
#define SENSOR_STATE_BITS 8
#define CRC_BITS 16

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
