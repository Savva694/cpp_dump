#include <iostream>
#include <vector>
#include <type_traits>

template<typename T>
class Deque {
public:

  template<bool is_const>
  struct template_iterator {
  private:
    T** global_coord_;
    T* current_chunk_;
    size_t chunk_coord_;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using difference_type = int;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;

    template_iterator() :
            global_coord_(nullptr),
            current_chunk_(nullptr),
            chunk_coord_(0) {}

    template_iterator(T** global_coord, size_t chunk_coord) :
            global_coord_(global_coord),
            current_chunk_(*global_coord),
            chunk_coord_(chunk_coord) {}

    template_iterator(const template_iterator& other) :
            global_coord_(other.global_coord_),
            current_chunk_(other.current_chunk_),
            chunk_coord_(other.chunk_coord_) {}

    operator template_iterator<true>() const {
      return template_iterator<true>(global_coord_, chunk_coord_);
    }

    template_iterator operator=(const template_iterator& other) {
      global_coord_ = other.global_coord_;
      current_chunk_ = other.current_chunk_;
      chunk_coord_ = other.chunk_coord_;
      return *this;
    }

    template_iterator& operator++() {
      *this += 1;
      return *this;
    }

    template_iterator& operator--() {
      *this -= 1;
      return *this;
    }

    template_iterator operator++(int) {
      template_iterator<is_const> copy_iter = *this;
      ++(*this);
      return copy_iter;
    }

    template_iterator operator--(int) {
      template_iterator<is_const> copy_iter = *this;
      --(*this);
      return copy_iter;
    }

    template_iterator& operator+=(int length) {
      if (length < 0) {
        return *this -= -length;
      }
      global_coord_ += (length + chunk_coord_) / chunk;
      if (chunk_coord_ + length >= chunk) {
        current_chunk_ = *global_coord_;
      }
      current_chunk_ = *global_coord_;
      chunk_coord_ = (length + chunk_coord_) % chunk;
      return *this;
    }

    template_iterator& operator-=(int length) {
      if (length < 0) {
        return *this += -length;
      }
      global_coord_ -= length / chunk;
      size_t last_chunk_coord = chunk_coord_;
      if (chunk_coord_ >= length % chunk) {
        chunk_coord_ -= length % chunk;
      } else {
        --global_coord_;
        chunk_coord_ = chunk - length % chunk;
      }
      if (last_chunk_coord < static_cast<size_t>(length)) {
        current_chunk_ = *global_coord_;
      }
      return *this;
    }

    template_iterator operator+(int length) const {
      template_iterator<is_const> copy_iter = *this;
      copy_iter += length;
      return copy_iter;
    }

    template_iterator operator-(int length) const {
      return *this + -length;
    }

    template<bool is_const_other>
    bool operator<(const template_iterator<is_const_other>& other) const {
      if (global_coord_ < other.global_coord_) {
        return true;
      }
      if (global_coord_ > other.global_coord_) {
        return false;
      }
      if (chunk_coord_ < other.chunk_coord_) {
        return true;
      }
      return false;
    }

    template<bool is_const_other>
    bool operator>(const template_iterator<is_const_other>& other) const {
      return other < *this;
    }

    template<bool is_const_other>
    bool operator<=(const template_iterator<is_const_other>& other) const {
      return !(other < *this);
    }

    template<bool is_const_other>
    bool operator>=(const template_iterator<is_const_other>& other) const {
      return !(*this < other);
    }

    template<bool is_const_other>
    bool operator==(const template_iterator<is_const_other>& other) const {
      return *this <= other && other <= *this;
    }

    template<bool is_const_other>
    bool operator!=(const template_iterator<is_const_other>& other) const {
      return !(*this == other);
    }

    template<bool is_const_other>
    int operator-(const template_iterator<is_const_other>& other) const {
      return (global_coord_ - other.global_coord_) * chunk + chunk_coord_ - other.chunk_coord_;
    }

    typename std::conditional<is_const, const T&, T&>::type operator*() const {
      return *(current_chunk_ + chunk_coord_);
    }

    typename std::conditional<is_const, const T*, T*>::type operator->() const {
      return current_chunk_ + chunk_coord_;
    }
  };

  using iterator = template_iterator<false>;
  using const_iterator = template_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  T** global_;
  iterator left_border_;
  iterator right_border_;
  iterator head_;
  iterator tail_;
  static const size_t global_start_size = 1;
  static const size_t chunk = 16;

