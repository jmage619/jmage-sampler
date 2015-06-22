#ifndef JM_COLLECTIONS_H
#define JM_COLLECTIONS_H

#include <typeinfo>
#include <iostream>

template<class T> class JMQueue {
  private:
    volatile size_t head;
    volatile size_t tail;
    size_t length;
    T* arr;

  public:
    JMQueue(size_t length);
    void add(T& item);
    bool remove();
    bool remove(T& item);
    T* get_head_ptr();
    T* inc_ptr(T* p);
    ~JMQueue();
};

template<class T> JMQueue<T>::JMQueue(size_t length):
  head(0), tail(0), length(length + 1) {
  arr = new T[this->length];
}

template<class T> JMQueue<T>::~JMQueue() {
  delete [] arr;
}

template<class T> void JMQueue<T>::add(T& item) {
  arr[tail] = item;
  tail = (tail + 1) % length;
}

template<class T> bool JMQueue<T>::remove() {
  if (head == tail)
    return false;

  head = (head + 1) % length;
  return true;
}

template<class T> bool JMQueue<T>::remove(T& item) {
  if (head == tail)
    return false;

  item = arr[head];
  head = (head + 1) % length;
  return true;
}

template<class T> T* JMQueue<T>::get_head_ptr() {
  if (head == tail)
    return NULL;
  return arr + head;
}

template<class T> T* JMQueue<T>::inc_ptr(T* p) {
  size_t new_off = (p - arr + 1) % length;
  if (new_off == tail)
    return NULL;
  return arr + new_off;
}

#endif
