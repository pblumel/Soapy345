PROJ_NAME = Soapy345
VPATH = src
BUILD_PATH = build

OBJECTS = main.o SensorMessageReceiver.o ManchesterDecoder.o crc16.o SensorTracker.o
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

# COMPILE/ASSEMBLE
$(BUILD_PATH)/%.o: %.cpp
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
$(BUILD_PATH)/main.o: filt.h SensorMessageReceiver.h SensorTracker.h
$(BUILD_PATH)/SensorMessageReceiver.o: SensorMessageReceiver.h SymbolLenTracker.h ManchesterDecoder.h crc16.h
$(BUILD_PATH)/ManchesterDecoder.o: ManchesterDecoder.h
$(BUILD_PATH)/crc16.o: crc16.h
$(BUILD_PATH)/SensorTracker.o: SensorTracker.h SensorMessageReceiver.h