  void create_left_chunks_in_change_cap(size_t new_left_cap, T** new_global_) {
    size_t cnt = 0;

    try {
      for (size_t i = 0; i < new_left_cap; ++i) {
        new_global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(new_global_[i]);
      }
      delete[] new_global_;
      throw;
    }
  }

  void copy_middle_elements_in_change_cap(size_t new_left_cap, T** new_global_) {
    size_t old_cap = static_cast<size_t>(right_border_ - left_border_) / chunk;
    size_t cnt = new_left_cap;

    try {
      for (size_t i = new_left_cap; i < new_left_cap + old_cap; ++i) {
        new_global_[i] = global_[i - new_left_cap];
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < new_left_cap; ++i) {
        delete[] reinterpret_cast<char*>(new_global_[i]);
      }
      for (size_t i = new_left_cap; i < cnt; ++i) {
        new_global_[i] = global_[i - new_left_cap];
      }
      delete[] new_global_;
      throw;
    }
  }

  void create_right_chunks_in_change_cap(size_t new_left_cap, size_t new_right_cap, T** new_global_) {
    size_t old_cap = static_cast<size_t>(right_border_ - left_border_) / chunk;
    size_t cnt = new_left_cap + old_cap;

    try {
      for (size_t i = new_left_cap + old_cap; i < new_left_cap + old_cap + new_right_cap; ++i) {
        new_global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < new_left_cap; ++i) {
        delete[] reinterpret_cast<char*>(new_global_[i]);
      }
      for (size_t i = new_left_cap; i < new_left_cap + old_cap; ++i) {
        delete[] reinterpret_cast<char*>(new_global_[i]);
      }
      for (size_t i = new_left_cap + old_cap; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(new_global_[i]);
      }
      delete[] new_global_;
      throw;
    }
  }

  void change_cap(size_t new_left_cap, size_t new_right_cap) {
    size_t old_cap = static_cast<size_t>(right_border_ - left_border_) / chunk;
    T** new_global_ = nullptr;
    new_global_ = new T* [new_left_cap + old_cap + new_right_cap];

    create_left_chunks_in_change_cap(new_left_cap, new_global_);
    copy_middle_elements_in_change_cap(new_left_cap, new_global_);
    create_right_chunks_in_change_cap(new_left_cap, new_right_cap, new_global_);

    int last_head = head_ - left_border_;
    int last_tail = tail_ - left_border_;
    left_border_ = iterator(new_global_, 0);
    right_border_ = left_border_ + (new_left_cap + old_cap + new_right_cap) * chunk;
    head_ = left_border_ + new_left_cap * chunk + last_head;
    tail_ = left_border_ + new_left_cap * chunk + last_tail;
    global_ = new_global_;
  }

public:

