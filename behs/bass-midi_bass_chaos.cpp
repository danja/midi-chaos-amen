#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#define BASS_CHAOS_URI "http://github.com/danja/midi-bass-chaos"

enum PortIndex {
    MIDI_IN         = 0,
    MIDI_OUT        = 1,
    CHAOS_K         = 2,
    CHAOS_INTENSITY = 3,
    BASS_VELOCITY   = 4,
    BASS_CHANNEL    = 5,
    REGGAE_MODE     = 6,
    SPARSITY        = 7
};

typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Sequence;
    LV2_URID midi_MidiEvent;
} URIDs;

class BassChaos {
private:
    URIDs urids;
    LV2_URID_Map* map;
    
    double chaos_x;
    int beat_count;
    
    // Reggae intervals (from root)
    int reggae_intervals[6] = {0, -12, 7, -5, 3, -9}; // root, octave down, 5th, 5th down, 3rd, 6th down
    
    // Active notes tracking
    bool active_notes[128];
    uint8_t last_root;
    
    // Ports
    const LV2_Atom_Sequence* midi_in;
    LV2_Atom_Sequence* midi_out;
    const float* chaos_k;
    const float* chaos_intensity;
    const float* bass_velocity;
    const float* bass_channel;
    const float* reggae_mode;
    const float* sparsity;
    
    void generateChaos() {
        double k = chaos_k ? fmax(1.0, fmin(4.0, *chaos_k)) : 3.8;
        chaos_x = k * chaos_x * (1.0 - chaos_x);
        if (chaos_x <= 0.0 || chaos_x >= 1.0) chaos_x = 0.5;
    }
    
    int selectInterval() {
        generateChaos();
        bool reggae = reggae_mode ? (*reggae_mode > 0.5f) : false;
        
        if (reggae) {
            return reggae_intervals[(int)(chaos_x * 5.999)];
        } else {
            // Simple intervals: unison, octave, 5th, 3rd
            int simple_intervals[] = {0, -12, 12, 7, -5, 3, -9};
            return simple_intervals[(int)(chaos_x * 6.999)];
        }
    }
    
    bool shouldTrigger() {
        generateChaos();
        float sparse_level = sparsity ? fmax(0.0f, fmin(1.0f, *sparsity)) : 0.0f;
        
        // Reggae syncopation pattern
        bool reggae = reggae_mode ? (*reggae_mode > 0.5f) : false;
        if (reggae) {
            int beat_pos = beat_count % 8;
            // Classic reggae: emphasize off-beats
            bool is_offbeat = (beat_pos == 1 || beat_pos == 3 || beat_pos == 5 || beat_pos == 7);
            if (is_offbeat && chaos_x > 0.3) return true;
            if (!is_offbeat && chaos_x < 0.7) return false;
        }
        
        return chaos_x > sparse_level;
    }
    
    uint8_t getBassVelocity() {
        generateChaos();
        uint8_t base_vel = bass_velocity ? (uint8_t)fmax(1, fmin(127, *bass_velocity)) : 90;
        
        // Add some velocity variation
        float intensity = chaos_intensity ? fmax(0.0f, fmin(1.0f, *chaos_intensity)) : 0.3f;
        int variation = (int)(chaos_x * intensity * 30) - 15;
        return (uint8_t)fmax(1, fmin(127, base_vel + variation));
    }
    
    uint8_t getBassChannel() {
        return bass_channel ? (uint8_t)fmax(0, fmin(15, *bass_channel)) : 0;
    }
    
    void writeBassNote(LV2_Atom_Forge* forge, uint32_t frames, uint8_t note, uint8_t velocity, bool note_on) {
        if (!forge || note > 127) return;
        
        uint8_t channel = getBassChannel();
        uint8_t midi_msg[3];
        midi_msg[0] = (note_on ? 0x90 : 0x80) | (channel & 0x0F);
        midi_msg[1] = note;
        midi_msg[2] = note_on ? velocity : 0;
        
        lv2_atom_forge_frame_time(forge, frames);
        lv2_atom_forge_atom(forge, 3, urids.midi_MidiEvent);
        lv2_atom_forge_raw(forge, midi_msg, 3);
        lv2_atom_forge_pad(forge, 3);
        
        active_notes[note] = note_on;
    }
    
    void stopActiveNotes(LV2_Atom_Forge* forge, uint32_t frames) {
        for (int i = 0; i < 128; i++) {
            if (active_notes[i]) {
                writeBassNote(forge, frames, i, 0, false);
            }
        }
    }
    
public:
    BassChaos(double rate, const LV2_Feature* const* features) : chaos_x(0.5), beat_count(0), last_root(60) {
        map = nullptr;
        midi_in = nullptr;
        midi_out = nullptr;
        chaos_k = nullptr;
        chaos_intensity = nullptr;
        bass_velocity = nullptr;
        bass_channel = nullptr;
        reggae_mode = nullptr;
        sparsity = nullptr;
        
        memset(active_notes, 0, sizeof(active_notes));
        
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
            case BASS_VELOCITY: bass_velocity = (const float*)data; break;
            case BASS_CHANNEL: bass_channel = (const float*)data; break;
            case REGGAE_MODE: reggae_mode = (const float*)data; break;
            case SPARSITY: sparsity = (const float*)data; break;
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
                    // Note on - generate bass line
                    beat_count++;
                    last_root = msg[1];
                    
                    if (shouldTrigger()) {
                        int interval = selectInterval();
                        int bass_note = last_root + interval;
                        
                        // Keep in bass range (E1 to E4: 28-64)
                        while (bass_note > 64) bass_note -= 12;
                        while (bass_note < 28) bass_note += 12;
                        
                        if (bass_note >= 28 && bass_note <= 64) {
                            writeBassNote(&forge, ev->time.frames, bass_note, getBassVelocity(), true);
                        }
                    }
                }
                else if ((msg[0] & 0xF0) == 0x80 || ((msg[0] & 0xF0) == 0x90 && msg[2] == 0)) {
                    // Note off - stop active bass notes
                    stopActiveNotes(&forge, ev->time.frames);
                }
            }
        }
        
        lv2_atom_forge_pop(&forge, &seq_frame);
    }
};

extern "C" {

static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double rate,
                             const char* bundle_path, const LV2_Feature* const* features) {
    return new BassChaos(rate, features);
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
    if (instance) ((BassChaos*)instance)->connectPort(port, data);
}

static void run(LV2_Handle instance, uint32_t n_samples) {
    if (instance) ((BassChaos*)instance)->run(n_samples);
}

static void cleanup(LV2_Handle instance) {
    if (instance) delete (BassChaos*)instance;
}

static const LV2_Descriptor descriptor = {
    BASS_CHAOS_URI, instantiate, connect_port, nullptr, run, nullptr, cleanup, nullptr
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : nullptr;
}

}
