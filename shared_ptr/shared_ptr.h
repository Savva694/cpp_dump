#include <iostream>
#include <memory>

template <typename T>
class EnableSharedFromThis;

template <typename T>
class WeakPtr;

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;

  BaseControlBlock() : shared_count(0), weak_count(0) {}

  BaseControlBlock(size_t s_c, size_t w_c) : shared_count(s_c), weak_count(w_c) {}

  virtual void destroy_object() {}

  virtual void deallocate_pointer() {}

  virtual ~BaseControlBlock() = default;
};

template <typename T>
class SharedPtr {
 private:
  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);

  template <typename>
  friend class WeakPtr;

  template <typename>
  friend class SharedPtr;

  template <typename Deleter = std::default_delete<T>, typename Alloc = std::allocator<T>>
  struct ControlBlockRegular : BaseControlBlock {
    T* ptr;
    [[no_unique_address]] Deleter del;
    [[no_unique_address]] Alloc alloc;

    ControlBlockRegular()
        : BaseControlBlock(), ptr(nullptr), del(std::default_delete<T>()), alloc(std::allocator<T>()) {}

    ControlBlockRegular(T* ptr, size_t s_c, size_t w_c)
        : BaseControlBlock(s_c, w_c), ptr(ptr), del(std::default_delete<T>()), alloc(std::allocator<T>()) {}

    ControlBlockRegular(T* ptr, size_t s_c, size_t w_c, Deleter del)
        : BaseControlBlock(s_c, w_c), ptr(ptr), del(del), alloc(std::allocator<T>()) {}

    ControlBlockRegular(T* ptr, size_t s_c, size_t w_c, Deleter del, Alloc alloc)
        : BaseControlBlock(s_c, w_c), ptr(ptr), del(del), alloc(alloc) {}

    void destroy_object() override { del(ptr); }

    void deallocate_pointer() override {
      using BlockAlloc = std::allocator_traits<Alloc>::template rebind_alloc<
          SharedPtr<T>::template ControlBlockRegular<Deleter, Alloc>>;
      using BlockTraits = std::allocator_traits<BlockAlloc>;
      BlockAlloc ba = alloc;
      BlockTraits::deallocate(ba, this, 1);
    }

    ~ControlBlockRegular() override {}
  };

  template <typename U, typename Alloc = std::allocator<U>>
  struct ControlBlockMakeShared : BaseControlBlock {
    U value;
    [[no_unique_address]] Alloc alloc;

    template <typename... Args>
    ControlBlockMakeShared(size_t s_c, size_t w_c, Args&&... args)
        : BaseControlBlock(s_c, w_c), value(U(std::forward<Args>(args)...)), alloc(std::allocator<U>()) {}

    template <typename... Args>
    ControlBlockMakeShared(int*, size_t s_c, size_t w_c, Alloc alloc, Args&&... args)
        : BaseControlBlock(s_c, w_c), value(U(std::forward<Args>(args)...)), alloc(alloc) {}

    void destroy_object() override {
      using UAlloc = std::allocator_traits<Alloc>::template rebind_alloc<U>;
      using UTraits = std::allocator_traits<UAlloc>;
      UAlloc ua = alloc;
      UTraits::destroy(ua, &value);
    }

    void deallocate_pointer() override {
      using BlockAlloc =
          std::allocator_traits<Alloc>::template rebind_alloc<SharedPtr<T>::template ControlBlockMakeShared<U, Alloc>>;
      using BlockTraits = std::allocator_traits<BlockAlloc>;
      BlockAlloc ba = alloc;
      BlockTraits::deallocate(ba, this, 1);
    }

    ~ControlBlockMakeShared() override {}
  };

  T* ptr;
  BaseControlBlock* cb;

  void reduce_counter() {
    if (!cb) {
      return;
    }
    if (--(cb->shared_count) == 0) {
      ptr = nullptr;
      if (cb->weak_count == 0) {
        cb->destroy_object();
        cb->deallocate_pointer();
        cb = nullptr;
      } else {
        cb->destroy_object();
      }
    }
  }

  void init_wp_field() {
    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
      ptr->wp = *this;
    }
  }

  template <typename U, typename Alloc>
  SharedPtr(ControlBlockMakeShared<U, Alloc>* other_cb) : ptr(static_cast<T*>(&(other_cb->value))), cb(other_cb) {
    init_wp_field();
  }

  SharedPtr(T* other_ptr, BaseControlBlock* other_bcb) : ptr(other_ptr), cb(other_bcb) { ++cb->shared_count; }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other, T* new_ptr) : ptr(new_ptr), cb(other.cb) {
    ++cb->shared_count;
    init_wp_field();
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other, T* new_ptr) : ptr(new_ptr), cb(other.cb) {
    other.ptr = nullptr;
    other.cb = nullptr;
    init_wp_field();
  }

 public:
  SharedPtr() : ptr(nullptr), cb(nullptr) {}

  template <typename U>
  SharedPtr(U* ptr)
      : ptr(static_cast<T*>(ptr)), cb(new ControlBlockRegular<std::default_delete<T>, std::allocator<T>>(ptr, 1, 0)) {
    init_wp_field();
  }

  template <typename U, typename Deleter>
  SharedPtr(U* new_ptr, const Deleter& del)
      : ptr(static_cast<T*>(new_ptr)), cb(new ControlBlockRegular<Deleter, std::allocator<T>>(ptr, 1, 0, del)) {
    init_wp_field();
  }

  template <typename U, typename Deleter, typename Alloc>
  SharedPtr(U* new_ptr, const Deleter& del, Alloc alloc) : ptr(static_cast<T*>(new_ptr)) {
    using CBRAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockRegular<Deleter, Alloc>>;
    using CBRTraits = std::allocator_traits<CBRAlloc>;
    CBRAlloc cbralloc = alloc;
    auto cbr = CBRTraits::allocate(cbralloc, 1);
    new (cbr) ControlBlockRegular<Deleter, Alloc>(ptr, 1, 0, del, alloc);
    cb = cbr;
    init_wp_field();
  }

  SharedPtr(const SharedPtr& other) : ptr(other.ptr), cb(other.cb) {
    if (cb) {
      ++cb->shared_count;
    }
    init_wp_field();
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) : ptr(static_cast<T*>(other.ptr)), cb(other.cb) {
    if (cb) {
      ++cb->shared_count;
    }
    init_wp_field();
  }

  SharedPtr(SharedPtr&& other) : ptr(other.ptr), cb(other.cb) {
    other.ptr = nullptr;
    other.cb = nullptr;
    init_wp_field();
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr(static_cast<T*>(other.ptr)), cb(other.cb) {
    other.ptr = nullptr;
    other.cb = nullptr;
    init_wp_field();
  }

  SharedPtr& operator=(const SharedPtr& other) {
    SharedPtr cpy = other;
    swap(cpy);
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    SharedPtr cpy = other;
    swap(cpy);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    SharedPtr cpy = std::move(other);
    swap(cpy);
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    SharedPtr cpy = std::move(other);
    swap(cpy);
    return *this;
  }

  long use_count() const { return cb->shared_count; }

  void reset() {
    reduce_counter();
    ptr = nullptr;
    cb = nullptr;
  }

  template <typename U>
  void reset(U* new_ptr) {
    T* new_T_ptr = static_cast<T*>(new_ptr);
    reduce_counter();
    ptr = new_T_ptr;
    cb = new ControlBlockRegular<typename std::default_delete<U>, typename std::allocator<U>>(ptr, 1, 0);
  }

  template <typename U, typename Deleter>
  void reset(U* new_ptr, const Deleter& del) {
    T* new_T_ptr = static_cast<T*>(new_ptr);
    reduce_counter();
    ptr = new_T_ptr;
    cb = new ControlBlockRegular<Deleter, typename std::allocator<U>>(ptr, 1, 0, del);
  }

  template <typename U, typename Deleter, typename Alloc>
  void reset(U* new_ptr, const Deleter& del, const Alloc& alloc) {
    reduce_counter();
    ptr = static_cast<T*>(new_ptr);
    using CBRTraits = std::allocator_traits<
        typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockRegular<Deleter, Alloc>>>;
    CBRTraits::destroy(alloc, cb);
    CBRTraits::construct(alloc, static_cast<ControlBlockRegular<Deleter, Alloc>*>(cb), ptr, 1, 0, del, alloc);
  }

  T& operator*() const { return *ptr; }

  T* operator->() const { return ptr; }

  T* get() const { return ptr; }

  void swap(SharedPtr& other) {
    std::swap(ptr, other.ptr);
    std::swap(cb, other.cb);
  }

  ~SharedPtr() {
    if (ptr) {
      reduce_counter();
    }
  }
};

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  auto* p = new SharedPtr<T>::template ControlBlockMakeShared<T>(1, 0, std::forward<Args>(args)...);
  return SharedPtr<T>(p);
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using CBM = SharedPtr<T>::template ControlBlockMakeShared<T, Alloc>;
  using BlockAlloc = std::allocator_traits<Alloc>::template rebind_alloc<CBM>;
  using BlockTraits = std::allocator_traits<BlockAlloc>;
  BlockAlloc ba = alloc;
  auto* ptr = BlockTraits::allocate(ba, 1);
  BlockTraits::construct(ba, ptr, nullptr, 1, 0, alloc, std::forward<Args>(args)...);
  return SharedPtr<T>(ptr);
}

