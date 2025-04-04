#include <cstring>
#include <iostream>

class String {
private:
  size_t sz;
  size_t cap;
  char* arr;

  explicit String(size_t count, bool) : sz(count), cap(sz + 1), arr(new char[sz + 1]) {
    arr[count] = '\0';
  }

public:
  String(char symbol) : String(1, true) {
    arr[0] = symbol;
  }

  String(const char* cstr) : String(strlen(cstr), true) {
    memcpy(arr, cstr, strlen(cstr) + 1);
  }

  String(size_t count, char symbol) : String(count, true) {
    memset(arr, symbol, count);
    arr[count] = '\0';
  }

  String() : String(0, true) {
    arr[0] = '\0';
  }

  String(const String& other) : sz(other.sz), cap(other.cap), arr(new char[other.cap]) {
    memcpy(arr, other.arr, other.sz + 1);
  }

  void swap(String& other) {
    std::swap(other.arr, arr);
    std::swap(other.sz, sz);
    std::swap(other.cap, cap);
  }

  String& operator=(const String& other) {
    if (this == &other) {
      return *this;
    }
    String answer(other.sz, true);
    memcpy(answer.data(), other.data(), other.sz + 1);
    swap(answer);
    return *this;
  }

  char& operator[](size_t index) {
    return arr[index];
  }

  const char& operator[](size_t index) const {
    return arr[index];
  }

  size_t length() const {
    return sz;
  }

  size_t size() const {
    return sz;
  }

  size_t capacity() const {
    return cap - 1;
  }

  void push_back(char symbol) {
    if (sz < cap - 1) {
      arr[sz++] = symbol;
      arr[sz] = '\0';
    } else {
      char* cstr = new char[cap * 2];
      memcpy(cstr, arr, sz);
      cstr[sz] = symbol;
      cstr[++sz] = '\0';
      std::swap(arr, cstr);
      cap *= 2;
      delete[] cstr;
    }
  }

  void pop_back() {
    if (sz != 0) {
      arr[--sz] = '\0';
    }
  }

  const char& front() const {
    return arr[0];
  }

  char& front() {
    return arr[0];
  }

  const char& back() const {
    return arr[sz - 1];
  }

  char& back() {
    return arr[sz - 1];
  }

  String& operator+=(const String& other) {
    if (cap >= sz + other.sz + 1) {
      memcpy(&arr[sz], other.arr, other.sz);
      sz += other.sz;
      arr[sz] = '\0';
      return *this;
    }
    char* cstr = new char[2 * (sz + other.sz + 1)];
    memcpy(cstr, arr, sz);
    memcpy(&cstr[sz], other.arr, other.sz);
    std::swap(cstr, arr);
    sz += other.sz;
    cap = 2 * sz + 1;
    arr[sz] = '\0';
    return *this;
  }

  String& operator+=(char symbol) {
    push_back(symbol);
    return *this;
  }

  size_t find(const String& substring) const {
    if (substring.sz > sz) {
      return sz;
    }
    for (size_t i = 1; i <= sz - substring.sz + 1; ++i) {
      if (memcmp(substring.arr, &arr[i - 1], substring.sz) == 0) {
        return i - 1;
      }
    }
    return sz;
  }

  size_t rfind(const String& substring) const {
    if (substring.sz > sz) {
      return sz;
    }
    for (size_t i = sz - substring.sz + 1; i > 0; --i) {
      if (memcmp(substring.arr, &arr[i - 1], substring.sz) == 0) {
        return i - 1;
      }
    }
    return sz;
  }

  String substr(size_t start, size_t count) const {
    count = std::min(count, sz - start);
    String answer(count, true);
    memcpy(answer.arr, &arr[start], count);
    answer[count] = '\0';
    return answer;
  }

  bool empty() const {
    return !sz;
  }

  void clear() {
    sz = 0;
    arr[0] = '\0';
  }

  void shrink_to_fit() {
    char* cstr = new char[sz + 1];
    memcpy(cstr, arr, sz);
    cstr[sz] = '\0';
    std::swap(arr, cstr);
    cap = sz + 1;
    delete[] cstr;
  }

  char* data() {
    return &arr[0];
  }

  const char* data() const {
    return &arr[0];
  }

  ~String() {
    delete[] arr;
  }

  friend std::istream& operator>>(std::istream&, String&);
};

bool operator<(const String& first, const String& second) {
  if (first.size() < second.size()) {
    for (size_t i = 0; i < first.size(); ++i) {
      if (first[i] < second[i]) {
        return true;
      }
      if (first[i] > second[i]) {
        return false;
      }
    }
    return true;
  }
  for (size_t i = 0; i < second.size(); ++i) {
    if (first[i] < second[i]) {
      return true;
    }
    if (first[i] > second[i]) {
      return false;
    }
  }
  return false;
}

bool operator>(const String& first, const String& second) {
  return second < first;
}

bool operator<=(const String& first, const String& second) {
  return !(first > second);
}

bool operator>=(const String& first, const String& second) {
  return !(first < second);
}

bool operator==(const String& first, const String& second) {
  return first >= second && first <= second;
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

String operator+(String first, const String& second) {
  first += second;
  return first;
}

String operator+(char symbol, const String& other) {
  return String(1, symbol) + other;
}

std::istream& operator>>(std::istream& in, String& other) {
  other.clear();
  while (std::isspace(in.peek()) || in.peek() == '\0') {
    in.get();
  }
  size_t now_size = 2;
  while (true) {
    size_t real_size = 0;
    String str(now_size, true);
    for (size_t i = 0; i < now_size; ++i) {
      real_size = i;
      in.get(str[i]);
      if (std::isspace(str[i]) || str[i] == '\0') {
        --real_size;
        str[i] = '\0';
        break;
      }
    }
    if (real_size == now_size - 1) {
      str[now_size] = '\0';
      other += str;
    } else {
      other += str.substr(0, real_size + 1);
      break;
    }
    now_size *= 2;
  }
  return in;
}

std::ostream& operator<<(std::ostream& out, const String& other) {
  out << other.data();
  return out;
}

