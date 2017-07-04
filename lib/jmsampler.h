#ifndef JMSAMPLER_H
#define JMSAMPLER_H

#include "zone.h"
#include "collections.h"
#include "components.h"

#define POLYPHONY 10

class JMSampler {
  private:
    const std::vector<jm::zone>& zones;
    // state
    bool sustain_on;
    SoundGenList sound_gens;

    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

  public:
    JMSampler(const std::vector<jm::zone>& zones, int sample_rate, size_t in_nframes, size_t out_nframes);
    ~JMSampler();
    void pre_process(size_t nframes);
    void handle_note_on(const unsigned char* midi_msg, size_t nframes, size_t curframe);
    void handle_note_off(const unsigned char* midi_msg);
    void handle_sustain(const unsigned char* midi_msg);
    void process_frame(size_t curframe, float amp, float* out1, float* out2);
};

#endif
