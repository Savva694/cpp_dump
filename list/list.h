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
  };

  BaseNode fake_node;
  size_t sz;
  using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeTraits = typename std::allocator_traits<NodeAlloc>;
  [[no_unique_address]] NodeAlloc alloc;

  void deallocate_new_nodes(std::vector<Node*>& new_nodes, size_t cnt) {
    for (size_t l = 0; l < cnt; ++l) {
      NodeTraits::deallocate(alloc, new_nodes[l], 1);
    }
  }

  void construct_with_size(size_t size) {
    fake_node = BaseNode();
    sz = 0;
    if (size == 0) {
      return;
    }
    if (size == 1) {
      this->push_back(T());
      return;
    }
    std::vector<Node*> new_nodes(size);

    size_t cnt_l = 0;
    size_t cnt_i = 1;
    try {
      for (size_t l = 0; l < size; ++l) {
        new_nodes[l] = NodeTraits::allocate(alloc, 1);
        ++cnt_l;
      }
      NodeTraits::construct(alloc, new_nodes[0], &fake_node, new_nodes[1]);
      for (size_t i = 1; i < size - 1; ++i) {
        NodeTraits::construct(alloc, new_nodes[i], new_nodes[i - 1], new_nodes[i + 1]);
        ++cnt_i;
      }
      NodeTraits::construct(alloc, new_nodes[size - 1], new_nodes[size - 2], &fake_node);
    } catch (...) {
      for (size_t i = 1; i < cnt_i; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t l = 0; l < cnt_l; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }
    sz = size;
  }

  void construct_with_size_and_value(size_t size, const T& value) {
    fake_node = BaseNode();
    sz = 0;
    if (size == 0) {
      return;
    }
    if (size == 1) {
      this->push_back(T());
      return;
    }
    std::vector<Node*> new_nodes(size);

    size_t cnt_l = 0;
    size_t cnt_i = 1;
    try {
      for (size_t l = 0; l < size; ++l) {
        new_nodes[l] = NodeTraits::allocate(alloc, 1);
        ++cnt_l;
      }
      NodeTraits::construct(alloc, new_nodes[0], &fake_node, new_nodes[1], value);
      for (size_t i = 1; i < size - 1; ++i) {
        NodeTraits::construct(alloc, new_nodes[i], new_nodes[i - 1], new_nodes[i + 1], value);
        ++cnt_i;
      }
      NodeTraits::construct(alloc, new_nodes[size - 1], new_nodes[size - 2], &fake_node, value);
    } catch (...) {
      for (size_t i = 1; i < cnt_i; ++i) {
        NodeTraits::destroy(alloc, new_nodes[i]);
      }
      NodeTraits::destroy(alloc, new_nodes[0]);
      for (size_t l = 0; l < cnt_l; ++l) {
        NodeTraits::deallocate(alloc, new_nodes[l], 1);
      }
      throw;
    }
    sz = size;
  }

  void create_node(BaseNode* left, BaseNode* right, const T& value) {
    Node* new_node = NodeTraits::allocate(alloc, 1);
    try {
      NodeTraits::construct(alloc, new_node, nullptr, nullptr, value);
    } catch (...) {
      NodeTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    new_node->prev = left;
    new_node->next = right;
    left->next = new_node;
    right->prev = new_node;
    ++sz;
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
    for (auto& elem : other) {
      push_back(elem);
    }
  }

  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }
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
    if (this == &other) {
      return *this;
    }
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

  void push_back(const T& value) { create_node(fake_node.prev, &fake_node, value); }

  void push_front(const T& value) { create_node(&fake_node, fake_node.next, value); }

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
    iterator notconst_it = iterator(const_cast<BaseNode*>(it.node));
    create_node(notconst_it.node->prev, notconst_it.node, value);
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
};
