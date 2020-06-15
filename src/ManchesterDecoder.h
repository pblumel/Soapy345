#ifndef MANCHESTERDECODER_H_
#define MANCHESTERDECODER_H_


enum ManchesterExceptions {INVALID_MANCHESTER};


class ManchesterDecoder {
public:
	const bool decode(const bool& first_bit, const bool& second_bit) const;
	void add(const bool& symbol_state);
	const unsigned long int pop_all();
	void clear();
	const unsigned int size() const;
private:
	unsigned char first_state {0xFF};	// 0xFF is an indicator that the variable is unset
	unsigned long int output_data {};
	unsigned int output_size {};
};


#endif /* MANCHESTERDECODER_H_ */
