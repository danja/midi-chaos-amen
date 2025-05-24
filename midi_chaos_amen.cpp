#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#define MIDI_CHAOS_AMEN_URI "http://github.com/danja/midi-chaos-amen"

enum PortIndex {
    MIDI_IN           = 0,
    MIDI_OUT          = 1,
    LEARN_MODE        = 2,
    CHAOS_K           = 3,
    CHAOS_INTENSITY   = 4,
    KICK_VELOCITY     = 5,
    SNARE_VELOCITY    = 6,
    HIHAT_VELOCITY    = 7,
    COWBELL_VELOCITY  = 8,
    TOM_LOW_VELOCITY  = 9,
    TOM_MID_VELOCITY  = 10,
    TOM_HIGH_VELOCITY = 11
};

// MIDI drum notes (GM standard, channel 10)
enum DrumNotes {
    KICK_NOTE     = 36,  // C2
    SNARE_NOTE    = 38,  // D2  
    HIHAT_NOTE    = 42,  // F#2
    COWBELL_NOTE  = 56,  // G#3
    TOM_LOW_NOTE  = 41,  // F2
    TOM_MID_NOTE  = 43,  // G2
    TOM_HIGH_NOTE = 45   // A2
};

typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Sequence;
    LV2_URID midi_MidiEvent;
} URIDs;

// Main plugin class
class MidiChaosAmen {
private:
    URIDs urids;
    LV2_URID_Map* map;
    
    uint32_t clock_count;
    uint32_t current_step;
    uint32_t learn_step;
    bool learning_active;
    
    // Chaos variables
    double chaos_x;
    
    // Base Amen pattern - now 7 instruments
    bool base_pattern[7][16] = {
        {1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0}, // kick
        {0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0}, // snare
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // hihat
        {0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,0}, // cowbell
        {0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}, // tom low
        {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0}, // tom mid
        {0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0}  // tom high
    };
    
    // Learned pattern buffer
    bool learned_pattern[7][16];
    
    // Current chaotic pattern
    bool current_pattern[7][16];
    
    // Ports with null pointer safety
    const LV2_Atom_Sequence* midi_in;
    LV2_Atom_Sequence* midi_out;
    const float* learn_mode;
    const float* chaos_k;
    const float* chaos_intensity;
    const float* kick_velocity;
    const float* snare_velocity;
    const float* hihat_velocity;
    const float* cowbell_velocity;
    const float* tom_low_velocity;
    const float* tom_mid_velocity;
    const float* tom_high_velocity;
    
    // Default values for safety
    float default_chaos_k;
    float default_chaos_intensity;
    float default_kick_velocity;
    float default_snare_velocity;
    float default_hihat_velocity;
    float default_cowbell_velocity;
    float default_tom_low_velocity;
    float default_tom_mid_velocity;
    float default_tom_high_velocity;
    
    // Drum note mapping
    uint8_t drum_notes[7] = {
        KICK_NOTE, SNARE_NOTE, HIHAT_NOTE, COWBELL_NOTE,
        TOM_LOW_NOTE, TOM_MID_NOTE, TOM_HIGH_NOTE
    };
    
    void initializePatterns() {
        // Copy base to learned and current
        memcpy(learned_pattern, base_pattern, sizeof(base_pattern));
        memcpy(current_pattern, base_pattern, sizeof(base_pattern));
    }
    
    void clearLearnedPattern() {
        // Clear learned pattern
        memset(learned_pattern, 0, sizeof(learned_pattern));
    }
    
    int getDrumIndex(uint8_t note) {
        for (int i = 0; i < 7; i++) {
            if (drum_notes[i] == note) return i;
        }
        return -1; // Not found
    }
    
    void learnFromMidi(uint8_t note, uint32_t step) {
        if (!learning_active || step >= 16) return;
        
        int drum_idx = getDrumIndex(note);
        if (drum_idx >= 0) {
            learned_pattern[drum_idx][step] = true;
        }
    }
    