template <typename T>
class WeakPtr {
 private:
  template <typename>
  friend class SharedPtr;

  template <typename>
  friend class WeakPtr;

  T* ptr;
  BaseControlBlock* cb;

  void reduce_counter() {
    if (!cb) {
      return;
    }
    if (--(cb->weak_count) == 0 && cb->shared_count == 0) {
      cb->deallocate_pointer();
    }
  }

  template <typename U>
  WeakPtr(U* ptr, BaseControlBlock* cb) : ptr(static_cast<T*>(ptr)), cb(cb) {
    if (cb) {
      ++cb->weak_count;
    }
  }

 public:
  WeakPtr() : ptr(nullptr), cb(nullptr) {}

  template <typename U>
  WeakPtr(const SharedPtr<U>& sp) : ptr(static_cast<T*>(static_cast<U*>(sp.ptr))), cb(sp.cb) {
    if (cb) {
      ++cb->weak_count;
    }
  }

  WeakPtr(const WeakPtr& other) : ptr(other.ptr), cb(other.cb) {
    if (cb) {
      ++cb->weak_count;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : ptr(static_cast<T*>(static_cast<U*>(other.ptr))), cb(other.cb) {
    if (cb) {
      ++cb->weak_count;
    }
  }

  WeakPtr(WeakPtr&& other) : ptr(other.ptr), cb(other.cb) {
    other.ptr = nullptr;
    other.cb = nullptr;
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : ptr(static_cast<T*>(static_cast<U*>(other.ptr))), cb(other.cb) {
    other.ptr = nullptr;
    other.cb = nullptr;
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this == &other) {
      return *this;
    }
    WeakPtr cpy = other;
    swap(cpy);
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    WeakPtr cpy = other;
    swap(cpy);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    WeakPtr cpy = std::move(other);
    swap(cpy);
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    WeakPtr cpy = std::move(other);
    swap(cpy);
    return *this;
  }

  bool expired() const {
    if (cb->shared_count) {
      return false;
    }
    return true;
  }

  long use_count() const { return cb->shared_count; }

  SharedPtr<T> lock() const { return SharedPtr<T>(ptr, cb); }

  T* get() const { return ptr; }

  void swap(WeakPtr& other) {
    std::swap(ptr, other.ptr);
    std::swap(cb, other.cb);
  }

  ~WeakPtr() {
    if (!cb) {
      return;
    }
    reduce_counter();
  }
};

template <typename T>
class EnableSharedFromThis {
 private:
  template <typename>
  friend class SharedPtr;

  WeakPtr<T> wp;

 protected:
  EnableSharedFromThis() {}

 public:
  SharedPtr<T> shared_from_this() {
    if (wp.get() == nullptr) {
      throw std::bad_weak_ptr();
    }
    return wp.lock();
  }
};

