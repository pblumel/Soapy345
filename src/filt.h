#ifndef FILT_H_
#define FILT_H_

#include <complex>

enum filterType {LPF, HPF};

template <typename T>
class Filter {
public:
	Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq, const int& xlation_freq);
	Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq);
	~Filter();
	T* compute(const T& sample);
private:
	void computeLPFTaps();
	void computeHPFTaps();
	T getFreqXlationMult() const;
	const unsigned int num_taps {};
	const unsigned int samp_rate {};
	const unsigned int decimation {};
	const unsigned int cutoff_freq {};
	const float normalized_xlation_freq {0.0};
	unsigned int num_xlation_steps {1};
	unsigned int xlation_step {};
	float* taps {nullptr};
	T* shift_register {nullptr};
	unsigned int decimation_counter {0};
	T filt_output;
};


template <typename T>
Filter<T>::Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq, const int& xlation_freq)
	: num_taps(num_taps), samp_rate(samp_rate), decimation(decimation), cutoff_freq(cutoff_freq), normalized_xlation_freq((2*M_PI*xlation_freq) / samp_rate) {

	taps = new float[this->num_taps];

	if (filt_t == LPF) {
		computeLPFTaps();
	} else if (filt_t == HPF) {
		computeHPFTaps();
	}

	shift_register = new T[this->num_taps];

	// Determine number of LO samples to generate for frequency xlation such that there are no discontinuities
	while ((abs(xlation_freq)*num_xlation_steps) % samp_rate != 0) {
		num_xlation_steps++;
	}
}


template <typename T>
Filter<T>::Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq)
	: num_taps(num_taps), samp_rate(samp_rate), decimation(decimation), cutoff_freq(cutoff_freq) {

	taps = new float[this->num_taps];

	if (filt_t == LPF) {
		computeLPFTaps();
	} else if (filt_t == HPF) {
		computeHPFTaps();
	}

	shift_register = new T[this->num_taps];
}


template <typename T>
Filter<T>::~Filter() {
 delete[] taps;
 delete[] shift_register;
}


template <typename T>
T* Filter<T>::compute(const T& sample) {
	// Shift in new sample
	for (unsigned int i = num_taps-1; i > 0; i--) {
		shift_register[i] = shift_register[i-1];
	}
	shift_register[0] = sample;

	// If frequency translation is enabled, mix new sample with digital LO
	if (normalized_xlation_freq != 0.0) {
		shift_register[0] *= getFreqXlationMult();
		xlation_step = (xlation_step+1) % num_xlation_steps;	// Prepare LO generator for next sample
	}

	decimation_counter++;
	if (decimation_counter == decimation) {	// Compute filtered output sample
		decimation_counter = 0;

		filt_output = 0;
		for (unsigned int i = 0; i < num_taps; i++) {
			filt_output += shift_register[i] * taps[i];
		}

		return &filt_output;
	}

	return nullptr;
}


/*
 * https://www.vyssotski.ch/BasicsOfInstrumentation/SpikeSorting/Design_of_FIR_Filters.pdf
 */
template <typename T>
void Filter<T>::computeLPFTaps() {
	double normalized_cutoff_freq = (2*M_PI*cutoff_freq) / samp_rate;
	double delayed_impulse_response = 0;

	// Determine ideal filter coefficients for LPF
	for (unsigned int tap = 0; tap < num_taps; tap++) {
		delayed_impulse_response = tap - (num_taps - 1.0)/2.0;
		if (delayed_impulse_response == 0.0) {
			taps[tap] = normalized_cutoff_freq/M_PI;
		} else {
			taps[tap] = sin(normalized_cutoff_freq * delayed_impulse_response)/
					(M_PI * delayed_impulse_response);
		}
	}
}


template <typename T>
void Filter<T>::computeHPFTaps() {
	double normalized_cutoff_freq = (2*M_PI*cutoff_freq) / samp_rate;
	double delayed_impulse_response = 0;

	// Determine ideal filter coefficients for HPF
	for (unsigned int tap = 0; tap < num_taps; tap++) {
		delayed_impulse_response = tap - (num_taps - 1.0)/2.0;
		if (delayed_impulse_response == 0.0) {
			taps[tap] = 1.0-normalized_cutoff_freq/M_PI;
		} else {
			taps[tap] = -sin(normalized_cutoff_freq * delayed_impulse_response)/
					(M_PI * delayed_impulse_response);
		}
	}
}


template <>
std::complex<float> Filter<std::complex<float>>::getFreqXlationMult() const {	// Get the appropriate mixer value for the current sample
	return std::complex<float>(cos(xlation_step * normalized_xlation_freq), sin(xlation_step * normalized_xlation_freq));
}


template <typename T>
T Filter<T>::getFreqXlationMult() const {	// Get the appropriate mixer value for the current sample
	return cos(xlation_step * normalized_xlation_freq);
}

#endif /* FILT_H_ */
