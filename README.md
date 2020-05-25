# Soapy345
A (WIP) 345 MHz sensor receiver based on the SoapySDR wrapper for the HackRF One. It is a rewrite of software I wrote previously in Python using GNU Radio.
Currently, baseline hardware functionality and signal processing functionality is working, but needs some adjustment. Messages are received and verified using the CRC. Next I plan to add functionality to translate the raw sensor state data into useful status fields, and monitor sensors for changes in those fields.
