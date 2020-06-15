#include "crc16.h"
#include <algorithm>


void CRC16::push(const unsigned long int& data, const unsigned int& num_bits, const bool& accumulate) {
	bool ov;

	// Process each bit of data from MSB to LSB
	for (int i = num_bits-1; i>=0; i--) {
		ov = calc_crc>>15;	// Save overflow bit
		calc_crc = (calc_crc<<1) | ((data>>i) & 0x1);	// Shift next data bit in

		// If overflow is true, xor CRC value with the polynomial
		if (ov) {
			calc_crc ^= polynomial;
		}
	}

	if (accumulate) {
		data_accum = (data_accum<<num_bits) | data; // Shift data into accumulator
	}
}


void CRC16::push(const unsigned long int& data, const unsigned int& num_bits) {
	push(data, num_bits, true);	// Public method always accumulates data bits
}


char16_t CRC16::getCRC() {
	auto temp_crc = calc_crc;
	push(0x00, 16, false);	// Don't accumulate these data bits, it would affect our getData() output
	std::swap(temp_crc, calc_crc);	// Preserve original value so we can call this function as many times as desired

	return temp_crc ^ fxor;	// Apply final XOR and return
}
