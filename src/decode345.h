#ifndef DECODE345_H_
#define DECODE345_H_

#include "SymbolLenTracker.h"
#include "ManchesterDecoder.h"

#define SYNC_LEN 32-2	// Length of sync sequence with manchester encoding shortened due to two ignored sync bits
#define SYNC_LEVELS_FORMAT 0x55555556	// Sync sequence with manchester encoding
#define SYNC_LEVEL_MASK 0x3FFFFFFF	// First couple bits are inconsistent on some sensors

#define NUM_CHANNELS 32

enum messageState {SYNC, CHANNEL, TXID, SENSOR_STATE, CRC};
enum vendors {UNKNOWN, HONEYWELL, TWOGIG};


class Decode345 {
public:
	Decode345(const unsigned int& est_symbol_len) :
		symbol_len_tracker(SymbolLenTracker<unsigned int>(SYNC_LEN-1, est_symbol_len)) {};
					// -1 slot is required because 11b in the manchester sync sequence only takes 1 slot
	void push(const float& sample);
private:
	const bool symbolChanged(const float& sample) const;
	unsigned int rx_sync_sr {};
	SymbolLenTracker<unsigned int> symbol_len_tracker;	// Shortened window size due to ignored sync bits
	ManchesterDecoder manchester_decoder;
	float prev_sample {1.0};
	messageState message_state {SYNC};
	const vendors vendor_channel_map[NUM_CHANNELS] {	// A map of channel numbers to the associated vendors
		UNKNOWN, UNKNOWN, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
		HONEYWELL, TWOGIG, TWOGIG, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
		UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
		UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};
	vendors current_vendor {UNKNOWN};	// Vendor of sensor for current incoming message
};

#endif /* DECODE345_H_ */