  Deque() {
    global_ = new T* [global_start_size];
    int cnt = 0;

    try {
      for (size_t i = 0; i < global_start_size; ++i) {
        global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
    }

    left_border_ = iterator(&global_[0], 0);
    right_border_ = iterator(&global_[global_start_size], 0);
    head_ = iterator(&global_[global_start_size / 2], 0);
    tail_ = iterator(&global_[global_start_size / 2], 0);
  }

  Deque(int length, const T& elem) {
    length = static_cast<size_t>(length);
    size_t blocks = 1 + length / chunk;

    global_ = new T* [blocks];
    int cnt = 0;

    try {
      for (size_t i = 0; i < blocks; ++i) {
        global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
    }

    left_border_ = iterator(global_, 0);
    right_border_ = iterator(&global_[blocks], 0);
    head_ = iterator(&global_[0], 0);
    tail_ = head_ + length;
    iterator it = head_;

    try {
      for (; it != tail_; ++it) {
        new(&*it) T(elem);
      }
    } catch (...) {
      --it;
      for (; it >= head_; --it) {
        it->~T();
      }
      for (size_t i = 0; i < blocks; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
      throw;
    }
  }

  Deque(int length) {
    length = static_cast<size_t>(length);
    size_t blocks = 1 + length / chunk;

    global_ = new T* [blocks];
    int cnt = 0;

    try {
      for (size_t i = 0; i < blocks; ++i) {
        global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
    }

    left_border_ = iterator(global_, 0);
    right_border_ = iterator(&global_[blocks], 0);
    head_ = iterator(&global_[0], 0);
    tail_ = head_ + length;
    iterator it = head_;

    try {
      for (; it != tail_; ++it) {
        new(&*it) T();
      }
    } catch (...) {
      --it;
      for (; it >= head_; --it) {
        it->~T();
      }
      for (size_t i = 0; i < blocks; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
      throw;
    }
  }

  Deque(const Deque& other) {
    global_ = new T* [(other.right_border_ - other.left_border_) / chunk];
    int cnt = 0;

    try {
      for (size_t i = 0; i < static_cast<size_t>(other.right_border_ - other.left_border_) / chunk; ++i) {
        global_[i] = reinterpret_cast<T*>(new char[sizeof(T) * chunk]);
        ++cnt;
      }
    } catch (...) {
      for (size_t i = 0; i < cnt; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
      throw;
    }

    int start = other.head_ - other.left_border_;
    iterator iter(&global_[start / chunk], start % chunk);

    cnt = 0;

    try {
      for (int i = start; i < other.tail_ - other.left_border_; ++i) {
        new(&*iter) T(*(other.left_border_ + i));
        ++cnt;
        ++iter;
      }
    } catch (...) {
      for (int i = 0; i < cnt; ++i) {
        iter->~T();
        --iter;
      }
      for (size_t i = 0; i < static_cast<size_t>(other.right_border_ - other.left_border_) / chunk; ++i) {
        delete[] reinterpret_cast<char*>(global_[i]);
      }
      delete[] global_;
      throw;
    }

    left_border_ = iterator(global_, 0);
    right_border_ = iterator(global_ + (other.right_border_ - other.left_border_) / chunk, 0);
    head_ = left_border_ + start;
    tail_ = left_border_ + (other.tail_ - other.left_border_);
  }

  Deque& operator=(const Deque& other) {
    Deque other_cpy = other;
    std::swap(this->global_, other_cpy.global_);
    std::swap(this->left_border_, other_cpy.left_border_);
    std::swap(this->right_border_, other_cpy.right_border_);
    std::swap(this->head_, other_cpy.head_);
    std::swap(this->tail_, other_cpy.tail_);
    return *this;
  }

  size_t size() const {
    return static_cast<size_t>(tail_ - head_);
  }

  T& operator[](size_t index) {
    return *(head_ + index);
  }

  const T& operator[](size_t index) const {
    return *(head_ + index);
  }

  T& at(size_t index) {
    if (index < size()) {
      return *(head_ + index);
    }
    throw std::out_of_range("at threw out_of_range");
  }

  const T& at(size_t index) const {
    if (index < size()) {
      return *(head_ + index);
    }
    throw std::out_of_range("at threw out_of_range");
  }

  void push_back(const T& elem) {
    if (tail_ >= right_border_) {
      change_cap(0, static_cast<size_t>(right_border_ - left_border_) / chunk);
    }

    try {
      new(&*(tail_++)) T(elem);
    } catch (...) {
      --tail_;
      throw;
    }
  }

  void push_front(const T& elem) {
    if (head_ <= left_border_) {
      change_cap(static_cast<size_t>(right_border_ - left_border_) / chunk, 0);
    }

    try {
      new(&*(--head_)) T(elem);
    } catch (...) {
      ++head_;
      throw;
    }
  }

  void pop_back() {
    (tail_ - 1)->~T();
    --tail_;
  }

  void pop_front() {
    head_->~T();
    ++head_;
  }

  iterator begin() {
    return head_;
  }

  const_iterator begin() const {
    return head_;
  }

  iterator end() {
    return tail_;
  }

  const_iterator end() const {
    return tail_;
  }

  const_iterator cbegin() const {
    return head_;
  }

  const_iterator cend() const {
    return tail_;
  }

  reverse_iterator rbegin() {
    return reverse_iterator(tail_);
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(tail_);
  }

  reverse_iterator rend() {
    return reverse_iterator(head_);
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(head_);
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(tail_);
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(head_);
  }

  iterator insert(iterator it, const T& elem) {
    size_t index = it - head_;
    push_back(elem);

    iterator now = tail_ - 1;
    for (; now > head_ + index; --now) {
      std::swap<T>(*now, *(now - 1));
    }
    return head_ + index;
  }

  iterator erase(iterator it) {
    size_t index = it - head_;
    iterator now = head_ + index;
    for (; now < tail_ - 1; ++now) {
      std::swap<T>(*now, *(now + 1));
    }
    tail_->~T();
    --tail_;
    return head_ + index;
  }

  ~Deque() {
    for (iterator it = head_; it < tail_; ++it) {
      it->~T();
    }
    int size = (right_border_ - left_border_) / chunk;
    for (int i = 0; i < size; ++i) {
      delete[] reinterpret_cast<char*>(global_[i]);
    }
    delete[] global_;
  }
};

