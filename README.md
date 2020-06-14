# Soapy345
A (WIP) 345 MHz sensor receiver based on the [SoapySDR](https://github.com/pothosware/SoapySDR) wrapper for the HackRF One. It is a rewrite of software I wrote previously in Python using GNU Radio.
Currently, baseline hardware functionality and signal processing functionality is working, but needs some adjustment. Messages are received and verified using the CRC. Messages are used to track sensor state and output readable status change sumamries.
Next up is to add the ability to process a prerecorded file of IQ samples so that the signal processing implementation can be quickly optimized for accuracy and efficiency with consistent input data quality.

## Install Dependencies:
apt install build-essential libsoapysdr-dev soapysdr-module-hackrf

# Signal Processing Overview
## 1. Raw Signal
![200 kHz bandwidth raw signal data](doc/raw_signal.png)

## 2. Intermediate Frequency Signal
![50 kHz bandwidth filtered IF signal data](doc/if_filt_signal.png)

## 3. Baseband Signal
![bb unfiltered signal data time domain](doc/bb_signal_time.png)
![bb unfiltered signal data freq domain](doc/bb_signal_freq.png)
Note the lage DC offset due to using a digital diode detector (magnitude squared)

## 4. Filtered Baseband Signal (DC Remove)
![bb filtered signal data time domain](doc/bb_filt_signal_time.png)
![bb filtered signal data freq domain](doc/bb_filt_signal_freq.png)
With optimal DC offset applied to the signal, a square wave can easily be derived from the zero-crossings

## 5. Final Square Wave
![final square wave](doc/final_square_wave.png)
Note that even the low-amplitude noise signal (at the left and right edges of this graphic) results in a square wave due to no threshold detection. This noise data is easily eliminated by comparing it with the expected sync sequence of a sensor transmission.
