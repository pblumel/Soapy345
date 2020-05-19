#include "decode345.h"


void Decode345::push(const float& sample) {
	if (!symbolChanged(sample)) {
		symbol_len_tracker++;
		return;
	}

	// New symbol incoming, process received symbol(s)
	// There can be two in manchester sequences LO HI HI LO or HI LO LO HI
	bool symbol_state = prev_sample > 0.0;
	for (unsigned int i = symbol_len_tracker.getCurSymbolCount(); i>0; i--) {

		/*------STATE MACHINE------*/
		switch(message_state) {
			case SYNC:
				rx_sync_sr = (rx_sync_sr<<1) | symbol_state;	// Shift in a bit

				// Check sync levels
				if (!((rx_sync_sr ^ SYNC_LEVELS_FORMAT) & SYNC_LEVEL_MASK)) {
					// Determine the pulse width for this transmitter to read following bits
					// Last pulse(s) are double width due to 0xFFFE pattern, ignore them
					//avg_symbol_len = sum(self.symbol_len[:-2]) / (len(self.symbol_len)-2);
					symbol_len_tracker.computeSyncAvg();
					message_state = VENDOR;
				}

				break;
			case VENDOR:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case HEADER:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case SENSOR_STATE:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case TXID:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case CRC:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
				// TODO: When we find an abort condition, remember to reset average to estimated value
		}
	}

	symbol_len_tracker.newSymbol();
	prev_sample = sample;
}


const bool Decode345::symbolChanged(const float& sample) const {
	if (prev_sample > 0.0) {
		if (sample < 0.0) {
			return true;
		}
	} else if (prev_sample < 0.0) {
		if (sample > 0.0) {
			return true;
		}
	}

	// This is a continuation of the same symbol
	return false;
}
