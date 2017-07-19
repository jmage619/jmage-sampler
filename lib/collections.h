#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <typeinfo>
#include <iostream>
#include <stdexcept>

template<class T> class JMStack {
  private:
    ssize_t head;
    T* arr;

  public:
    JMStack(size_t length);
    ~JMStack();
    void push(const T& item);
    T pop();
    size_t size();
};

template<class T> JMStack<T>::JMStack(size_t length):
    head(-1) {
  arr = new T[length];
}

template<class T> JMStack<T>::~JMStack() {
  delete [] arr;
}

template<class T> void JMStack<T>::push(const T& item) {
  arr[++head] = item;
}

template<class T> T JMStack<T>::pop() {
  if (head < 0)
    throw std::runtime_error("empty stack!");
  return arr[head--];
}

template<class T> size_t JMStack<T>::size() {
  return head + 1;
}

template<class T> class JMQueue {
  private:
    volatile size_t head;
    volatile size_t tail;
    size_t length;
    T* arr;

  public:
    JMQueue(size_t length);
    void add(const T& item);
    T remove();
    T* get_head_ptr();
    T* inc_ptr(T const * p);
    bool empty();
    ~JMQueue();
};

template<class T> JMQueue<T>::JMQueue(size_t length):
  head(0), tail(0), length(length + 1) {
  arr = new T[this->length];
}

template<class T> JMQueue<T>::~JMQueue() {
  delete [] arr;
}

template<class T> void JMQueue<T>::add(const T& item) {
  arr[tail] = item;
  tail = (tail + 1) % length;
}

template<class T> T JMQueue<T>::remove() {
  if (head == tail)
    throw std::runtime_error("empty queue!");

  T& item = arr[head];
  head = (head + 1) % length;
  return item;
}

template<class T> T* JMQueue<T>::get_head_ptr() {
  if (head == tail)
    return NULL;
  return arr + head;
}

template<class T> T* JMQueue<T>::inc_ptr(T const * p) {
  size_t new_off = (p - arr + 1) % length;
  if (new_off == tail)
    return NULL;
  return arr + new_off;
}

template<class T> bool JMQueue<T>::empty() {
  return head == tail;
}

#endif
