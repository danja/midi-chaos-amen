#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#define CHORD_CHAOS_URI "http://github.com/danja/midi-chord-chaos"

enum PortIndex {
    MIDI_IN         = 0,
    MIDI_OUT        = 1,
    CHAOS_K         = 2,
    CHAOS_INTENSITY = 3,
    CHORD_VELOCITY  = 4,
    CHORD_CHANNEL   = 5,
    STRANGE_KEY_SHIFT = 6
};

typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Sequence;
    LV2_URID midi_MidiEvent;
} URIDs;

class ChordChaos {
private:
    URIDs urids;
    LV2_URID_Map* map;
    
    double chaos_x;
    
    // Chord types (intervals from root)
    int chord_types[8][4] = {
        {0, 4, 7, -1},   // Major
        {0, 3, 7, -1},   // Minor  
        {0, 4, 7, 11},   // Maj7
        {0, 3, 7, 10},   // Min7
        {0, 4, 8, -1},   // Aug
        {0, 3, 6, -1},   // Dim
        {0, 4, 7, 10},   // Dom7
        {0, 2, 7, -1}    // Sus2
    };
    
    // Ports
    const LV2_Atom_Sequence* midi_in;
    LV2_Atom_Sequence* midi_out;
    const float* chaos_k;
    const float* chaos_intensity;
    const float* chord_velocity;
    const float* chord_channel;
    const float* strange_key_shift;
    
    // Active notes for chord off
    bool active_chords[128];
    
    // Voice leading - track previous chord
    int previous_chord[4];
    int previous_chord_size;
    bool first_chord;
    
    // Bar tracking for key shifts
    int beat_count;
    int current_key_shift;
    
