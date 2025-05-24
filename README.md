# MIDI Chaos Amen - LV2 Plugin

MIDI chaos Amen break generator with pattern learning and tom-toms.

## Repository Structure

```
midi-chaos-amen-lv2/
├── midi_chaos_amen.cpp    # Main plugin implementation
├── midi_chaos_amen.ttl    # Plugin description 
├── manifest.ttl           # Plugin manifest
├── Makefile              # Build configuration
└── README.md             # This file
```

## Build Instructions

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential pkg-config lv2-dev

# Build and install
make
make install
```

## Plugin Features

- **MIDI clock sync** - responds to incoming MIDI clock (0xF8)
- **Pattern learning** - captures incoming MIDI patterns as baseline
- **Channel 10 output** - standard GM drum channel
- **7 drum voices:** Kick (36), Snare (38), Hi-hat (42), Cowbell (56), Low Tom (41), Mid Tom (43), High Tom (45)
- **Chaotic variations** using logistic map algorithm
- **Null pointer safety** - extensive safety checks

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Learn Mode | 0/1 | 0 | Toggle pattern learning |
| Chaos K | 1.0-4.0 | 3.8 | Chaos coefficient |
| Chaos Intensity | 0.0-1.0 | 0.3 | Variation amount |
| Kick Velocity | 1-127 | 100 | MIDI velocity |
| Snare Velocity | 1-127 | 90 | MIDI velocity |
| Hihat Velocity | 1-127 | 70 | MIDI velocity |
| Cowbell Velocity | 1-127 | 80 | MIDI velocity |
| Low Tom Velocity | 1-127 | 85 | MIDI velocity |
| Mid Tom Velocity | 1-127 | 85 | MIDI velocity |
| High Tom Velocity | 1-127 | 85 | MIDI velocity |

## Usage

1. **Basic mode:** Load plugin, send MIDI clock, get chaotic Amen variations
2. **Learn mode:** Enable learn mode, play drum pattern, disable learn mode - plugin uses your pattern as baseline for chaos
3. Route plugin output to drum sampler/synth on channel 10
4. Plugin triggers every 6 MIDI clocks (16th notes)

## Pattern Learning

- Enable Learn Mode parameter
- Play drums on channel 10 while MIDI clock runs
- Plugin captures note timing and maps to pattern grid
- Disable Learn Mode - chaos variations now based on learned pattern
- Supports all 7 drum voices simultaneously
