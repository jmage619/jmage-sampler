#ifndef JM_ZONE_LIST_H
#define JM_ZONE_LIST_H

#include <pthread.h>
#include <vector>

#include "jmage/sampler.h"

class JMZoneList {
  private:
    std::vector<jm_key_zone> values;
    pthread_mutex_t mutex;
  public:
    JMZoneList();
    ~JMZoneList();
    void insert(int index, const jm_key_zone& zone);
    const jm_key_zone& get(int index);
    void set(int index, const jm_key_zone& zone);
    void erase(int index);
    std::vector<jm_key_zone>::size_type size();
    void lock();
    void unlock();
};

#endif
