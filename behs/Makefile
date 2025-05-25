PLUGIN_SO = midi_bass_chaos.so
BUNDLE_DIR = midi-bass-chaos.lv2
INSTALL_DIR = ~/.lv2/$(BUNDLE_DIR)

CXX = g++
CXXFLAGS = -O3 -fPIC -DPIC -Wall -std=c++11
LDFLAGS = -shared -lm
LV2_CFLAGS = $(shell pkg-config --cflags lv2)

SOURCES = bass-midi_bass_chaos.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean install

all: $(PLUGIN_SO)

$(PLUGIN_SO): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -c $< -o $@

bundle: $(PLUGIN_SO)
	mkdir -p $(BUNDLE_DIR)
	cp $(PLUGIN_SO) $(BUNDLE_DIR)/
	cp bass-manifest.ttl $(BUNDLE_DIR)/manifest.ttl
	cp bass-midi_bass_chaos.ttl $(BUNDLE_DIR)/midi_bass_chaos.ttl

install: bundle
	mkdir -p $(INSTALL_DIR)
	cp -r $(BUNDLE_DIR)/* $(INSTALL_DIR)/
	@echo "Bass Chaos plugin installed to $(INSTALL_DIR)"

clean:
	rm -f $(OBJECTS) $(PLUGIN_SO)
	rm -rf $(BUNDLE_DIR)

# Dependencies  
bass-midi_bass_chaos.o: bass-midi_bass_chaos.cpp