    int calculateVoiceDistance(int* new_chord, int chord_size) {
        if (first_chord) return 0;
        
        int total_distance = 0;
        int min_prev_size = (previous_chord_size < chord_size) ? previous_chord_size : chord_size;
        
        for (int i = 0; i < min_prev_size; i++) {
            int min_distance = 127;
            for (int j = 0; j < chord_size; j++) {
                int distance = abs(new_chord[j] - previous_chord[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                }
            }
            total_distance += min_distance;
        }
        return total_distance;
    }
    
    void optimizeVoiceLeading(int* chord_notes, int chord_size, uint8_t root) {
        if (chord_size <= 0 || chord_size > 4) return;
        
        int best_arrangement[4];
        int best_distance = 999;
        int best_size = chord_size;
        
        // Try different octave arrangements
        for (int oct_shift = -12; oct_shift <= 12; oct_shift += 12) {
            int test_chord[4];
            int valid_size = 0;
            
            for (int i = 0; i < chord_size && i < 4; i++) {
                int note = chord_notes[i] + oct_shift;
                if (note >= 0 && note <= 127) {
                    test_chord[valid_size++] = note;
                }
            }
            
            if (valid_size > 0) {
                int distance = calculateVoiceDistance(test_chord, valid_size);
                if (distance < best_distance) {
                    best_distance = distance;
                    memcpy(best_arrangement, test_chord, valid_size * sizeof(int));
                    best_size = valid_size;
                }
            }
        }
        
        // Copy optimized arrangement back
        chord_size = (best_size < chord_size) ? best_size : chord_size;
        for (int i = 0; i < chord_size && i < 4; i++) {
            chord_notes[i] = best_arrangement[i];
        }
        
        // Store for next iteration
        previous_chord_size = (chord_size < 4) ? chord_size : 4;
        memcpy(previous_chord, chord_notes, previous_chord_size * sizeof(int));
        first_chord = false;
    }
    
    void generateChaos() {
        double k = chaos_k ? *chaos_k : 3.8;
        k = fmax(1.0, fmin(4.0, k)); // Clamp k
        chaos_x = k * chaos_x * (1.0 - chaos_x);
        
        // Reset if chaos gets stuck or invalid
        if (chaos_x < 0.0 || chaos_x > 1.0 || chaos_x == 0.0 || chaos_x == 1.0) {
            chaos_x = 0.5;
        }
    }
    
    int selectChordType() {
        generateChaos();
        return (int)(chaos_x * 7.999); // Ensure < 8
    }
    
    int selectInversion() {
        generateChaos();
        return (int)(chaos_x * 2.999); // Ensure < 3
    }
    
    bool getStrangeKeyShift() {
        return strange_key_shift ? (*strange_key_shift > 0.5f) : false;
    }
    
    uint8_t getChordVelocity() {
        return chord_velocity ? (uint8_t)fmax(1, fmin(127, *chord_velocity)) : 80;
    }
    
    uint8_t getChordChannel() {
        return chord_channel ? (uint8_t)fmax(0, fmin(15, *chord_channel)) : 0;
    }
    
    int calculateStrangeKeyShift() {
        generateChaos();
        // Strange intervals: tritone, minor 2nd, major 7th, minor 6th
        int strange_shifts[] = {6, 1, 11, 8, -6, -1, -11, -8};
        int index = (int)(chaos_x * 7.999); // Ensure < 8
        return strange_shifts[index];
    }
    
    void updateBarTracking() {
        beat_count++;
        // Check for bar boundary (assuming 4/4 time)
        if (beat_count % 4 == 0) {
            if (getStrangeKeyShift()) {
                current_key_shift = calculateStrangeKeyShift();
            }
        }
    }
    
    void writeChord(LV2_Atom_Forge* forge, uint32_t frames, uint8_t root, bool note_on) {
        if (!forge) return;
        
        int chord_type = selectChordType();
        int inversion = selectInversion();
        uint8_t channel = getChordChannel();
        uint8_t velocity = note_on ? getChordVelocity() : 0;
        
        if (note_on) {
            // Generate chord notes
            int chord_notes[4];
            int chord_size = 0;
            
            for (int i = 0; i < 4; i++) {
                if (chord_types[chord_type][i] == -1) break;
                
                int note = root + chord_types[chord_type][i] + current_key_shift;
                
                // Apply inversion
                if (i < inversion) {
                    note += 12;
                }
                
                chord_notes[chord_size++] = note;
            }
            
            // Optimize voice leading
            optimizeVoiceLeading(chord_notes, chord_size, root);
            
            // Output optimized chord
            for (int i = 0; i < chord_size; i++) {
                if (chord_notes[i] >= 0 && chord_notes[i] <= 127) {
                    uint8_t midi_msg[3];
                    midi_msg[0] = 0x90 | (channel & 0x0F);
                    midi_msg[1] = chord_notes[i];
                    midi_msg[2] = velocity;
                    
                    lv2_atom_forge_frame_time(forge, frames);
                    lv2_atom_forge_atom(forge, 3, urids.midi_MidiEvent);
                    lv2_atom_forge_raw(forge, midi_msg, 3);
                    lv2_atom_forge_pad(forge, 3);
                    
                    active_chords[chord_notes[i]] = true;
                }
            }
        } else {
            // Note off - stop active chord notes
            for (int i = 0; i < 128; i++) {
                if (active_chords[i]) {
                    uint8_t midi_msg[3] = {(uint8_t)(0x80 | (channel & 0x0F)), (uint8_t)i, 0};
                    
                    lv2_atom_forge_frame_time(forge, frames);
                    lv2_atom_forge_atom(forge, 3, urids.midi_MidiEvent);
                    lv2_atom_forge_raw(forge, midi_msg, 3);
                    lv2_atom_forge_pad(forge, 3);
                    
                    active_chords[i] = false;
                }
            }
        }
    }
    
public:
    ChordChaos(double rate, const LV2_Feature* const* features) : chaos_x(0.5) {
        map = nullptr;
        midi_in = nullptr;
        midi_out = nullptr;
        chaos_k = nullptr;
        chaos_intensity = nullptr;
        chord_velocity = nullptr;
        chord_channel = nullptr;
        strange_key_shift = nullptr;
        
        memset(active_chords, 0, sizeof(active_chords));
        
        // Initialize voice leading
        memset(previous_chord, 0, sizeof(previous_chord));
        previous_chord_size = 0;
        first_chord = true;
        
        // Initialize bar tracking
        beat_count = 0;
        current_key_shift = 0;
        
        if (features) {
            for (int i = 0; features[i]; i++) {
                if (features[i]->URI && !strcmp(features[i]->URI, LV2_URID__map)) {
                    map = (LV2_URID_Map*)features[i]->data;
                    break;
                }
            }
        }
        
        if (map) {
            urids.atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
            urids.atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
            urids.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
        }
    }
    
    void connectPort(uint32_t port, void* data) {
        if (!data) return;
        
        switch (port) {
            case MIDI_IN: midi_in = (const LV2_Atom_Sequence*)data; break;
            case MIDI_OUT: midi_out = (LV2_Atom_Sequence*)data; break;
            case CHAOS_K: chaos_k = (const float*)data; break;
            case CHAOS_INTENSITY: chaos_intensity = (const float*)data; break;
            case CHORD_VELOCITY: chord_velocity = (const float*)data; break;
            case CHORD_CHANNEL: chord_channel = (const float*)data; break;
            case STRANGE_KEY_SHIFT: strange_key_shift = (const float*)data; break;
        }
    }
    
    void run(uint32_t n_samples) {
        if (!midi_in || !midi_out || !map) return;
        
        const uint32_t out_capacity = midi_out->atom.size;
        LV2_Atom_Forge forge;
        lv2_atom_forge_init(&forge, map);
        lv2_atom_forge_set_buffer(&forge, (uint8_t*)midi_out, out_capacity);
        
        LV2_Atom_Forge_Frame seq_frame;
        lv2_atom_forge_sequence_head(&forge, &seq_frame, 0);
        
        LV2_ATOM_SEQUENCE_FOREACH(midi_in, ev) {
            if (ev->body.type == urids.midi_MidiEvent) {
                const uint8_t* const msg = (const uint8_t*)(ev + 1);
                
                if ((msg[0] & 0xF0) == 0x90 && msg[2] > 0) {
                    // Update bar tracking for key shifts
                    updateBarTracking();
                    
                    // Note on - generate chord
                    writeChord(&forge, ev->time.frames, msg[1], true);
                }
                else if ((msg[0] & 0xF0) == 0x80 || ((msg[0] & 0xF0) == 0x90 && msg[2] == 0)) {
                    // Note off - stop active chord notes
                    writeChord(&forge, ev->time.frames, msg[1], false);
                }
            }
        }
        
        lv2_atom_forge_pop(&forge, &seq_frame);
    }
};

extern "C" {

static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double rate,
                             const char* bundle_path, const LV2_Feature* const* features) {
    return new ChordChaos(rate, features);
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    if (instance) ((ChordChaos*)instance)->connectPort(port, data);
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    if (instance) ((ChordChaos*)instance)->run(n_samples);
}

static void cleanup(LV2_Handle instance) {
    if (instance) delete (ChordChaos*)instance;
}

static const LV2_Descriptor descriptor = {
    CHORD_CHAOS_URI, instantiate, connect_port, nullptr, run, nullptr, cleanup, nullptr
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : nullptr;
}

}
