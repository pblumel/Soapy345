#ifndef DECODE345_H_
#define DECODE345_H_

#define SYNC_LEN (32-2)	// Length of sync sequence with manchester encoding shortened due to two ignored sync bits
#define SYNC_LEVELS_FORMAT 0x55555556	// Sync sequence with manchester encoding
#define SYNC_LEVEL_MASK 0x3FFFFFFF	// First couple bits are inconsistent on some sensors

enum messageState {SYNC, VENDOR, HEADER, TXID, SENSOR_STATE_CRC};

class Decode345 {
public:
	Decode345(const unsigned int& est_symbol_len) : est_symbol_len(est_symbol_len), avg_symbol_len(est_symbol_len) {};
	void push(const float& sample);
private:
	unsigned int est_symbol_len;	// Constant for this execution cycle
	unsigned int avg_symbol_len;	// For now use the estimated length, will calculate per-sensor value
	//std::vector<unsigned int> symbol_len_tracker(SYNC_LEN);	// Shortened window size due to ignored sync bits
	float prev_sample {0.0};
	messageState message_state {SYNC};
	unsigned int rx_sync_sr {};
};

#endif /* DECODE345_H_ */
