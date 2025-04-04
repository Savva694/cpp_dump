#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

template <size_t N>
class StackStorage {
 private:
  char storage[N];
  size_t head;

 public:
  StackStorage() : head(0) {}

  char* alloc(size_t count, size_t align_of, size_t size_of) {
    void* last_ptr = static_cast<void*>(&storage[head]);
    size_t last_space = N - head;
    std::align(align_of, size_of * count, last_ptr, last_space);
    size_t dist = static_cast<char*>(last_ptr) - &storage[head];
    size_t last_head = head;
    head += dist + count * size_of;
    return &storage[last_head + dist];
  }

  StackStorage(const StackStorage<N>& other) = delete;

  template <size_t M>
  StackStorage(const StackStorage<M>& other) = delete;

  StackStorage<N>& operator=(const StackStorage<N>& other) = delete;

  template <size_t M>
  StackStorage<N>& operator=(const StackStorage<M>& other) = delete;

  ~StackStorage() {}
};

template <typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>* storage;

 public:
  using value_type = T;

  StackAllocator() : storage(nullptr) {}

  StackAllocator(StackStorage<N>& storage) : storage(&storage) {}

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) : storage(other.get_storage()) {}

  StackAllocator& operator=(const StackAllocator& other) {
    storage = other.storage;
    return *this;
  }

  bool operator==(const StackAllocator& other) { return storage == other.storage; }

  bool operator!=(const StackAllocator& other) { return storage != other.storage; }

  T* allocate(size_t count) { return reinterpret_cast<T*>((*storage).alloc(count, alignof(T), sizeof(T))); }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  void deallocate(T*, size_t) {}

  StackStorage<N>* get_storage() const { return storage; }

  ~StackAllocator() {}
};

