#include <csignal>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>

#include "dsp/Filter.h"
#include "messaging/SensorMessageReceiver.h"
#include "tracking/SensorTracker.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <complex>
#include <cstring>


#define RX_BUF_SIZE 1024

#define SAMP_RATE 200e3
#define SIG_FREQ 345006e3
#define TUNE_FREQ_OFFSET -70e3	// Space the DC spike well away from the signal
#define SENSOR_BW 40e3
#define PULSE_WIDTH 130e-6	// 130 us

#define IF_FILT_ORDER 11
#define IF_FILT_DECIMATION 4

#define BB_DC_FILT_CUTOFF 1530
#define BB_DC_FILT_ORDER 15	// Must be odd for type 1 filter
#define BB_DC_FILT_DECIMATION 1

#define BB_LP_FILT_CUTOFF 9800
#define BB_LP_FILT_ORDER 5
#define BB_LP_FILT_DECIMATION 2

using std::cout;
using std::cerr;
using std::endl;

using std::strcmp;

using std::ifstream;

using std::complex;


bool not_terminated = true;


void printHelp(char* command) {
	cerr << "Usage:" << endl << command << " [INPUT FILE]" << endl << endl;
	cerr << "INPUT FILE: Optional stream of CF32 (complex floating point) values, compatible with GNU Radio File Source/Sink blocks. "
			"When not specified, SDR hardware is used by default." << endl;
}


void signalHandler(int signum) {
	not_terminated = false;
}


void processSample(const complex<float>& sample, Filter<complex<float>>& IFfilter, Filter<float>& BB_DC_remove, Filter<float>& BB_LP_filter, SensorMessageReceiver& message_receiver, SensorTracker& sensor_tracker) {
	// Apply frequency translation and lowpass filter to IF
	auto filt_samp = IFfilter.compute(sample);
	if (!filt_samp) {	// If this sample is decimated
		return;
	}

	// Compute magnitude (BB) and apply highpass filter to center signal at zero.
	// This allows BB pulse widths to be determined by tracking zero-crossings.
	auto BB_DC_remove_samp = BB_DC_remove.compute(
			filt_samp->real()*filt_samp->real() +	// Real^2
			filt_samp->imag()*filt_samp->imag());	// Imag^2
	if (!BB_DC_remove_samp) {	// If this sample is decimated
		return;
	}

	// Use a lowpass filter to clean up signal and reduce undesireable zero crossings
	auto BB_LP_filt_samp = BB_LP_filter.compute(*BB_DC_remove_samp);
	if (!BB_LP_filt_samp) {	// If this sample is decimated
		return;
	}

	// Process sensor messages if they exist
	sensor_tracker.push(	// SensorTracker gets messages from SensorMessageReceiver
			message_receiver.push(	// Extract messages from square wave signal
					!signbit(*BB_LP_filt_samp)));	// Float to square wave conversion
													// 0 when <0, 1 when >=0
}


