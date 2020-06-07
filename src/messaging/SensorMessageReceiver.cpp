#include "SensorMessageReceiver.h"

#include <iostream>
#include <iomanip>


std::shared_ptr<SensorMessage> SensorMessageReceiver::push(const bool& sample) {
	if (sample == symbol_state) {	// Continuation of the same symbol state
		symbol_len_tracker++;
		return std::shared_ptr<SensorMessage>(nullptr);
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
						sensor_message = std::make_shared<SensorMessage>(channel);	// Determine vendor based on known correlations of vendors to specific channels
					} else {
						std::cerr << channel << " is not a valid channel. Channels range from 0-" << (0x1<<CHANNEL_BITS)-1 << "." << std::endl;
						resetToSync();
						break;
					}

					// Configure CRC checker to settings used by the vendor
					crc16.reset();
					switch (sensor_message->getVendor()) {
					case TWOGIG:
						crc16.setPoly(0x8050);
						message_state = TXID;
						break;
					case VIVINT:
						// TODO: Determine real CRC parameters used
						crc16.setPoly(0x8005);
						message_state = HEADER;
						break;
					case UNKNOWN:
						std::cout << std::endl;
						std::cout << "No known vendor uses channel " << channel << ", may cause CRC failure." << std::endl;
					default:
						crc16.setPoly(0x8005);	// Default Honeywell parameters
						message_state = TXID;
					}
					crc16.push(channel, CHANNEL_BITS);
				}

				break;
			case HEADER:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == HEADER_BITS) {
					sensor_message->header = manchester_decoder.pop_all();
					crc16.push(sensor_message->getHeader(), HEADER_BITS);

					message_state = SENSOR_STATE;
				}

				break;
			case TXID:
				manchester_decoder.add(symbol_state);

				// Wait for all TXID bits
				if (manchester_decoder.size() < STD_TXID_BITS) {
					break;
				} else if (sensor_message->getVendor() == VIVINT) {
					if (manchester_decoder.size() != VIVINT_TXID_BITS) {
						break;
					}

					// The vendor is VIVINT and all expected TXID bits have been received
					message_state = CRC;
				} else {
					// The vendor is not VIVINT and all expected TXID bits have been received
					message_state = SENSOR_STATE;
				}

				{
					auto num_txid_bits = manchester_decoder.size();
					sensor_message->txid = manchester_decoder.pop_all();

					if (!sensor_message->getTXID()) {	// Invalid TXID, reset and wait for next message
						resetToSync();
						break;
					}

					crc16.push(sensor_message->getTXID(), num_txid_bits);
				}

				break;
			case SENSOR_STATE:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == SENSOR_STATE_BITS) {
					sensor_message->sensor_state = manchester_decoder.pop_all();
					crc16.push(sensor_message->getState(), SENSOR_STATE_BITS);

					if (sensor_message->getVendor() == VIVINT) {
						message_state = TXID;
					} else {
						message_state = CRC;
					}
				}

				break;
			case CRC:
				manchester_decoder.add(symbol_state);

				if (manchester_decoder.size() == CRC_BITS) {
					// Verify CRC
					auto rx_crc = manchester_decoder.pop_all();
					if (sensor_message->getVendor() == VIVINT) {	// Temporarily bypass CRC for Vivint
																// sensors, CRC parameters are unknown
																// TODO: Enable CRC check for Vivint
						std::cout << "VIVINT SENSOR MESSAGE: 0x";
						std::cout << std::setfill('0') << std::setw((CHANNEL_BITS+HEADER_BITS+VIVINT_TXID_BITS+SENSOR_STATE_BITS)/4)
									<< std::hex << crc16.getData();
						std::cout << " CRC 0x" << std::setw(CRC_BITS/4) << rx_crc << std::dec << std::endl;
					} else if (rx_crc == crc16.getCRC()) {
						symbol_len_tracker.newSymbol();
						symbol_state = sample;

						auto output_message = sensor_message;	// Save this good message before resetting
						resetToSync();

						return output_message;	// Declare this message ready for processing
												// By returning here, a bit may be lost from the next message if it
												// follows directly behind this one, but the likelihood is negligible
					} else {
						std::cout << "CRC FAIL FOR DATA 0x" << std::hex << crc16.getData() << std::dec << std::endl;
					}

					resetToSync();	// Reset and wait for next message
				}

				break;
			default:
				std::cerr << "Decode345 unimplemented state machine state " << message_state << std::endl;
				throw 1;
		}
	}

	symbol_len_tracker.newSymbol();
	symbol_state = sample;

	return std::shared_ptr<SensorMessage>(nullptr);
}