    void generateChaoticPattern() {
        if (!chaos_k || !chaos_intensity) return;
        
        double k = *chaos_k;
        double intensity = *chaos_intensity;
        
        // Clamp values for safety
        k = fmax(1.0, fmin(4.0, k));
        intensity = fmax(0.0, fmin(1.0, intensity));
        
        // Use learned pattern as base if learning was active
        bool (*source_pattern)[16] = learning_active ? learned_pattern : base_pattern;
        
        // Copy source pattern
        memcpy(current_pattern, source_pattern, sizeof(current_pattern));
        
        for (int i = 0; i < 16; i++) {
            // Generate chaos values with bounds checking
            for (int drum = 0; drum < 7; drum++) {
                chaos_x = k * chaos_x * (1.0 - chaos_x);
                if (chaos_x < 0.0 || chaos_x > 1.0) chaos_x = 0.5;
                
                double chaos_val = chaos_x;
                double threshold = 0.3 + (drum * 0.1); // Different sensitivity per drum
                
                // Apply chaos modifications
                if (!source_pattern[drum][i] && chaos_val < intensity * threshold) {
                    current_pattern[drum][i] = true;
                } else if (source_pattern[drum][i] && chaos_val > (1.0 - intensity * 0.15)) {
                    current_pattern[drum][i] = false;
                }
            }
        }
    }
    
    void writeMidiNote(LV2_Atom_Forge* forge, uint32_t frames, uint8_t note, uint8_t velocity, bool note_on) {
        if (!forge) return;
        
        uint8_t midi_msg[3];
        midi_msg[0] = (note_on ? 0x90 : 0x80) | 9; // Channel 10 (9 in 0-based)
        midi_msg[1] = note;
        midi_msg[2] = note_on ? velocity : 0;
        
        lv2_atom_forge_frame_time(forge, frames);
        lv2_atom_forge_atom(forge, 3, urids.midi_MidiEvent);
        lv2_atom_forge_raw(forge, midi_msg, 3);
        lv2_atom_forge_pad(forge, 3);
    }
    
public:
    MidiChaosAmen(double rate, const LV2_Feature* const* features) : 
        clock_count(0), current_step(0), learn_step(0), learning_active(false), chaos_x(0.5) {
        
        // Initialize all pointers to null for safety
        map = nullptr;
        midi_in = nullptr;
        midi_out = nullptr;
        learn_mode = nullptr;
        chaos_k = nullptr;
        chaos_intensity = nullptr;
        kick_velocity = nullptr;
        snare_velocity = nullptr;
        hihat_velocity = nullptr;
        cowbell_velocity = nullptr;
        tom_low_velocity = nullptr;
        tom_mid_velocity = nullptr;
        tom_high_velocity = nullptr;
        
        // Get URID map - critical for operation
        if (features) {
            for (int i = 0; features[i]; i++) {
                if (features[i]->URI && !strcmp(features[i]->URI, LV2_URID__map)) {
                    map = (LV2_URID_Map*)features[i]->data;
                    break;
                }
            }
        }
        
        if (!map) {
            // Plugin cannot work without URID map
            return;
        }
        
        // Map URIDs
        urids.atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
        urids.atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
        urids.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
        
        // Set safe defaults
        default_chaos_k = 3.8f;
        default_chaos_intensity = 0.3f;
        default_kick_velocity = 100.0f;
        default_snare_velocity = 90.0f;
        default_hihat_velocity = 70.0f;
        default_cowbell_velocity = 80.0f;
        default_tom_low_velocity = 85.0f;
        default_tom_mid_velocity = 85.0f;
        default_tom_high_velocity = 85.0f;
        
        initializePatterns();
    }
    
    // Safe parameter getters with null checks
    bool getLearnMode() { return learn_mode ? (*learn_mode > 0.5f) : false; }
    float getChaosK() { return chaos_k ? *chaos_k : default_chaos_k; }
    float getChaosIntensity() { return chaos_intensity ? *chaos_intensity : default_chaos_intensity; }
    uint8_t getKickVelocity() { return kick_velocity ? (uint8_t)*kick_velocity : (uint8_t)default_kick_velocity; }
    uint8_t getSnareVelocity() { return snare_velocity ? (uint8_t)*snare_velocity : (uint8_t)default_snare_velocity; }
    uint8_t getHihatVelocity() { return hihat_velocity ? (uint8_t)*hihat_velocity : (uint8_t)default_hihat_velocity; }
    uint8_t getCowbellVelocity() { return cowbell_velocity ? (uint8_t)*cowbell_velocity : (uint8_t)default_cowbell_velocity; }
    uint8_t getTomLowVelocity() { return tom_low_velocity ? (uint8_t)*tom_low_velocity : (uint8_t)default_tom_low_velocity; }
    uint8_t getTomMidVelocity() { return tom_mid_velocity ? (uint8_t)*tom_mid_velocity : (uint8_t)default_tom_mid_velocity; }
    uint8_t getTomHighVelocity() { return tom_high_velocity ? (uint8_t)*tom_high_velocity : (uint8_t)default_tom_high_velocity; }
    
