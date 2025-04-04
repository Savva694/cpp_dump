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

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;

    BaseNode() : prev(this), next(this) {}

    BaseNode(BaseNode* prev, BaseNode* next) : prev(prev), next(next) {}

    BaseNode(BaseNode&& other) : prev(std::move(other.prev)), next(std::move(other.next)) {
      prev->next = this;
      next->prev = this;
    }

    BaseNode& operator=(BaseNode&& other) {
      prev = std::move(other.prev);
      next = std::move(other.next);
      other.prev = &other;
      other.next = &other;
      prev->next = this;
      next->prev = this;
      return *this;
    }
  };

  struct Node : BaseNode {
    T value;

    Node() : BaseNode(), value() {}

    Node(BaseNode* prev, BaseNode* next) : BaseNode(prev, next), value(T()) {
      prev->next = this;
      next->prev = this;
    }

    Node(BaseNode* prev, BaseNode* next, const T& value) : BaseNode(prev, next), value(value) {}

    Node(BaseNode* prev, BaseNode* next, T&& value) : BaseNode(prev, next), value(std::move(value)) {}

    template <typename Key, typename Value>
    Node(BaseNode* prev, BaseNode* next, Key&& key, Value&& value, size_t hash)
        : BaseNode(prev, next),
          value(std::make_pair(std::make_pair(std::forward<Key>(key), std::forward<Value>(value)), hash)) {}

    Node(Node&& other) : BaseNode(std::move(other)), value(std::move(other.value)) {}

    Node& operator=(Node&& other) {
      this->prev = std::move(other.prev);
      this->next = std::move(other.next);
      this->prev->next = this;
      this->next->prev = this;
      value = std::move(other.value);
      return *this;
    }
  };

  template <bool is_const>
  class template_iterator {
   private:
    typename std::conditional<is_const, const BaseNode*, BaseNode*>::type node;

   public:
    using iterator_category = typename std::bidirectional_iterator_tag;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using difference_type = int;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;

    template_iterator() : node(nullptr) {}

    template_iterator(typename std::conditional<is_const, const BaseNode*, BaseNode*>::type node) : node(node) {}

    template_iterator(const template_iterator& other) : node(other.node) {}

    operator template_iterator<true>() const { return template_iterator<true>(node); }

    template_iterator operator=(const template_iterator& other) {
      node = other.node;
      return *this;
    }

    template_iterator& operator++() {
      node = (*node).next;
      return *this;
    }

    template_iterator& operator--() {
      node = (*node).prev;
      return *this;
    }

    template_iterator operator++(int) {
      template_iterator<is_const> copy_iter = *this;
      node = node->next;
      return copy_iter;
    }

    template_iterator operator--(int) {
      template_iterator<is_const> copy_iter = *this;
      node = node->prev;
      return copy_iter;
    }

    template_iterator& operator+=(int length) {
      for (int i = 0; i < length; ++i) {
        ++*this;
      }
      return *this;
    }

    template_iterator& operator-=(int length) {
      for (int i = 0; i < length; ++i) {
        --*this;
      }
      return *this;
    }

    template_iterator operator+(int length) const {
      template_iterator<is_const> copy_iter = *this;
      copy_iter += length;
      return copy_iter;
    }

    template_iterator operator-(int length) const {
      template_iterator<is_const> copy_iter = *this;
      copy_iter -= length;
      return copy_iter;
    }

    template <bool is_const_other>
    int operator-(const template_iterator<is_const_other>& other) const {
      template_iterator this_cpy = *this;
      template_iterator other_cpy = other;
      int cnt = 0;
      while (true) {
        if (other_cpy == *this) {
          return cnt;
        }
        if (this_cpy == other) {
          return -cnt;
        }
        if (static_cast<typename std::conditional<is_const, const Node*, Node*>::type>(this_cpy.node) != nullptr) {
          ++this_cpy;
        }
        if (static_cast<typename std::conditional<is_const, const Node*, Node*>::type>(other_cpy.node) != nullptr) {
          ++other_cpy;
        }
      }
    }

    template <bool is_const_other>
    bool operator<(const template_iterator<is_const_other>& other) const {
      return *this - other < 0;
    }

    template <bool is_const_other>
    bool operator>(const template_iterator<is_const_other>& other) const {
      return other < *this;
    }

    template <bool is_const_other>
    bool operator<=(const template_iterator<is_const_other>& other) const {
      return !(other < *this);
    }

    template <bool is_const_other>
    bool operator>=(const template_iterator<is_const_other>& other) const {
      return !(*this < other);
    }

    template <bool is_const_other>
    bool operator==(const template_iterator<is_const_other>& other) const {
      return node == other.node;
    }

    template <bool is_const_other>
    bool operator!=(const template_iterator<is_const_other>& other) const {
      return node != other.node;
    }

    typename std::conditional<is_const, const T&, T&>::type operator*() const {
      return (*static_cast<typename std::conditional<is_const, const Node*, Node*>::type>(node)).value;
    }

    typename std::conditional<is_const, const T*, T*>::type operator->() const {
      return &(*static_cast<typename std::conditional<is_const, const Node*, Node*>::type>(node)).value;
    }

    friend class List<T, Alloc>;

    template <class MapKey, class MapValue, class Hash, class Equal, class MapAlloc>
    friend class UnorderedMap;
  };

  BaseNode fake_node;
  size_t sz;
  using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeTraits = typename std::allocator_traits<NodeAlloc>;
  [[no_unique_address]] NodeAlloc alloc;

  void construct_with_size(size_t size) {
    fake_node = BaseNode();
    sz = size;

    if (sz == 0) {
      return;
    }

    if (sz == 1) {
      sz = 0;
      this->push_back(T());
      return;
    }

    std::vector<Node*> new_nodes(sz);

    size_t cnt_l = 0;
    try {
      for (size_t l = 0; l < sz; ++l) {
        new_nodes[l] = NodeTraits::allocate(alloc, 1);
        ++cnt_l;
      }
    } catch (...) {
      for (size_t l = 0; l < cnt_l; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    fake_node.next = new_nodes[0];
    fake_node.prev = new_nodes[sz - 1];

    try {
      NodeTraits::construct(alloc, new_nodes[0], &fake_node, new_nodes[1]);
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    size_t cnt_i = 1;

    try {
      for (size_t i = 1; i < sz - 1; ++i) {
        NodeTraits::construct(alloc, new_nodes[i], new_nodes[i - 1], new_nodes[i + 1]);
        ++cnt_i;
      }
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t i = 1; i < cnt_i; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    try {
      NodeTraits::construct(alloc, new_nodes[sz - 1], new_nodes[sz - 2], &fake_node);
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t i = 1; i < sz - 1; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      NodeTraits::destroy(alloc, new_nodes[sz - 1]);
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }
  }

  void construct_with_size_and_value(size_t size, const T& value) {
    fake_node = BaseNode();
    sz = size;

    if (sz == 0) {
      return;
    }

    if (sz == 1) {
      sz = 0;
      this->push_back(T());
      return;
    }

    std::vector<Node*> new_nodes(sz);

    size_t cnt_l = 0;
    try {
      for (size_t l = 0; l < sz; ++l) {
        new_nodes[l] = NodeTraits::allocate(alloc, 1);
        ++cnt_l;
      }
    } catch (...) {
      for (size_t l = 0; l < cnt_l; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    fake_node.next = new_nodes[0];
    fake_node.prev = new_nodes[sz - 1];

    try {
      NodeTraits::construct(alloc, new_nodes[0], &fake_node, new_nodes[1], value);
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    size_t cnt_i = 1;

    try {
      for (size_t i = 1; i < sz - 1; ++i) {
        NodeTraits::construct(alloc, new_nodes[i], new_nodes[i - 1], new_nodes[i + 1], value);
        ++cnt_i;
      }
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t i = 1; i < cnt_i; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }

    try {
      NodeTraits::construct(alloc, new_nodes[sz - 1], new_nodes[sz - 2], &fake_node, value);
    } catch (...) {
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t i = 1; i < sz - 1; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      NodeTraits::destroy(alloc, new_nodes[sz - 1]);
      for (size_t l = 0; l < sz; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }
  }

 public:
  using iterator = template_iterator<false>;
  using const_iterator = template_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List() : fake_node(BaseNode()), sz(0) {}

  List(size_t size, const T& value) { construct_with_size_and_value(size, value); }

  List(Alloc new_alloc) : fake_node(BaseNode()), sz(0), alloc(new_alloc) {}

  List(size_t size) { construct_with_size(size); }

  List(size_t new_size, Alloc new_alloc) {
    alloc = new_alloc;
    construct_with_size(new_size);
  }

  List(size_t size, const T& value, Alloc new_alloc) {
    alloc = new_alloc;
    construct_with_size_and_value(size, value);
  }

  List(const List& other) : List(NodeTraits::select_on_container_copy_construction(other.alloc)) {
    for (const auto& elem : other) {
      push_back(elem);
    }
  }

  List& operator=(const List& other) {
    List other_cpy = other;
    std::swap(this->fake_node, other_cpy.fake_node);
    std::swap(this->sz, other_cpy.sz);
    if (NodeTraits::propagate_on_container_copy_assignment::value) {
      other_cpy.alloc = alloc;
      alloc = other.alloc;
    }
    return *this;
  }

  List& operator=(List&& other) {
    fake_node = std::move(other.fake_node);
    sz = std::move(other.sz);
    other.sz = 0;
    if (NodeTraits::propagate_on_container_move_assignment::value) {
      alloc = std::move(other.alloc);
    }
    return *this;
  }

  const NodeAlloc& get_allocator() const { return alloc; }

  size_t size() const { return sz; }

  void push_back(const T& value) {
    Node* new_node = nullptr;
    new_node = NodeTraits::allocate(alloc, 1);

    try {
      NodeTraits::construct(alloc, &new_node->value, value);
    } catch (...) {
      NodeTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    if (sz == 0) {
      new_node->next = &fake_node;
      new_node->prev = &fake_node;
      fake_node.next = new_node;
      fake_node.prev = new_node;
      ++sz;
      return;
    }
    new_node->next = &fake_node;
    new_node->prev = fake_node.prev;
    fake_node.prev->next = new_node;
    fake_node.prev = new_node;
    ++sz;
  }

  void push_front(const T& value) {
    Node* new_node = nullptr;
    try {
      new_node = NodeTraits::allocate(alloc, 1);
    } catch (...) {
      NodeTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    try {
      NodeTraits::construct(alloc, new_node, nullptr, nullptr, value);
    } catch (...) {
      NodeTraits::destroy(alloc, &new_node->value);
      NodeTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    NodeTraits::construct(alloc, new_node, nullptr, nullptr, value);
    if (sz == 0) {
      new_node->next = &fake_node;
      new_node->prev = &fake_node;
      fake_node.next = new_node;
      fake_node.prev = new_node;
      ++sz;
      return;
    }
    new_node->next = fake_node.next;
    new_node->prev = &fake_node;
    fake_node.next->prev = new_node;
    fake_node.next = new_node;
    ++sz;
  }

  void pop_back() { erase(end() - 1); }

  void pop_front() { erase(begin()); }

  iterator begin() { return iterator(fake_node.next); }

  const_iterator begin() const { return const_iterator(fake_node.next); }

  iterator end() { return iterator(&fake_node); }

  const_iterator end() const { return const_iterator(&fake_node); }

  const_iterator cbegin() const { return const_iterator(fake_node.next); }

  const_iterator cend() const { return const_iterator(&fake_node); }

  reverse_iterator rbegin() { return reverse_iterator(&fake_node); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(&fake_node); }

  reverse_iterator rend() { return reverse_iterator(iterator(fake_node.next)); }

  const_reverse_iterator rend() const { return const_reverse_iterator(iterator(fake_node.next)); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(&fake_node); }

  const_reverse_iterator crend() const { return const_reverse_iterator(iterator(fake_node.next)); }

  iterator insert(const_iterator it, const T& value) {
    Node* new_node = nullptr;

    try {
      new_node = NodeTraits::allocate(alloc, 1);
    } catch (...) {
      NodeTraits::destroy(alloc, new_node);
      throw;
    }

    iterator notconst_it = iterator(const_cast<BaseNode*>(it.node));

    try {
      NodeTraits::construct(alloc, new_node, notconst_it.node->prev, notconst_it.node, value);
    } catch (...) {
      NodeTraits::deallocate(alloc, new_node, 1);
      NodeTraits::destroy(alloc, new_node);
      throw;
    }

    new_node->next = notconst_it.node;
    new_node->prev = notconst_it.node->prev;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    ++sz;
    return --notconst_it;
  }

  template <typename Key, typename Value>
  iterator insert(const_iterator it, Key&& key, Value&& value, size_t hash) {
    Node* new_node = nullptr;

    try {
      new_node = NodeTraits::allocate(alloc, 1);
    } catch (...) {
      NodeTraits::destroy(alloc, new_node);
      throw;
    }

    iterator notconst_it = iterator(const_cast<BaseNode*>(it.node));

    try {
      NodeTraits::construct(alloc, new_node, notconst_it.node, notconst_it.node->next, std::forward<Key>(key),
                            std::forward<Value>(value), hash);
    } catch (...) {
      NodeTraits::deallocate(alloc, new_node, 1);
      NodeTraits::destroy(alloc, new_node);
      throw;
    }

    new_node->next = notconst_it.node;
    new_node->prev = notconst_it.node->prev;
    new_node->prev->next = new_node;
    new_node->next->prev = new_node;
    ++sz;
    return --notconst_it;
  }

  iterator erase(const_iterator it) {
    iterator notconst_it = iterator(const_cast<BaseNode*>(it.node));
    iterator prev = notconst_it - 1;
    iterator next = notconst_it + 1;
    NodeTraits::destroy(alloc, static_cast<Node*>(notconst_it.node));
    NodeTraits::deallocate(alloc, static_cast<Node*>(notconst_it.node), 1);
    prev.node->next = next.node;
    next.node->prev = prev.node;
    --sz;
    return next;
  }

  void swap(List& other) {
    std::swap<BaseNode>(fake_node, other.fake_node);
    std::swap<size_t>(sz, other.sz);
    if (NodeTraits::propagate_on_container_swap::value) {
      std::swap(alloc, other.alloc);
    }
  }

  ~List() {
    size_t start_sz = sz;
    for (size_t i = 0; i < start_sz; ++i) {
      erase(this->begin());
    }
  }

  template <class MapKey, class MapValue, class Hash, class Equal, class MapAlloc>
  friend class UnorderedMap;
};

template <class Key, class Value, class Hash = std::hash<Key>, class Equal = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using NodeType = std::pair<const Key, Value>;

 private:
  using ListElemType = std::pair<NodeType, size_t>;
  using ListAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ListElemType>;
  List<ListElemType, ListAlloc> list_;

  using ListIter = typename List<ListElemType, ListAlloc>::iterator;
  using ConstListIter = typename List<ListElemType, ListAlloc>::const_iterator;
  std::vector<ListIter> global_;

  double max_load_factor_ = 0.95;

  [[no_unique_address]] Hash hash_;

  [[no_unique_address]] Equal equal_;

  using NonConstNodeType = std::pair<Key, Value>;
  using NonConstAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<NonConstNodeType>;
  using NonConstAllocTraits = typename std::allocator_traits<NonConstAlloc>;
  [[no_unique_address]] NonConstAlloc alloc_;

  void check_max_load_factor() {
    if (max_load_factor_ < load_factor()) {
      reserve(size() * 2 + 1);
    }
  }

  ListIter iter_to_key(const Key& key, size_t hash) {
    ListIter iter = global_[hash % global_.size()];
    if (iter == ListIter()) {
      return list_.end();
    }
    while (iter != list_.end() && (*iter).second % global_.size() == hash % global_.size()) {
      if (equal_(key, (*iter).first.first)) {
        return iter;
      }
      ++iter;
    }
    return list_.end();
  }

  ConstListIter iter_to_key(const Key& key, size_t hash) const {
    ListIter iter = global_[hash % global_.size()];
    if (iter == ListIter()) {
      return list_.end();
    }
    while (iter != list_.end() && (*iter).second % global_.size() == hash % global_.size()) {
      if (equal_(key, (*iter).first.first)) {
        return iter;
      }
      ++iter;
    }
    return list_.end();
  }

 public:
  template <bool is_const>
  class template_iterator {
   private:
    using ListElemType = std::pair<std::pair<const Key, Value>, size_t>;
    using ListAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ListElemType>;
    using ListIterType = typename std::conditional<is_const, typename List<ListElemType, ListAlloc>::const_iterator,
                                                   typename List<ListElemType, ListAlloc>::iterator>::type;
    ListIterType iter;

   public:
    using iterator_category = typename ListIterType::iterator_category;
    using value_type = typename std::conditional<is_const, const NodeType, NodeType>::type;
    using difference_type = typename ListIterType::difference_type;
    using pointer = typename std::conditional<is_const, const NodeType*, NodeType*>::type;
    using reference = typename std::conditional<is_const, const NodeType&, NodeType&>::type;

    template_iterator() : iter(ListIterType()) {}

    template_iterator(ListIterType new_iter) : iter(new_iter) {}

    template_iterator(const template_iterator& other) : iter(other.iter) {}

    operator template_iterator<true>() const { return template_iterator<true>(iter); }

    template_iterator operator=(const template_iterator& other) {
      iter = other.iter;
      return *this;
    }

    template_iterator& operator++() {
      ++iter;
      return *this;
    }

    template_iterator& operator--() {
      --iter;
      return *this;
    }

    template_iterator operator++(int) { return iter++; }

    template_iterator operator--(int) { return iter--; }

    template_iterator& operator+=(int length) { return iter += length; }

    template_iterator& operator-=(int length) { return iter -= length; }

    template_iterator operator+(int length) const { return iter + length; }

    template_iterator operator-(int length) const { return iter - length; }

    template <bool is_const_other>
    int operator-(const template_iterator<is_const_other>& other) const {
      return iter - other.iter;
    }

    template <bool is_const_other>
    bool operator<(const template_iterator<is_const_other>& other) const {
      return iter < other.iter;
    }

    template <bool is_const_other>
    bool operator>(const template_iterator<is_const_other>& other) const {
      return iter > other.iter;
    }

    template <bool is_const_other>
    bool operator<=(const template_iterator<is_const_other>& other) const {
      return iter <= other.iter;
    }

    template <bool is_const_other>
    bool operator>=(const template_iterator<is_const_other>& other) const {
      return iter >= other.iter;
    }

    template <bool is_const_other>
    bool operator==(const template_iterator<is_const_other>& other) const {
      return iter == other.iter;
    }

    template <bool is_const_other>
    bool operator!=(const template_iterator<is_const_other>& other) const {
      return iter != other.iter;
    }

    typename std::conditional<is_const, const NodeType&, NodeType&>::type operator*() const { return (*iter).first; }

    typename std::conditional<is_const, const NodeType*, NodeType*>::type operator->() const { return &(iter->first); }
  };

  using iterator = template_iterator<false>;
  using const_iterator = template_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  UnorderedMap() : list_(List<ListElemType, ListAlloc>(alloc_)), global_(std::vector<ListIter>(1)) {}

  UnorderedMap(size_t bucket_count) : UnorderedMap() { global_.resize(bucket_count); }

  UnorderedMap(size_t bucket_count, const Hash& hash, const Equal& equal, const Alloc& alloc) {
    hash_ = hash;
    equal_ = equal;
    if (NonConstAllocTraits::select_on_container_copy_construction()) {
      alloc_ = alloc;
    }
    list_ = List<ListElemType, Alloc>(0, alloc_);
    global_ = std::vector<ListIter>(bucket_count);
  }

  UnorderedMap(size_t bucket_count, const Alloc& alloc) : UnorderedMap(bucket_count, Hash(), Equal(), alloc) {}

  UnorderedMap(size_t bucket_count, const Hash& hash, const Alloc& alloc)
      : UnorderedMap(bucket_count, hash, Equal(), alloc) {}

  UnorderedMap(const UnorderedMap& other)
      : list_(other.list_), max_load_factor_(other.max_load_factor_), hash_(other.hash_), equal_(other.equal_) {
    global_.resize(other.global_.size());
    ListIter iter = list_.begin();
    for (; iter != list_.end(); ++iter) {
      if (global_[(*iter).second % global_.size()] == ListIter()) {
        global_[(*iter).second % global_.size()] = iter;
      }
    }
    if (NonConstAllocTraits::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
  }

  UnorderedMap(UnorderedMap&& other) {
    list_ = std::move(other.list_);
    global_ = std::move(other.global_);
    max_load_factor_ = std::move(other.max_load_factor_);
    hash_ = std::move(other.hash_);
    equal_ = std::move(other.equal_);
    if (NonConstAllocTraits::propagate_on_container_copy_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    UnorderedMap other_cpy = std::move(other);
    swap(other_cpy);
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) {
    list_ = std::move(other.list_);
    global_ = std::move(other.global_);
    max_load_factor_ = std::move(other.max_load_factor_);
    hash_ = std::move(other.hash_);
    equal_ = std::move(other.equal_);
    if (NonConstAllocTraits::propagate_on_container_move_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }
    return *this;
  }

  Value& operator[](const Key& key) {
    size_t hash = hash_(key);
    ListIter iter = iter_to_key(key, hash);
    if (iter != list_.end()) {
      return (*iter).first.second;
    }
    if (global_[hash % (global_.size())] != ListIter()) {
      iter = list_.insert(global_[hash % (global_.size())], {{key, Value()}, hash});
      global_[hash % (global_.size())] = iter;
      return (*(iter)).first.second;
    }
    iter = list_.insert(iter, {{key, Value()}, hash});
    global_[hash % (global_.size())] = iter;
    check_max_load_factor();
    return (*(iter)).first.second;
  }

  Value& at(const Key& key) {
    size_t hash = hash_(key);
    ListIter iter = iter_to_key(key, hash);
    if (iter != list_.end()) {
      return (*iter).first.second;
    }
    throw("at out of range");
  }

  const Value& at(const Key& key) const {
    size_t hash = hash_(key);
    ConstListIter iter = iter_to_key(key, hash);
    if (iter != list_.end()) {
      return (*iter).first.second;
    }
    throw("at out of range");
  }

  size_t size() const { return list_.size(); }

  iterator begin() { return iterator(list_.begin()); }

  const_iterator begin() const { return const_iterator(list_.begin()); }

  iterator end() { return iterator(list_.end()); }

  const_iterator end() const { return const_iterator(list_.end()); }

  const_iterator cbegin() const { return const_iterator(list_.cbegin()); }

  const_iterator cend() const { return const_iterator(list_.cend()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

  const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

 private:
  template <typename pair_type>
  std::pair<iterator, bool> insert_helper(pair_type&& pair) {
    size_t hash = hash_(pair.first);
    ListIter iter = iter_to_key(pair.first, hash);
    if (iter != list_.end()) {
      return std::make_pair(iter, false);
    }
    if (global_[hash % global_.size()] != ListIter()) {
      iter = list_.insert(global_[hash % global_.size()], std::forward<pair_type>(pair).first,
                          std::forward<pair_type>(pair).second, hash);
    } else {
      iter = list_.insert(iter, std::forward<pair_type>(pair).first, std::forward<pair_type>(pair).second, hash);
    }
    global_[hash % (global_.size())] = iter;
    check_max_load_factor();
    return {iter, true};
  }

 public:
  std::pair<iterator, bool> insert(const std::pair<Key, Value>& new_node) { return insert_helper(new_node); }

  std::pair<iterator, bool> insert(std::pair<Key, Value>&& new_node) { return insert_helper(std::move(new_node)); }

  template <typename InputIt>
  void insert(InputIt first, InputIt second) {
    for (; first != second; ++first) {
      insert(std::forward<std::pair<Key, Value>>(*first));
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    NonConstNodeType* new_node_ptr = NonConstAllocTraits::allocate(alloc_, 1);
    NonConstAllocTraits::construct(alloc_, new_node_ptr, std::forward<Args>(args)...);
    size_t hash = hash_((*new_node_ptr).first);
    ListIter iter = iter_to_key((*new_node_ptr).first, hash);
    if (iter != list_.end()) {
      return std::make_pair(iter, false);
    }
    if (global_[hash % global_.size()] != ListIter()) {
      iter = list_.insert(global_[hash % global_.size()], std::forward<Key>((*new_node_ptr).first),
                          std::forward<Value>((*new_node_ptr).second), hash);
    } else {
      iter = list_.insert(iter, std::forward<Key>((*new_node_ptr).first), std::forward<Value>((*new_node_ptr).second),
                          hash);
    }
    NonConstAllocTraits::destroy(alloc_, new_node_ptr);
    NonConstAllocTraits::deallocate(alloc_, new_node_ptr, 1);
    global_[hash % (global_.size())] = iter;
    check_max_load_factor();
    return {iter, true};
  }

  iterator erase(const Key& key) {
    size_t hash = hash_(key);
    ListIter list_iter = iter_to_key(key, hash);
    ListIter list_next_iter = list_iter + 1;
    if (list_iter != list_.end()) {
      if (list_next_iter != list_.end() && (*list_next_iter).second % global_.size() == hash % global_.size()) {
        global_[hash % (global_.size())] = list_next_iter;
      } else {
        global_[hash % (global_.size())] = ListIter();
      }
      list_.erase(list_iter);
    }
    return list_next_iter;
  }

  iterator erase(iterator iter) { return erase((*iter).first); }

  template <typename InputIt>
  void erase(InputIt first, InputIt second) {
    for (; first != second;) {
      ++first;
      erase((*(first - 1)).first);
    }
  }

  const_iterator find(const Key& key) const { return const_iterator(iter_to_key(key, hash_(key))); }

  iterator find(const Key& key) { return iterator(iter_to_key(key, hash_(key))); }

  void reserve(size_t count) {
    size_t new_global_sz = (count + max_load_factor_ - 1) / max_load_factor_;
    if (list_.size() == 0) {
      global_.resize(new_global_sz);
      return;
    }
    std::vector<ListIter> new_global(new_global_sz);
    using Node = typename List<ListElemType, ListAlloc>::Node;
    using BaseNode = typename List<ListElemType, ListAlloc>::BaseNode;

    BaseNode* elem = list_.fake_node.next;
    list_.fake_node.prev = &list_.fake_node;
    list_.fake_node.next = &list_.fake_node;

    while (elem != &list_.fake_node) {
      Node* node_elem = static_cast<Node*>(elem);
      BaseNode* after_elem = elem->next;

      BaseNode* place = nullptr;
      if (new_global[node_elem->value.second % new_global_sz] == ListIter()) {
        place = &list_.fake_node;
      } else {
        place = new_global[node_elem->value.second % new_global_sz].node;
      }
      elem->next = place;
      elem->prev = (place)->prev;
      elem->prev->next = elem;
      elem->next->prev = elem;
      new_global[node_elem->value.second % new_global_sz] = ListIter(elem);

      elem = after_elem;
    }

    global_.swap(new_global);
  }

  float load_factor() const { return list_.size() / global_.size(); }

  void max_load_factor(double mlf) { max_load_factor_ = mlf; }

  void swap(UnorderedMap& other) {
    list_.swap(other.list_);
    global_.swap(other.global_);
    std::swap(max_load_factor_, other.max_load_factor_);
    std::swap(hash_, other.hash_);
    std::swap(equal_, other.equal_);
    if (NonConstAllocTraits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }
};

