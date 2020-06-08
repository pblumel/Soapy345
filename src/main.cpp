#include <csignal>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>

#include "filt.h"
#include "decode345.h"
#include "SensorTracker.h"

#include <iostream>
#include <iomanip>
#include <complex>


#define RX_BUF_SIZE 1024

#define SAMP_RATE 200e3
#define SIG_FREQ 345006e3
#define TUNE_FREQ 344936e3	// Space the DC spike well away from the signal
#define SENSOR_BW 40e3
#define SPS 3

#define IF_FILT_ORDER 50
#define IF_DECIMATION 4

#define BB_FILT_CUTOFF 1225
#define BB_FILT_ORDER 25	// Must be odd for type 1 filter
#define BB_DECIMATION 2


using std::cout;
using std::cerr;
using std::endl;
using std::complex;

bool not_terminated = true;

void signalHandler(int signum) {
	not_terminated = false;
}


int main() {
	// register signal SIGINT and signal handler  
	signal(SIGINT, signalHandler);
	



	/* -------------------------------------------
	 * ----------QUERY CONNECTED DEVICES----------
	   -------------------------------------------*/

	// Get list of connected devices
	SoapySDR::KwargsList devices = SoapySDR::Device::enumerate();

	if (devices.empty()) {
		cout << "No devices found, please connect a device and try again." << endl;
		return EXIT_FAILURE;
	}

	// Output list of connected devices and their properties
	for (const auto& device : devices) {
		cout << "Found ";
		for (const auto& prop : device) {
			cout << prop.first << " = " << prop.second << endl;
		}
		cout << endl;
	}




	/* -------------------------------------------
	 * -----------QUERY SELECTED DEVICE-----------
	   -------------------------------------------*/

	// Make device zero
	SoapySDR::Device *sdr = SoapySDR::Device::make(devices[0]);

	if (sdr == nullptr) {
		cerr << "SoapySDR::Device::make failed" << endl;
		return EXIT_FAILURE;
	}

	cout << endl << "HARDWARE OPTIONS" << endl;

	//	List available antenna ports on device
	cout << "Rx antennas: ";
	for (const auto& antenna : sdr->listAntennas(SOAPY_SDR_RX, 0)) {
		cout << antenna << ", ";
	}
	cout << endl;

	//	List available gains on device
	cout << "Rx gains: ";
	for (const auto& gain : sdr->listGains(SOAPY_SDR_RX, 0)) {
		cout << gain << ", ";
	}
	cout << endl;

	//	List frequency ranges of device
	cout << "Rx freq ranges: ";
	for (const auto& freq_range : sdr->getFrequencyRange(SOAPY_SDR_RX, 0)) {
		cout << "[" << freq_range.minimum() << " Hz -> " << freq_range.maximum() << " Hz], ";
	}
	cout << endl;




	/* -----------------------------------------
	 * --------CONFIGURE SELECTED DEVICE--------
	   -----------------------------------------*/

	cout << endl << "HARDWARE CONFIGURATION" << endl;

	// Disable automatic gain control
	sdr->setGainMode(SOAPY_SDR_RX, 0, false);

	// Configure manual gain stages
	for (const auto& gain : sdr->listGains(SOAPY_SDR_RX, 0)) {
		std::cout << gain;

		if (gain == "LNA") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 20);
		} else if (gain == "VGA") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 20);
		} else if (gain == "AMP") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 0);
		}
		std::cout << " gain: " << std::setfill('0') << std::setw(2) << sdr->getGain(SOAPY_SDR_RX, 0, gain) << " dB" << std::endl;
	}
	
	// Configure sample rate
	sdr->setSampleRate(SOAPY_SDR_RX, 0, SAMP_RATE);
	cout << "Sample rate: " << sdr->getSampleRate(SOAPY_SDR_RX, 0) << " samples/second" << endl;

	// Configure frequency
	sdr->setFrequency(SOAPY_SDR_RX, 0, TUNE_FREQ);
	cout << "Freqency: " << sdr->getFrequency(SOAPY_SDR_RX, 0) << " Hz" << endl;




	/* -----------------------------------------
	 * ---------CONFIGURE SAMPLE STREAM---------
	   -----------------------------------------*/

	// Setup a stream (complex floats)
	SoapySDR::Stream *rx_stream = sdr->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
	if(rx_stream == nullptr) {
		cerr << "Sample stream creation failed" << endl;
		SoapySDR::Device::unmake(sdr);
		return EXIT_FAILURE;
	}
	sdr->activateStream(rx_stream, 0, 0, 0);

	// Create a re-usable buffer for rx samples
	complex<float> buff[RX_BUF_SIZE];




	/* -----------------------------------------
	 * ---------BEGIN SIGNAL PROCESSING---------
	   -----------------------------------------*/

	// Configure IF (complex LPF with frequency Xlation) and BB (HPF) filters
	Filter<complex<float>> IFfilter(LPF, IF_FILT_ORDER, sdr->getSampleRate(SOAPY_SDR_RX, 0), IF_DECIMATION, SENSOR_BW/2,
			sdr->getFrequency(SOAPY_SDR_RX, 0)-SIG_FREQ);	// Includes frequency translation.
															// The HackRF One samples have significant DC noise, so
															// tuning the hardware to some offset frequency and translating
															// the signal back to FFT center in the digital domain greatly
															// improves SNR.
	Filter<float> BBfilter(HPF, BB_FILT_ORDER, sdr->getSampleRate(SOAPY_SDR_RX, 0)/IF_DECIMATION, BB_DECIMATION, BB_FILT_CUTOFF);

	// Create decoder for 345 data with estimated sample per symbol value for finding sync bits.
	// The decoder computes more accurate SPS estimations per-message using sync bits
	// for overall SPS accuracy throughout the message.
	Decode345 decoder(SPS);
	SensorMessage* sensor_message = nullptr;
	SensorTracker sensor_tracker;

	// Loop through sample buffers until sample stream is terminated
	while (not_terminated) {
		void *buffs[] = {buff};
		int flags;
		long long time_ns;

		// Read samples into buffer
		int ret = sdr->readStream(rx_stream, buffs, RX_BUF_SIZE, flags, time_ns, 1e5);
		
		if (ret < 0) {	// Report stream errors
			if (ret == SOAPY_SDR_OVERFLOW) {
				cout << "-----OVERFLOW-----" << endl;
			} else if (ret != SOAPY_SDR_TIMEOUT) {
				cout << "Unknown readStream return code " << ret << endl;
			}
		} else {	// If sample stream is intact, process samples in buffer
			for (auto const& sample : buff) {
				// Apply frequency translation and lowpass filter to IF
				auto filt_samp = IFfilter.compute(sample);
				if (filt_samp) {	// If a sample has been output after decimation
					// Compute magnitude (BB) and apply highpass filter to center signal at zero.
					// This allows BB pulse widths to be determined by tracking zero-crossings.
					auto BB_filt_samp = BBfilter.compute(filt_samp->real()*filt_samp->real() + filt_samp->imag()*filt_samp->imag());
					if (BB_filt_samp) {	// If a sample has been output after decimation
						sensor_message = decoder.push(*BB_filt_samp);

						// Process this sensor message if it hasn't been already
						if (!sensor_message->isProcessed()) {
							sensor_tracker.push(sensor_message->getTXID(), sensor_message->getState());
							sensor_message->setProcessed();
						}
					}
				}
			}
		}
	}
	
	// Shutdown the stream
	sdr->deactivateStream(rx_stream, 0, 0);	//stop streaming
	sdr->closeStream(rx_stream);

	// Cleanup device handle
	SoapySDR::Device::unmake(sdr);

	return EXIT_SUCCESS;
}
