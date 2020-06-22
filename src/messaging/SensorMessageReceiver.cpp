#include "SensorMessageReceiver.h"
#include <iostream>
#include <iomanip>


SensorMessage* SensorMessageReceiver::push(const bool& sample) {
	if (sample == symbol_state) {	// Continuation of the same symbol state
		symbol_len_tracker++;
		return &sensor_message;
	}

	// New symbol incoming, process received symbol(s)
	// There can be two in manchester sequences LO HI HI LO or HI LO LO HI
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

				if (manchester_decoder.size() == CHANNEL_BITS) {
					auto channel = manchester_decoder.pop_all();
					if (channel < (0x1<<CHANNEL_BITS)) {	// Check bounds of valid channels
						sensor_message.idVendor(channel);	// Determine vendor based on known correlations of vendors to specific channels
					} else {
						std::cerr << channel << " is not a valid channel. Channels range from 0-" << (0x1<<CHANNEL_BITS)-1 << "." << std::endl;
						resetToSync();
						break;
					}

					// Configure CRC checker to settings used by the vendor
					crc16.reset();
					crc16.setPoly(0x8005);
					// DEBUG OUTPUT
					if (sensor_message.getVendor() == UNKNOWN) {
						std::cout << std::endl;
						std::cout << "No known vendor uses channel " << channel << ", may cause CRC failure." << std::endl;
					} else if (sensor_message.getVendor() == TWOGIG) {
						crc16.setPoly(0x8050);
					}
					crc16.push(channel, CHANNEL_BITS);

					message_state = TXID;
				}

				break;
			case TXID:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == TXID_BITS) {
					unsigned long int txid = manchester_decoder.pop_all();

					if (!txid) {	// Invalid TXID, reset and wait for next message
						resetToSync();
						break;
					}

					sensor_message.setTXID(txid);
					crc16.push(sensor_message.getTXID(), TXID_BITS);

					message_state = SENSOR_STATE;
				}

				break;
			case SENSOR_STATE:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == SENSOR_STATE_BITS) {
					sensor_message.setState(manchester_decoder.pop_all());
					crc16.push(sensor_message.getState(), SENSOR_STATE_BITS);

					message_state = CRC;
				}

				break;
			case CRC:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == CRC_BITS) {
					// Verify CRC
					if (manchester_decoder.pop_all() == crc16.getCRC()) {
						sensor_message.newMessage();	// Declare this message ready for processing
					} else {
						std::cout << "CRC FAIL FOR DATA 0x" << std::hex << crc16.getData() << std::dec << std::endl;
					}

					resetToSync();	// Message received, reset and wait for next message
				}

				break;
			default:
				std::cerr << "Decode345 unimplemented state machine state " << message_state << std::endl;
				throw 1;
		}
	}

	symbol_len_tracker.newSymbol();
	symbol_state = sample;

	return &sensor_message;
}
