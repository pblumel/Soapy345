#include "ManchesterDecoder.h"


const bool ManchesterDecoder::decode(const bool& first_bit, const bool& second_bit) const {
	if (!first_bit & second_bit) {	// Manchester 01
		return true;
	} else if (first_bit & !second_bit) {	// Manchester 10
		return false;
	} else {
		throw INVALID_MANCHESTER;
	}
}


void ManchesterDecoder::add(const bool& symbol_state) {
	if (first_state == 0xFF) {	// Variable is unset,
								// this is the first state of the manchester sequence
		first_state = symbol_state;
	} else {
		try {
			// Shift the new decoded bit into the output data
			output_data = (output_data<<1) | decode(first_state, symbol_state);
		} catch (ManchesterExceptions& e) {
			/*	Invalid manchester sequence 00 or 11 was identified
			 *	It is known that this invalid sequence should not come from the transmitter now that
			 *		the receiver is synced to the transmitter via sync bits
			 *	A single pulse must have been confused for a double pulse, so ignore the more recent
			 *		state and try decoding first_state with the next state
			 *	This assumption will be verified by CRC later
			 */
		}

		output_size++;
		first_state = 0xFF;
	}
}


const unsigned int ManchesterDecoder::pop_all() {
	auto temp = output_data;
	this->clear();
	return temp;
}


void ManchesterDecoder::clear() {
	first_state = 0xFF;
	output_data = 0;
	output_size = 0;
}


const unsigned int ManchesterDecoder::size() const {
	return output_size;
}
