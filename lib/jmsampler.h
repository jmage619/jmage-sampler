#ifndef JMSAMPLER_H
#define JMSAMPLER_H

#include "zone.h"
#include "collections.h"
#include "components.h"

#define POLYPHONY 10

class JMSampler {
  private:
    std::vector<jm::zone>* zones;
    // state
    bool sustain_on;
    SoundGenList sound_gens;

    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

  public:
    JMSampler(std::vector<jm::zone>* zones);
    ~JMSampler();
};

#endif
