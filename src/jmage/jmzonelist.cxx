#include <pthread.h>
#include <vector>

#include "jmage/sampler.h"
#include "jmage/jmzonelist.h"

JMZoneList::JMZoneList() {
  pthread_mutex_init(&mutex, NULL);
}

JMZoneList::~JMZoneList() {
  pthread_mutex_destroy(&mutex);
}

void JMZoneList::insert(int index, const jm_key_zone& zone) {
  std::vector<jm_key_zone>::iterator it = values.begin() + index;
  // if index is beyond the end, just put the item at the end
  if (it > values.end())
    it = values.end();

  values.insert(it, zone);
}

const jm_key_zone& JMZoneList::get(int index) {
  return values.at(index);
}

void JMZoneList::set(int index, const jm_key_zone& zone) {
  values.at(index) = zone;
}

void JMZoneList::erase(int index) {
  // use at to induce an exception if out of bounds
  values.at(index);
  values.erase(values.begin() + index);
}

std::vector<jm_key_zone>::size_type JMZoneList::size() {
  return values.size();
}

void JMZoneList::lock() {
  pthread_mutex_lock(&mutex);
}

void JMZoneList::unlock() {
  pthread_mutex_unlock(&mutex);
}

