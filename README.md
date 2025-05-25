# MIDI Chaos Plugins

Three LV2 MIDI plugins using chaos mathematics to transform simple input into complex musical output.

## Plugins

### MIDI Chaos Amen
Drum pattern generator responding to MIDI notes with chaotic Amen break variations.
- **7 drum voices**: Kick, Snare, Hi-hat, Cowbell, 3 Toms
- **Pattern learning**: Capture custom patterns as chaos baseline
- **Sparsity control**: Gates output based on input drum types

### MIDI Chord Chaos  
Chord generator with intelligent voice leading and strange key shifts.
- **8 chord types**: Major, Minor, 7ths, Augmented, Diminished, Sus2
- **Voice leading**: Minimizes movement between chord changes
- **Strange key shifts**: Chaotic key changes every 4 beats
- **Sparsity**: Variable chord density (full chords to single notes)

### MIDI Bass Chaos
Bass line generator with reggae-style syncopation.
- **Simple intervals**: Root, octave, 5th, 3rd variations
- **Reggae mode**: Off-beat emphasis patterns
- **Bass range lock**: E1-E4 (28-64)
- **Velocity variation**: Chaos-controlled dynamics

## Build & Install

### Dependencies
```bash
# Ubuntu/Debian
sudo apt install build-essential pkg-config lv2-dev

# Fedora
sudo dnf install gcc-c++ pkgconfig lv2-devel

# Arch
sudo pacman -S base-devel pkg-config lv2
```

### Build Process
```bash
# Each plugin in separate directory
cd drums && make && make install
cd ../chords && make -f chord-Makefile && make -f chord-Makefile install  
cd ../bass && make -f bass-Makefile && make -f bass-Makefile install

# Verify installation
lv2ls | grep danja
```

## Chaos Algorithm

All plugins use the **logistic map**: `x[n+1] = k * x[n] * (1 - x[n])`

**Parameters:**
- **Chaos K (1.0-4.0)**: Controls predictability
  - < 3.0: Stable patterns
  - 3.8: Complex chaos (default)
  - → 4.0: Maximum unpredictability
- **Chaos Intensity (0.0-1.0)**: Amount of variation applied

## Usage

### Basic Setup
1. Load plugin on MIDI track
2. Route output to appropriate instrument (drums channel 10, bass/chords to synths)
3. Play simple patterns → receive complex variations

### Creative Applications
- **Live performance**: Complex output from minimal input
- **Composition**: Generate unexpected patterns as inspiration
- **Arrangement**: Add rhythmic/harmonic complexity to basic tracks

### Parameter Tips
- **Low chaos**: Subtle variations, musical predictability
- **High chaos**: Adventurous, experimental results
- **Sparsity**: Control density for breathing room
- **Learn mode** (drums): Train on your patterns

## DAW Compatibility

Tested with:
- **Reaper**: Load as Virtual Instruments
- **Ardour**: MIDI effects or instruments
- **Qtractor**: MIDI plugins

**Note**: Some DAWs require rescan after installation.

## Technical Notes

- **Zero latency**: Real-time suitable
- **Polyphonic**: Multiple simultaneous triggers
- **MIDI standard**: GM drum mapping, configurable channels
- **Memory safe**: Extensive bounds checking

## File Structure
```
midi-chaos-amen/
├── drums/          # MIDI Chaos Amen
├── chords/         # MIDI Chord Chaos  
├── bass/           # MIDI Bass Chaos
└── README.md       # This file
```

## License
MIT License - Free for all uses.