    uint8_t getVelocityForDrum(int drum_idx) {
        switch (drum_idx) {
            case 0: return getKickVelocity();
            case 1: return getSnareVelocity();
            case 2: return getHihatVelocity();
            case 3: return getCowbellVelocity();
            case 4: return getTomLowVelocity();
            case 5: return getTomMidVelocity();
            case 6: return getTomHighVelocity();
            default: return 80;
        }
    }
    
    void connectPort(uint32_t port, void* data) {
        if (!data) return; // Safety check
        
        switch (port) {
            case MIDI_IN: midi_in = (const LV2_Atom_Sequence*)data; break;
            case MIDI_OUT: midi_out = (LV2_Atom_Sequence*)data; break;
            case LEARN_MODE: learn_mode = (const float*)data; break;
            case CHAOS_K: chaos_k = (const float*)data; break;
            case CHAOS_INTENSITY: chaos_intensity = (const float*)data; break;
            case KICK_VELOCITY: kick_velocity = (const float*)data; break;
            case SNARE_VELOCITY: snare_velocity = (const float*)data; break;
            case HIHAT_VELOCITY: hihat_velocity = (const float*)data; break;
            case COWBELL_VELOCITY: cowbell_velocity = (const float*)data; break;
            case TOM_LOW_VELOCITY: tom_low_velocity = (const float*)data; break;
            case TOM_MID_VELOCITY: tom_mid_velocity = (const float*)data; break;
            case TOM_HIGH_VELOCITY: tom_high_velocity = (const float*)data; break;
        }
    }
    
    void run(uint32_t n_samples) {
        if (!midi_in || !midi_out || !map) return;
        
        // Check learn mode state change
        bool should_learn = getLearnMode();
        if (should_learn && !learning_active) {
            // Start learning
            learning_active = true;
            learn_step = 0;
            clearLearnedPattern();
        } else if (!should_learn && learning_active) {
            // Stop learning
            learning_active = false;
        }
        
        // Set up forge to write to output
        const uint32_t out_capacity = midi_out->atom.size;
        LV2_Atom_Forge forge;
        lv2_atom_forge_init(&forge, map);
        lv2_atom_forge_set_buffer(&forge, (uint8_t*)midi_out, out_capacity);
        
        // Start sequence
        LV2_Atom_Forge_Frame seq_frame;
        lv2_atom_forge_sequence_head(&forge, &seq_frame, 0);
        
        // Process incoming MIDI
        LV2_ATOM_SEQUENCE_FOREACH(midi_in, ev) {
            if (ev->body.type == urids.midi_MidiEvent) {
                const uint8_t* const msg = (const uint8_t*)(ev + 1);
                
                // Handle note on for chaos trigger
                if ((msg[0] & 0xF0) == 0x90 && msg[2] > 0) {
                    // Learn from incoming notes
                    if (learning_active && (msg[0] & 0x0F) == 9) {
                        int drum_idx = getDrumIndex(msg[1]);
                        if (drum_idx >= 0) {
                            learned_pattern[drum_idx][current_step] = true;
                        }
                    }
                    
                    // Trigger chaotic pattern step
                    for (int drum = 0; drum < 7; drum++) {
                        if (current_pattern[drum][current_step]) {
                            writeMidiNote(&forge, ev->time.frames, drum_notes[drum], 
                                        getVelocityForDrum(drum), true);
                        }
                    }
                    
                    current_step = (current_step + 1) % 16;
                    
                    // Generate new pattern every bar
                    if (current_step == 0 && (rand() % 4) == 0) {
                        generateChaoticPattern();
                    }
                }
            }
        }
        
        lv2_atom_forge_pop(&forge, &seq_frame);
    }
};

// LV2 C interface
extern "C" {

static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                             double rate,
                             const char* bundle_path,
                             const LV2_Feature* const* features) {
    return new MidiChaosAmen(rate, features);
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    if (!instance) return;
    MidiChaosAmen* plugin = (MidiChaosAmen*)instance;
    plugin->connectPort(port, data);
}

static void activate(LV2_Handle instance) {
    // Nothing to do
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    if (!instance) return;
    MidiChaosAmen* plugin = (MidiChaosAmen*)instance;
    plugin->run(n_samples);
}

static void deactivate(LV2_Handle instance) {
    // Nothing to do
}

static void cleanup(LV2_Handle instance) {
    if (instance) {
        delete (MidiChaosAmen*)instance;
    }
}

static const void* extension_data(const char* uri) {
    return NULL;
}

static const LV2_Descriptor descriptor = {
    MIDI_CHAOS_AMEN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    switch (index) {
        case 0: return &descriptor;
        default: return NULL;
    }
}

} // extern "C"
