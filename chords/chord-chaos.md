# MIDI Chord Chaos - LV2 Plugin

Transform single notes into sophisticated chord progressions using chaos mathematics. Features intelligent voice leading for smooth harmonic transitions.

**Key Features:**
- Logistic map chaos algorithm drives chord selection
- 8 chord types: Major, Minor, 7ths, Augmented, Diminished, Sus2
- Smart voice leading minimizes note movement between chords
- Real-time harmonic complexity from minimal input

## Quick Start
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential pkg-config lv2-dev

# Build
make && make install

# Load in DAW → play single notes → get complex chords
```
```bash
# Ubuntu/Debian
sudo apt install build-essential pkg-config lv2-dev

# Fedora/RHEL  
sudo dnf install gcc-c++ pkgconfig lv2-devel

# Arch Linux
sudo pacman -S base-devel pkg-config lv2
```

### Build Process
```bash
git clone <your-repo>
cd midi-chord-chaos-lv2

# Create files
# Copy midi_chord_chaos.cpp and midi_chord_chaos.ttl from artifacts above

# Create manifest.ttl:
cat > manifest.ttl << 'EOF'
@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

<http://github.com/danja/midi-chord-chaos>
	a lv2:Plugin ;
	lv2:binary <midi_chord_chaos.so> ;
	rdfs:seeAlso <midi_chord_chaos.ttl> .
EOF

# Create Makefile:
cat > Makefile << 'EOF'
PLUGIN_NAME = midi-chord-chaos
PLUGIN_SO = midi_chord_chaos.so
BUNDLE_DIR = $(PLUGIN_NAME).lv2
INSTALL_DIR = ~/.lv2/$(BUNDLE_DIR)

CXX = g++
CXXFLAGS = -O3 -fPIC -DPIC -Wall -std=c++11
LDFLAGS = -shared -lm
LV2_CFLAGS = $(shell pkg-config --cflags lv2)

SOURCES = midi_chord_chaos.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TTL_FILES = manifest.ttl midi_chord_chaos.ttl

all: $(PLUGIN_SO)

$(PLUGIN_SO): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -c $< -o $@

install: $(PLUGIN_SO)
	mkdir -p $(BUNDLE_DIR)
	cp $(PLUGIN_SO) $(BUNDLE_DIR)/
	cp $(TTL_FILES) $(BUNDLE_DIR)/
	mkdir -p $(INSTALL_DIR)
	cp -r $(BUNDLE_DIR)/* $(INSTALL_DIR)/

clean:
	rm -f $(OBJECTS) $(PLUGIN_SO)
	rm -rf $(BUNDLE_DIR)
EOF

# Build and install
make
make install
```

### Verify Installation
```bash
lv2ls | grep chord-chaos
```

## Algorithm Description

### Chaos Engine
The plugin uses the **logistic map equation**: `x[n+1] = k * x[n] * (1 - x[n])`

- **k parameter (1.0-4.0)**: Controls chaos behavior
  - k < 3.0: Stable, predictable patterns
  - k ≈ 3.57: Onset of chaos
  - k = 3.8: Complex chaotic behavior (default)
  - k → 4.0: Maximum chaos, highly unpredictable

- **Intensity (0.0-1.0)**: Controls how chaos affects output
  - 0.0: Minimal variation
  - 0.3: Moderate chaos (default)
  - 1.0: Maximum variation

### Chord Generation Process

Each input note triggers three chaos calculations:

1. **Chord Type Selection**: `chaos_value * 8` selects from 8 chord types
2. **Inversion Selection**: `chaos_value * 3` chooses inversion (0-2)
3. **Octave Shift**: Probability-based octave displacement (±12 semitones)

## Chord Selection Rationale

### The 8 Chord Types

| Index | Chord | Intervals | Rationale |
|-------|-------|-----------|-----------|
| 0 | Major | 0,4,7 | Fundamental consonance |
| 1 | Minor | 0,3,7 | Essential counterpart to major |
| 2 | Major 7th | 0,4,7,11 | Jazz sophistication |
| 3 | Minor 7th | 0,3,7,10 | Blues/jazz foundation |
| 4 | Augmented | 0,4,8 | Symmetric tension |
| 5 | Diminished | 0,3,6 | Maximum dissonance |
| 6 | Dominant 7th | 0,4,7,10 | Classical resolution driver |
| 7 | Sus2 | 0,2,7 | Open, unresolved quality |

### Design Philosophy

**Tonal Balance**: Mix of consonant (Major, Minor) and dissonant (Diminished, Augmented) chords creates dynamic tension.

**Jazz Influence**: Inclusion of 7th chords (Major 7th, Minor 7th, Dominant 7th) provides sophisticated harmonic color beyond basic triads.

**Symmetric Properties**: Augmented (major thirds) and Diminished (minor thirds) chords offer mathematical elegance and equal intervallic spacing.

**Suspension**: Sus2 chord provides harmonic ambiguity - neither major nor minor resolution.

### Chaos Interaction

The logistic map's non-linear behavior ensures:
- **Unpredictable sequences**: Same input note can produce different chords
- **Strange attractors**: Certain chord types may cluster then suddenly shift
- **Sensitivity**: Small parameter changes create dramatically different harmonic progressions
- **Non-repetition**: Long sequences before pattern repetition (if any)

## Usage

### Basic Setup
1. Load plugin on MIDI track
2. Route plugin output to synthesizer/sampler
3. Play single notes → receive complex chords

### Parameter Exploration
- **Low Chaos K (2.0-3.0)**: More predictable, stable progressions
- **High Chaos K (3.8-4.0)**: Unpredictable, adventurous harmonies
- **Variable Intensity**: Fine-tune chaos influence without changing core behavior

### Creative Applications
- **Harmonic sketching**: Generate unexpected progressions from simple melodies
- **Composition catalyst**: Use chaotic output as inspiration for arrangements
- **Live performance**: Real-time harmonic complexity from minimal input
- **Sound design**: Create evolving harmonic textures

## Technical Notes

### MIDI Implementation
- **Input**: Any MIDI channel, single notes
- **Output**: Configurable channel (0-15), up to 4-note chords
- **Note handling**: Polyphonic - each input note generates independent chord
- **Timing**: Zero-latency chord generation

### Performance
- **CPU usage**: Minimal - simple arithmetic operations
- **Memory**: ~1KB per instance
- **Latency**: Real-time suitable for live performance

### Limitations
- **Chord complexity**: Maximum 4 notes per chord (could expand)
- **No harmonic analysis**: Ignores musical context (purely algorithmic)
- **Fixed chord set**: 8 predefined types (extensible in code)

## Advanced Usage

### Chain Multiple Instances
Use different chaos parameters on parallel tracks for harmonic layering.

### Automation
Automate Chaos K parameter for evolving harmonic complexity over time.

### Integration
Combine with arpeggiators, delays, or other MIDI effects for complex textures.

## Future Enhancements

- **Scale awareness**: Constrain chords to specific keys
- **Voice leading**: Smooth chord transitions
- **Rhythm integration**: Chaos-controlled timing variations
- **Extended chords**: 9th, 11th, 13th extensions
- **Custom chord sets**: User-definable chord libraries

## License

MIT License - Free for all uses.
