#include "decode345.h"
#include <iostream>


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
					symbol_len_tracker.computeSyncAvg();
					message_state = CHANNEL;
				}

				break;
			case CHANNEL:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == 4) {	// Channel is 4 unencoded bits
					auto channel = manchester_decoder.pop_all();
					if (channel < NUM_CHANNELS) {	// Prevent exceeding array bounds
						current_vendor = vendor_channel_map[channel];
					}

					// DEBUG OUTPUT
					if (current_vendor == UNKNOWN) {
						std::cout << "No known vendor uses channel " << channel << ", may cause CRC failure." << std::endl;
					} else if (current_vendor == HONEYWELL) {
						std::cout << "Vendor HONEYWELL" << std::endl;
					} else if (current_vendor == TWOGIG) {
						std::cout << "Vendor 2GIG" << std::endl;
					}

					message_state = TXID;
				}

				break;
			case TXID:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case SENSOR_STATE:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
			case CRC:
				message_state = SYNC;
				symbol_len_tracker.resetSyncAvg();
				break;
				// TODO: When we find an abort condition, remember to reset average to estimated value
			default:
				std::cerr << "Decode345 unimplemented state machine state " << message_state << std::endl;
				throw 1;
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
