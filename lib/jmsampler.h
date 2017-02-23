#ifndef JMSAMPLER_H
#define JMSAMPLER_H

#include "zone.h"
#include "collections.h"
#include "components.h"

#define POLYPHONY 10

class JMSampler {
  private:
    // this could be constant
    std::vector<jm::zone>& zones;
    // state
    bool sustain_on;
    SoundGenList sound_gens;

    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

  public:
    JMSampler(std::vector<jm::zone>& zones);
    ~JMSampler();
    void pre_process(size_t nframes);
    void handle_note_on(const char* midi_msg, size_t nframes, size_t curframe);
    void handle_note_off(const char* midi_msg);
    void handle_sustain(const char* midi_msg);
    void process_frame(size_t curframe, float amp, float* out1, float* out2);
};

#endif
