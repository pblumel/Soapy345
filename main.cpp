#include <csignal>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>

#include "filt.h"
#include "decode345.h"

#include <iostream>
#include <fstream>
#include <complex>


#define RX_BUF_SIZE 1024

#define SAMP_RATE 200e3
#define SIG_FREQ 345012e3
#define TUNE_FREQ 344967e3
#define SENSOR_BW 40e3
#define SPS 3

#define IF_FILT_ORDER 50
#define IF_DECIMATION 4

#define BB_FILT_CUTOFF 1225
#define BB_FILT_ORDER 25	// Must be odd for type 1 filter
#define BB_DECIMATION 2

using std::cout;
using std::endl;
using std::complex;

bool not_terminated = true;

void signalHandler(int signum) {
	cout << "Interrupt signal (" << signum << ") received.\n";

	not_terminated = false;
}


int main() {
	// register signal SIGINT and signal handler  
	signal(SIGINT, signalHandler);
	



	/* ------------------------------------------
	 * ------------CONFIGURE HARDWARE------------
	   ------------------------------------------*/

	// 0. enumerate devices (list all devices' information)
	SoapySDR::KwargsList results = SoapySDR::Device::enumerate();
	SoapySDR::Kwargs::iterator it;

	for(unsigned int i = 0; i < results.size(); ++i)
	{
		printf("Found device #%d: ", i);
		for( it = results[i].begin(); it != results[i].end(); ++it)
		{
			printf("%s = %s\n", it->first.c_str(), it->second.c_str());
		}
		printf("\n");
	}

	// 1. create device instance
	
	//	1.1 set arguments
	//		args can be user defined or from the enumeration result
	//		We use first results as args here:
	SoapySDR::Kwargs args = results[0];

	//	1.2 make device
	SoapySDR::Device *sdr = SoapySDR::Device::make(args);

	if( sdr == NULL )
	{
		fprintf(stderr, "SoapySDR::Device::make failed\n");
		return EXIT_FAILURE;
	}

	// 2. query device info
	std::vector< std::string > str_list;	//string list

	//	2.1 antennas
	str_list = sdr->listAntennas( SOAPY_SDR_RX, 0);
	printf("Rx antennas: ");
	for(unsigned int i = 0; i < str_list.size(); ++i)
		printf("%s,", str_list[i].c_str());
	printf("\n");

	//	2.2 gains
	str_list = sdr->listGains( SOAPY_SDR_RX, 0);
	printf("Rx Gains: ");
	for(unsigned int i = 0; i < str_list.size(); ++i)
		printf("%s, ", str_list[i].c_str());
	printf("\n");

	//	2.3. ranges(frequency ranges)
	SoapySDR::RangeList ranges = sdr->getFrequencyRange(SOAPY_SDR_RX, 0);
	printf("Rx freq ranges: ");
	for(unsigned int i = 0; i < ranges.size(); ++i)
		printf("[%g Hz -> %g Hz], ", ranges[i].minimum(), ranges[i].maximum());
	printf("\n");

	// 3. apply settings
	sdr->setGainMode(SOAPY_SDR_RX, 0, false);
	for (auto const& gain : str_list) {	// Configure each gain stage
		if (gain == "LNA") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 20);
			std::cout << gain << ": " << sdr->getGain(SOAPY_SDR_RX, 0, gain) << std::endl;
		} else if (gain == "VGA") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 20);
			std::cout << gain << ": " << sdr->getGain(SOAPY_SDR_RX, 0, gain) << std::endl;
		} else if (gain == "AMP") {
			sdr->setGain(SOAPY_SDR_RX, 0, gain, 0);
			std::cout << gain << ": " << sdr->getGain(SOAPY_SDR_RX, 0, gain) << std::endl;
		}
	}
	
	sdr->setSampleRate(SOAPY_SDR_RX, 0, SAMP_RATE);
	sdr->setFrequency(SOAPY_SDR_RX, 0, TUNE_FREQ);
	cout << "Tuned Freqency " << sdr->getFrequency(SOAPY_SDR_RX, 0) << endl;

	// 4. setup a stream (complex floats)
	SoapySDR::Stream *rx_stream = sdr->setupStream( SOAPY_SDR_RX, SOAPY_SDR_CF32);
	if( rx_stream == NULL)
	{
		fprintf( stderr, "Failed\n");
		SoapySDR::Device::unmake( sdr );
		return EXIT_FAILURE;
	}
	sdr->activateStream( rx_stream, 0, 0, 0);

	// 5. create a re-usable buffer for rx samples
	complex<float> buff[RX_BUF_SIZE];




	/* -----------------------------------------
	 * ---------BEGIN SIGNAL PROCESSING---------
	   -----------------------------------------*/

	// Configure output files for data so that it can be read into GNU Radio file source blocks for debugging (temporary)
	std::ofstream rawfile;
	std::ofstream filtfile;
	rawfile.open("/home/philip/Desktop/soapy_raw_output.bin", std::ios::out | std::ios::binary);
	filtfile.open("/home/philip/Desktop/soapy_filt_output.bin", std::ios::out | std::ios::binary);

	// Configure IF (complex LPF with frequency Xlation) and BB (HPF) filters
	Filter<complex<float>> IFfilter(LPF, IF_FILT_ORDER, sdr->getSampleRate(SOAPY_SDR_RX, 0), SENSOR_BW/2,
			sdr->getFrequency(SOAPY_SDR_RX, 0)-SIG_FREQ);	// Includes frequency translation.
															// The HackRF One samples have significant DC noise, so
															// tuning the hardware to some offset frequency and translating
															// the signal back to FFT center in the digital domain greatly
															// improves SNR.
	Filter<float> BBfilter(HPF, BB_FILT_ORDER, sdr->getSampleRate(SOAPY_SDR_RX, 0)/IF_DECIMATION, BB_FILT_CUTOFF);

	// Configure sample counters for managing IF and BB decimation
	unsigned int if_decimation_count = 0;
	unsigned int bb_decimation_count = 0;

	// Create decoder for 345 data with estimated sample per symbol value for finding sync bits.
	// The decoder computes more accurate SPS estimations per-message using sync bits
	// for overall SPS accuracy throughout the message.
	Decode345 decoder(SPS);

	// Loop through sample buffers until sample stream is terminated
	while (not_terminated) {
		void *buffs[] = {buff};
		int flags;
		long long time_ns;

		// Read samples into buffer
		int ret = sdr->readStream(rx_stream, buffs, RX_BUF_SIZE, flags, time_ns, 1e5);
		//printf("ret = %d, flags = %d, time_ns = %lld\n", ret, flags, time_ns);
		
		if (ret < 0) {	// Report stream errors
			if (ret == SOAPY_SDR_OVERFLOW) {
				cout << "-----OVERFLOW-----" << endl;
			} else if (ret != SOAPY_SDR_TIMEOUT) {
				cout << "Unknown readStream return code " << ret << endl;
			}
		} else {	// If sample stream is intact, process samples in buffer
			for (auto const& sample : buff) {
				/*auto tmp_cmp = sample.real();
				rawfile.write((char *)&tmp_cmp, sizeof(float));
				tmp_cmp = sample.imag();
				rawfile.write((char *)&tmp_cmp, sizeof(float));*/

				// Apply frequency translation and lowpass filter to IF
				auto filt_samp = IFfilter.compute(sample);
				if_decimation_count++;

				// Decimate IF signal
				if (if_decimation_count == IF_DECIMATION) {
					if_decimation_count = 0;

					/*auto tmp_cmp = filt_samp.real();
					rawfile.write((char *)&tmp_cmp, sizeof(float));
					tmp_cmp = filt_samp.imag();
					rawfile.write((char *)&tmp_cmp, sizeof(float));*/

					// Compute magnitude
					//auto tmp_cmp = filt_samp.real()*filt_samp.real() + filt_samp.imag()*filt_samp.imag();
					//rawfile.write((char *)&tmp_cmp, sizeof(float));

					// Compute magnitude (BB) and apply highpass filter to center signal at zero.
					// This allows BB pulse widths to be determined by tracking zero-crossings.
					auto BB_filt_samp = BBfilter.compute(filt_samp.real()*filt_samp.real() + filt_samp.imag()*filt_samp.imag());
					bb_decimation_count++;

					// Decimate BB signal
					if (bb_decimation_count == BB_DECIMATION) {
						bb_decimation_count = 0;

						//filtfile.write((char *)&BB_filt_samp, sizeof(float));
						decoder.push(BB_filt_samp);
					}
				}

			}
		}
	}
	rawfile.close();
	filtfile.close();
	
	// 7. shutdown the stream
	sdr->deactivateStream(rx_stream, 0, 0);	//stop streaming
	sdr->closeStream(rx_stream );

	// 8. cleanup device handle
	SoapySDR::Device::unmake(sdr);
	printf("Done\n");

	return EXIT_SUCCESS;
}
