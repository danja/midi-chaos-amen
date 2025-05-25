PLUGIN_SO = midi_chord_chaos.so
BUNDLE_DIR = midi-chord-chaos.lv2
INSTALL_DIR = ~/.lv2/$(BUNDLE_DIR)

CXX = g++
CXXFLAGS = -O3 -fPIC -DPIC -Wall -std=c++11
LDFLAGS = -shared -lm
LV2_CFLAGS = $(shell pkg-config --cflags lv2)

SOURCES = chord-midi_chord_chaos.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TTL_FILES = chord-manifest.ttl chord-midi_chord_chaos.ttl

.PHONY: all clean install

all: $(PLUGIN_SO)

$(PLUGIN_SO): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -c $< -o $@

bundle: $(PLUGIN_SO)
	mkdir -p $(BUNDLE_DIR)
	cp $(PLUGIN_SO) $(BUNDLE_DIR)/
	cp chord-manifest.ttl $(BUNDLE_DIR)/manifest.ttl
	cp chord-midi_chord_chaos.ttl $(BUNDLE_DIR)/midi_chord_chaos.ttl

install: bundle
	mkdir -p $(INSTALL_DIR)
	cp -r $(BUNDLE_DIR)/* $(INSTALL_DIR)/
	@echo "Plugin installed to $(INSTALL_DIR)"

clean:
	rm -f $(OBJECTS) $(PLUGIN_SO)
	rm -rf $(BUNDLE_DIR)

# Dependencies  
chord-midi_chord_chaos.o: chord-midi_chord_chaos.cpp
