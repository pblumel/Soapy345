#ifndef CRC16_H_
#define CRC16_H_


class CRC16 {
public:
	void setPoly(const char16_t& polynomial) {this->polynomial = polynomial;};
	void setFinalXOR(const char16_t& fxor) {this->fxor = fxor;};
	void push(const unsigned int& data, const unsigned int& num_bits);
	unsigned int getData() const {return data_accum;};
	char16_t getCRC();	// Explicitly limit the output CRC to 16 bits
	void reset() {calc_crc = 0; data_accum = 0;};
private:
	void push(const unsigned int& data, const unsigned int& num_bits, const bool& accumulate);
	char16_t polynomial {0};
	char16_t fxor {0};	// Final XOR value
	unsigned int data_accum {0};	// Accumulator for received data
									// Currently implemented sensors
	char16_t calc_crc {0};
};


#endif /* CRC16_H_ */
