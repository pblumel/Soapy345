/*
 * https://www.vyssotski.ch/BasicsOfInstrumentation/SpikeSorting/Design_of_FIR_Filters.pdf
 */

#ifndef FILT_H_
#define FILT_H_

#include <complex>

enum filterType {LPF, HPF};

template <typename T>
class Filter {
public:
	Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& cutoff_freq, const int& xlation_freq);
	Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& cutoff_freq);
	~Filter();
	T compute(const T& sample);
private:
	void computeLPFTaps();
	void computeHPFTaps();
	unsigned int num_taps {};
	unsigned int samp_rate {};
	unsigned int cutoff_freq {};
	float normalized_xlation_freq {0.0};
	unsigned int num_xlation_steps {1};
	unsigned int xlation_step {};
	float* taps {nullptr};
	T* shift_register {nullptr};
};


template <typename T>
Filter<T>::Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& cutoff_freq, const int& xlation_freq) {
	this->num_taps = num_taps;
	this->samp_rate = samp_rate;
	this->cutoff_freq = cutoff_freq;
	this->normalized_xlation_freq = (2*M_PI*xlation_freq) / samp_rate;

	taps = new float[this->num_taps];

	if (filt_t == LPF) {
		computeLPFTaps();
	} else if (filt_t == HPF) {
		computeHPFTaps();
	}

	shift_register = new T[this->num_taps];

	while ((abs(xlation_freq)*num_xlation_steps) % samp_rate != 0) {
		num_xlation_steps++;

		if (num_xlation_steps > 100) {
			throw 1;
		}
	}
}


template <typename T>
Filter<T>::Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& cutoff_freq) {
	this->num_taps = num_taps;
	this->samp_rate = samp_rate;
	this->cutoff_freq = cutoff_freq;

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


template <>
std::complex<float> Filter<std::complex<float>>::compute(const std::complex<float>& sample) {
	for (unsigned int i = num_taps-1; i > 0; i--) {
		shift_register[i] = shift_register[i-1];
	}
	shift_register[0] = sample;

	if (normalized_xlation_freq != 0.0) {
		shift_register[0] *= std::complex<float>(cos(xlation_step * normalized_xlation_freq), sin(xlation_step * normalized_xlation_freq));
		xlation_step = (xlation_step+1) % num_xlation_steps;
	}

	std::complex<float> sum(0.0, 0.0);
	for (unsigned int i = 0; i < num_taps; i++) {
		sum += shift_register[i] * taps[i];
	}

	return sum;
}

template <>
float Filter<float>::compute(const float& sample) {
	for (unsigned int i = num_taps-1; i > 0; i--) {
		shift_register[i] = shift_register[i-1];
	}
	shift_register[0] = sample;

	if (normalized_xlation_freq != 0.0) {
		shift_register[0] *= cos(xlation_step * normalized_xlation_freq);
		xlation_step = (xlation_step+1) % num_xlation_steps;
	}

	float sum = 0.0;
	for (unsigned int i = 0; i < num_taps; i++) {
		sum += shift_register[i] * taps[i];
	}

	return sum;
}


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

#endif /* FILT_H_ */
