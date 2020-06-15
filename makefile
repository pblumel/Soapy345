VPATH = src
BUILD_PATH = build
LINK_TARGET = $(BUILD_PATH)/Soapy345


all: $(BUILD_PATH)/Soapy345
	echo Done building, the executable can be found at build/Soapy345

# LINK
$(BUILD_PATH)/Soapy345: $(BUILD_PATH)/main.o $(BUILD_PATH)/SensorMessageReceiver.o $(BUILD_PATH)/ManchesterDecoder.o $(BUILD_PATH)/crc16.o $(BUILD_PATH)/SensorTracker.o
	g++ -o $@ $^ -lSoapySDR

# COMPILE/ASSEMBLE
$(BUILD_PATH)/%.o: %.cpp $(BUILD_PATH)
	g++ -std=c++17 -O3 -Wall -c $< -o $@

# Create build folder
$(BUILD_PATH):
	mkdir $@

# CLEAN BUILD FILES
clean:
	rm -f $(BUILD_PATH)/Soapy345 $(BUILD_PATH)/*.o
