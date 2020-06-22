PROJ_NAME = Soapy345
VPATH = src
BUILD_PATH = build

OBJECTS = main.o SensorMessageReceiver.o ManchesterDecoder.o CRC16.o SensorTracker.o SensorHistory.o
OBJ_FILES = $(addprefix build/,$(OBJECTS))


all: build_path $(BUILD_PATH)/$(PROJ_NAME)
	echo Done building
	
install: build_path $(BUILD_PATH)/$(PROJ_NAME)
	cp $(BUILD_PATH)/$(PROJ_NAME) /usr/local/bin/
	
uninstall:
	rm -f /usr/local/bin/$(PROJ_NAME)

# LINK
$(BUILD_PATH)/$(PROJ_NAME): $(OBJ_FILES)
	g++ -o $@ $^ -lSoapySDR

# COMPILE/ASSEMBLE GENERIC
$(BUILD_PATH)/%.o: %.cpp
	g++ -std=c++17 -O3 -Wall -c $< -o $@

# COMPILE/ASSEMBLE DSP
$(BUILD_PATH)/%.o: dsp/%.cpp
	g++ -std=c++17 -O3 -Wall -c $< -o $@

# COMPILE/ASSEMBLE MESSAGING
$(BUILD_PATH)/%.o: messaging/%.cpp
	g++ -std=c++17 -O3 -Wall -c $< -o $@

# COMPILE/ASSEMBLE TRACKING
$(BUILD_PATH)/%.o: tracking/%.cpp
	g++ -std=c++17 -O3 -Wall -c $< -o $@

# Create build folder
.PHONY: build_path
build_path: $(BUILD_PATH)
$(BUILD_PATH):
	mkdir -p $@

# CLEAN BUILD FILES
clean:
	rm -f $(BUILD_PATH)/$(PROJ_NAME) $(BUILD_PATH)/*.o

# Dependency Rules
$(BUILD_PATH)/main.o: dsp/Filter.h dsp/SignalGenerator.h messaging/SensorMessageReceiver.h tracking/SensorTracker.h tracking/SensorHistory.h
$(BUILD_PATH)/SensorMessageReceiver.o: messaging/SensorMessageReceiver.h messaging/SymbolLenTracker.h messaging/ManchesterDecoder.h messaging/CRC16.h
$(BUILD_PATH)/ManchesterDecoder.o: messaging/ManchesterDecoder.h
$(BUILD_PATH)/CRC16.o: messaging/CRC16.h
$(BUILD_PATH)/SensorTracker.o: tracking/SensorTracker.h tracking/SensorHistory.h messaging/SensorMessageReceiver.h
$(BUILD_PATH)/SensorHistory.o: tracking/SensorHistory.h messaging/SensorMessageReceiver.h
