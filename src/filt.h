#ifndef FILT_H_
#define FILT_H_

#include "SignalGenerator.h"

#include <memory>
#include <complex>

enum filterType {LPF, HPF};

template <typename T>
class Filter {
public:
	Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq, const int& xlation_freq = 0);
	T* compute(const T& sample);
private:
	void computeLPFTaps(const unsigned int& samp_rate, const unsigned int& cutoff_freq);
	void computeHPFTaps(const unsigned int& samp_rate, const unsigned int& cutoff_freq);
	std::unique_ptr<SignalGenerator<T>> localOscillator;
	const unsigned int num_taps;
	const unsigned int decimation;
	std::unique_ptr<float[]> taps;
	std::unique_ptr<T[]> shift_register;
	unsigned int decimation_counter {0};
	T filt_output;
};


template <typename T>
Filter<T>::Filter(const filterType& filt_t, const unsigned int& num_taps, const unsigned int& samp_rate, const unsigned int& decimation, const unsigned int& cutoff_freq, const int& xlation_freq)
	: num_taps(num_taps), decimation(decimation) {

	shift_register = std::make_unique<T[]>(num_taps);
	taps = std::make_unique<float[]>(num_taps);

	if (filt_t == LPF) {
		computeLPFTaps(samp_rate, cutoff_freq);
	} else if (filt_t == HPF) {
		computeHPFTaps(samp_rate, cutoff_freq);
	}

	if (xlation_freq != 0) {	// If frequency translation has been requested, create a local oscillator
		localOscillator = std::make_unique<SignalGenerator<T>>(samp_rate, xlation_freq);
	}
}


template <typename T>
T* Filter<T>::compute(const T& sample) {
	// Shift in new sample
	for (unsigned int i = num_taps-1; i > 0; i--) {
		shift_register[i] = shift_register[i-1];
	}
	shift_register[0] = sample;

	// If frequency translation is enabled, mix new sample with digital LO
	if (localOscillator) {
		shift_register[0] *= localOscillator->sample();
	}

	if (++decimation_counter != decimation) {	// If this sample is to be decimated, skip computing the sample
		return nullptr;
	}
	decimation_counter = 0;	// Reset decimator

	// Decimation completed, now compute and output a sample
	filt_output = 0;
	for (unsigned int i = 0; i < num_taps; i++) {
		filt_output += shift_register[i] * taps[i];
	}

	return &filt_output;
}


/*
 * https://www.vyssotski.ch/BasicsOfInstrumentation/SpikeSorting/Design_of_FIR_Filters.pdf
 */
template <typename T>
void Filter<T>::computeLPFTaps(const unsigned int& samp_rate, const unsigned int& cutoff_freq) {
	float normalized_cutoff_freq = (2*M_PI*cutoff_freq) / samp_rate;

	// Determine ideal filter coefficients for LPF
	for (unsigned int tap = 0; tap < num_taps; tap++) {
		float center_tap_offset = tap - (num_taps - 1.0)/2.0;
		if (center_tap_offset == 0.0) {
			taps[tap] = normalized_cutoff_freq/M_PI;
		} else {
			taps[tap] = sin(normalized_cutoff_freq * center_tap_offset)/
					(M_PI * center_tap_offset);
		}
	}
}


template <typename T>
void Filter<T>::computeHPFTaps(const unsigned int& samp_rate, const unsigned int& cutoff_freq) {
	float normalized_cutoff_freq = (2*M_PI*cutoff_freq) / samp_rate;

	// Determine ideal filter coefficients for HPF
	for (unsigned int tap = 0; tap < num_taps; tap++) {
		float center_tap_offset = tap - (num_taps - 1.0)/2.0;
		if (center_tap_offset == 0.0) {
			taps[tap] = 1.0-normalized_cutoff_freq/M_PI;
		} else {
			taps[tap] = -sin(normalized_cutoff_freq * center_tap_offset)/
					(M_PI * center_tap_offset);
		}
	}
}


#endif /* FILT_H_ */