int main(int argc, char* argv[]) {

	/* --------------------------------------------
	 * -----------IDENTIFY SAMPLE SOURCE-----------
	   --------------------------------------------*/

	ifstream inputFile;
	SoapySDR::KwargsList devices;
	if (argc > 1) {	// If one or more parameters were passed in
		// Check for help request
		for (signed int i = 1; i<argc; i++) {
			if (!strcmp(argv[i], "-h") | !strcmp(argv[i], "--help")) {
				printHelp(argv[0]);

				return EXIT_SUCCESS;
			}
		}

		// If no help request received, try to open a user-selected input file
		inputFile.open(argv[1], std::ios::in | std::ios::binary);
		if (!inputFile) {
			cerr << "\"" << argv[1] << "\"" << " is not a valid input file." << endl << endl;
			printHelp(argv[0]);

			return EXIT_FAILURE;
		}
	} else {	// Default to SDR hardware sample source and verify that one is available
		// Get list of connected devices
		devices = SoapySDR::Device::enumerate();

		if (devices.empty()) {
			cout << "No devices found, please connect a device and try again." << endl;
			return EXIT_FAILURE;
		}
	}




	/* --------------------------------------------
	 * ------CREATE SIGNAL PROCESSING OBJECTS------
	   --------------------------------------------*/

	// Configure IF (complex LPF with frequency Xlation) and BB (HPF) filters
	Filter<complex<float>> IFfilter(LPF, IF_FILT_ORDER, SAMP_RATE, IF_FILT_DECIMATION, SENSOR_BW/2,
			TUNE_FREQ_OFFSET);	// Includes frequency translation.
															// The HackRF One samples have significant DC noise, so
															// tuning the hardware to some offset frequency and translating
															// the signal back to FFT center in the digital domain greatly
															// improves SNR.
	Filter<float> BB_DC_remove(HPF, BB_DC_FILT_ORDER, SAMP_RATE/IF_FILT_DECIMATION, BB_DC_FILT_DECIMATION, BB_DC_FILT_CUTOFF);
	Filter<float> BB_LP_filter(LPF, BB_LP_FILT_ORDER, SAMP_RATE/(IF_FILT_DECIMATION*BB_DC_FILT_DECIMATION), BB_LP_FILT_DECIMATION, BB_LP_FILT_CUTOFF);

	// Create decoder for 345 data with estimated sample per symbol value for finding sync bits.
	// The decoder computes more accurate SPS estimations per-message using sync bits
	// for overall SPS accuracy throughout the message.
	SensorMessageReceiver message_receiver(PULSE_WIDTH*(SAMP_RATE/(IF_FILT_DECIMATION*BB_DC_FILT_DECIMATION*BB_LP_FILT_DECIMATION)));
	SensorTracker sensor_tracker;




	/* -------------------------------------------
	 * ----INITIATE THE SELECTED SAMPLE SOURCE----
	   -------------------------------------------*/

	if (inputFile.is_open()) {	// If file source was selected, fully process the file
		complex<float> sample;
		while (inputFile.read((char *)&sample, sizeof(complex<float>))) {
			processSample(sample, IFfilter, BB_DC_remove, BB_LP_filter, message_receiver, sensor_tracker);
		}

		return EXIT_SUCCESS;
	}	// Else default to SDR source

	// Output list of connected devices and their properties
	for (const auto& device : devices) {
		cout << "Found ";
		for (const auto& prop : device) {
			cout << prop.first << " = " << prop.second << endl;
		}
		cout << endl;
	}

	// Make device zero
	SoapySDR::Device *sdr = SoapySDR::Device::make(devices[0]);
	if (sdr == nullptr) {
		cerr << "SoapySDR::Device::make failed" << endl;
		return EXIT_FAILURE;
	}




	/* -------------------------------------------
	 * -----------QUERY SELECTED DEVICE-----------
	   -------------------------------------------*/

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
	sdr->setFrequency(SOAPY_SDR_RX, 0, SIG_FREQ+TUNE_FREQ_OFFSET);
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

	// Register signal SIGINT and signal handler to cleanly exit processing loop
	signal(SIGINT, signalHandler);

	// Loop through sample buffers until sample stream is terminated
	while (not_terminated) {
		void *buffs[] = {buff};
		int flags;
		long long time_ns;

		// Read samples into buffer
		int ret = sdr->readStream(rx_stream, buffs, RX_BUF_SIZE, flags, time_ns, 1e5);
		
		if (ret < 0) {	// Report stream errors
			if ((ret != SOAPY_SDR_TIMEOUT) & (ret != SOAPY_SDR_OVERFLOW)) {
				cout << "Unknown readStream return code " << ret << endl;
			}
		} else {	// If sample stream is intact, process samples in buffer
			for (auto const& sample : buff) {
				processSample(sample, IFfilter, BB_DC_remove, BB_LP_filter, message_receiver, sensor_tracker);
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
