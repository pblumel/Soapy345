#ifndef SRC_SIGNALGENERATOR_H_
#define SRC_SIGNALGENERATOR_H_


#include <memory>
#include <complex>


enum SignalGeneratorError {INSUFFICIENT_SAMP_RATE, PHASE_DISCONTINUITY};


template <typename T>
class SignalGenerator {
public:
	SignalGenerator(const unsigned int& samp_rate, const int& freq);
	T sample();
private:
	T computeSamp(const float& normalized_freq, const unsigned int& step) const;
	unsigned int num_samples {1};
	std::unique_ptr<T[]> samples;	// Lookup table of signal samples
	unsigned int cur_step {};
};


template <typename T>
SignalGenerator<T>::SignalGenerator(const unsigned int& samp_rate, const int& freq) {
	// Abort if the requested frequency cannot be generated at the specified sample rate
	if (samp_rate/2 < abs(freq)) {
		throw INSUFFICIENT_SAMP_RATE;
	}

	// Determine number of samples to generate such that there are no discontinuities
	while ((abs(freq)*num_samples) % samp_rate != 0) {
		num_samples++;
	}

	if (num_samples == 0) {	// If there was overflow, no number of samples resulted in continuous phase
		throw PHASE_DISCONTINUITY;
	}

	// Allocate sample lookup table
	samples = std::make_unique<T[]>(num_samples);

	// Compute samples for lookup table
	float normalized_freq = (2*M_PI*freq) / samp_rate;
	for (unsigned int i = 0; i<num_samples; i++) {
		samples[i] = computeSamp(normalized_freq, i);
	}
}


template <>
std::complex<float> SignalGenerator<std::complex<float>>::computeSamp(const float& normalized_freq, const unsigned int& step) const {	// Get the appropriate mixer value for the current sample
	return std::complex<float>(cos(step * normalized_freq), sin(step * normalized_freq));
}


template <typename T>
T SignalGenerator<T>::computeSamp(const float& normalized_freq, const unsigned int& step) const {	// Get the appropriate mixer value for the current sample
	return cos(step * normalized_freq);
}


template <typename T>
T SignalGenerator<T>::sample() {
	cur_step = (cur_step+1) % num_samples;	// Prepare LO generator for next sample
	return samples[cur_step];
}


#endif /* SRC_SIGNALGENERATOR_H_ */
