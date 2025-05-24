# LV2 MIDI Chaos Amen Plugin Makefile

# Plugin details
PLUGIN_NAME = midi-chaos-amen
PLUGIN_SO = midi_chaos_amen.so
PLUGIN_URI = http://github.com/danja/midi-chaos-amen

# Directories
BUNDLE_DIR = $(PLUGIN_NAME).lv2
INSTALL_DIR = ~/.lv2/$(BUNDLE_DIR)
SYSTEM_INSTALL_DIR = /usr/lib/lv2/$(BUNDLE_DIR)

# Compiler settings
CXX = g++
CXXFLAGS = -O3 -fPIC -DPIC -Wall -std=c++11
LDFLAGS = -shared -lm

# LV2 includes (adjust path if needed)
PKG_CONFIG = pkg-config
LV2_CFLAGS = $(shell $(PKG_CONFIG) --cflags lv2)
LV2_LIBS = $(shell $(PKG_CONFIG) --libs lv2)

# Source files
SOURCES = midi_chaos_amen.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# TTL files
TTL_FILES = manifest.ttl midi_chaos_amen.ttl

.PHONY: all clean install install-system uninstall bundle

all: $(PLUGIN_SO)

$(PLUGIN_SO): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LV2_LIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -c $< -o $@

bundle: $(PLUGIN_SO)
	mkdir -p $(BUNDLE_DIR)
	cp $(PLUGIN_SO) $(BUNDLE_DIR)/
	cp $(TTL_FILES) $(BUNDLE_DIR)/

install: bundle
	mkdir -p $(INSTALL_DIR)
	cp -r $(BUNDLE_DIR)/* $(INSTALL_DIR)/
	@echo "Plugin installed to $(INSTALL_DIR)"

install-system: bundle
	sudo mkdir -p $(SYSTEM_INSTALL_DIR)
	sudo cp -r $(BUNDLE_DIR)/* $(SYSTEM_INSTALL_DIR)/
	@echo "Plugin installed to $(SYSTEM_INSTALL_DIR)"

uninstall:
	rm -rf $(INSTALL_DIR)
	@echo "Plugin uninstalled from $(INSTALL_DIR)"

clean:
	rm -f $(OBJECTS) $(PLUGIN_SO)
	rm -rf $(BUNDLE_DIR)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean all

# Dependencies
chaos_amen.o: chaos_amen.cpp

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build plugin (default)"
	@echo "  bundle       - Create LV2 bundle directory"
	@echo "  install      - Install to user LV2 directory"
	@echo "  install-system - Install system-wide (requires sudo)"
	@echo "  uninstall    - Remove from user LV2 directory"
	@echo "  clean        - Remove build files"
	@echo "  debug        - Build with debug symbols"
	@echo "  help         - Show this message"
