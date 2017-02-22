#ifndef WAVE_H
#define WAVE_H

namespace jm {
  struct wave {
    float* wave;
    int num_channels;
    int length;
    int left;
    int right;
    int has_loop;
  };

  wave parse_wave(const char* path);
  inline void free_wave(wave& wav) {delete [] wav.wave;}
}

#endif
