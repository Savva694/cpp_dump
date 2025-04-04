#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include <cassert>

enum Sign: int8_t {
  minus = -1, zero = 0, plus = 1
};

class BigInteger {
private:
  void DeleteExtraZeros() {
    size_t length = nums_.size();
    for (size_t i = length; i > 0; --i) {
      if (nums_[i - 1] != 0) {
        return;
      }
      nums_.pop_back();
    }
    *this = 0;
  }

public:
  static const int ns_ = 1e9;
  Sign sign_;
  std::vector<int> nums_;

  BigInteger() : BigInteger(0) {}

  BigInteger(int other) {
    if (other == 0) {
      sign_ = zero;
      nums_.push_back(0);
    } else if (other > 0) {
      sign_ = plus;
      while (other > 0) {
        nums_.push_back(other % ns_);
        other /= ns_;
      }
    } else {
      *this = BigInteger(-other);
      sign_ = minus;
    }
  }

  explicit BigInteger(int64_t other) {
    if (other == 0) {
      sign_ = zero;
      nums_.push_back(0);
    } else if (other > 0) {
      sign_ = plus;
      std::string str;
      while (other > 0) {
        nums_.push_back(other % static_cast<int64_t>(ns_));
        other /= ns_;
      }
    } else {
      *this = BigInteger(-other);
      sign_ = minus;
    }
  }

  explicit BigInteger(unsigned long long other) {
    if (other == 0) {
      sign_ = zero;
      nums_.push_back(0);
    } else {
      sign_ = plus;
      std::string str;
      while (other > 0) {
        nums_.push_back(other % static_cast<int64_t>(ns_));
        other /= ns_;
      }
    }
  }

  BigInteger& operator+=(const BigInteger& other) {
    if (other.sign_ == zero) {
      return *this;
    }
    if (other.sign_ == minus) {
      return *this -= -other;
    }
    if (sign_ == zero) {
      *this = other;
      return *this;
    }
    if (sign_ == minus) {
      BigInteger temp = -*this;
      temp -= other;
      *this = -temp;
      return *this;
    }
    size_t index = 0;
    bool flag = false;
    while (true) {
      if (index < nums_.size() && index < other.nums_.size()) {
        if (flag) {
          if (nums_[index] + other.nums_[index] + 1 >= ns_) {
            flag = true;
            nums_[index] = nums_[index] + other.nums_[index] + 1 - ns_;
          } else {
            flag = false;
            nums_[index] = nums_[index] + other.nums_[index] + 1;
          }
        } else {
          if (nums_[index] + other.nums_[index] >= ns_) {
            flag = true;
            nums_[index] = nums_[index] + other.nums_[index] - ns_;
          } else {
            flag = false;
            nums_[index] = nums_[index] + other.nums_[index];
          }
        }
      } else if (index < nums_.size() && index >= other.nums_.size()) {
        if (flag) {
          if (nums_[index] + 1 >= ns_) {
            flag = true;
            nums_[index] = nums_[index] + 1 - ns_;
          } else {
            flag = false;
            nums_[index] = nums_[index] + 1;
          }
        } else {
          break;
        }
      } else if (index >= nums_.size() && index < other.nums_.size()) {
        if (flag) {
          if (other.nums_[index] + 1 >= ns_) {
            flag = true;
            nums_.push_back(other.nums_[index] + 1 - ns_);
          } else {
            flag = false;
            nums_.push_back(other.nums_[index] + 1);
          }
        } else {
          nums_.push_back(other.nums_[index]);
        }
      } else {
        if (flag) {
          nums_.push_back(1);
        }
        break;
      }
      ++index;
    }
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other) {
    if (other.sign_ == zero) {
      return *this;
    }
    if (other.sign_ == minus) {
      return *this += -other;
    }
    if (sign_ == zero) {
      *this = -other;
      return *this;
    }
    if (sign_ == minus) {
      BigInteger temp = -*this;
      temp += other;
      *this = -temp;
      return *this;
    }
    if (*this == other) {
      *this = 0;
      return *this;
    }
    if (*this < other) {
      BigInteger temp = other;
      temp -= *this;
      *this = -temp;
      return *this;
    }
    size_t index = 0;
    bool flag = false;
    while (true) {
      if (index < other.nums_.size()) {
        if (flag) {
          if (nums_[index] - other.nums_[index] - 1 >= 0) {
            flag = false;
            nums_[index] = nums_[index] - other.nums_[index] - 1;
          } else {
            flag = true;
            nums_[index] = nums_[index] - other.nums_[index] - 1 + ns_;
          }
        } else {
          if (nums_[index] - other.nums_[index] >= 0) {
            flag = false;
            nums_[index] = nums_[index] - other.nums_[index];
          } else {
            flag = true;
            nums_[index] = nums_[index] - other.nums_[index] + ns_;
          }
        }
      } else {
        if (flag) {
          if (nums_[index] - 1 >= 0) {
            flag = false;
            nums_[index] = nums_[index] - 1;
            break;
          }
          flag = true;
          nums_[index] = nums_[index] - 1 + ns_;
        } else {
          break;
        }
      }
      ++index;
    }
    DeleteExtraZeros();
    return *this;
  }

  BigInteger& operator*=(const BigInteger& other) {
    if (sign_ == zero || other.sign_ == zero) {
      if (other.sign_ == zero) {
        *this = other;
      }
      return *this;
    }
    std::vector<int64_t> rez;
    for (size_t i = 0; i < other.nums_.size(); ++i) {
      int64_t from_last = 0;
      std::vector<int64_t> temp;
      for (size_t j = 0; j < nums_.size(); ++j) {
        if (i + j < rez.size()) {
          int64_t num = static_cast<int64_t>(other.nums_[i]) * nums_[j] + from_last + rez[i + j];
          rez[i + j] = num % ns_;
          from_last = num / ns_;
        } else {
          int64_t num = static_cast<int64_t>(other.nums_[i]) * nums_[j] + from_last;
          rez.push_back(num % ns_);
          from_last = num / ns_;
        }
      }
      if (from_last != 0) {
        rez.push_back(from_last);
      }
    }
    if ((sign_ == minus && other.sign_ == plus) || (sign_ == plus && other.sign_ == minus)) {
      sign_ = minus;
    } else {
      sign_ = plus;
    }
    nums_.clear();
    for (size_t i = 0; i < rez.size(); ++i) {
      nums_.push_back(static_cast<int>(rez[i]));
    }
    return *this;
  }

  BigInteger& operator/=(const BigInteger&);

  BigInteger& operator%=(const BigInteger&);

  void DivideTen() {
    int deg = ns_ / 10;
    nums_[0] /= 10;
    for (size_t i = 1; i < nums_.size(); ++i) {
      int num = nums_[i] % 10;
      nums_[i - 1] += num * deg;
      nums_[i] /= 10;
    }
    DeleteExtraZeros();
  }

  BigInteger operator-() const {
    BigInteger answer = *this;
    if (sign_ == plus) {
      answer.sign_ = minus;
    }
    if (sign_ == minus) {
      answer.sign_ = plus;
    }
    return answer;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger copy = *this;
    *this += 1;
    return copy;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger copy = *this;
    *this -= 1;
    return copy;
  }

  bool operator==(const BigInteger& other) const {
    if (sign_ != other.sign_ || nums_.size() != other.nums_.size()) {
      return false;
    }
    for (size_t i = nums_.size(); i >= 1; --i) {
      if (nums_[i - 1] != other.nums_[i - 1]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const BigInteger& other) const { return !(*this == other); }

  std::strong_ordering Comparison(const BigInteger& other) const {
    if (nums_.size() < other.nums_.size()) {
      return std::strong_ordering::less;
    }
    if (nums_.size() > other.nums_.size()) {
      return std::strong_ordering::greater;
    }
    for (size_t i = nums_.size(); i > 0; --i) {
      if (nums_[i - 1] > other.nums_[i - 1]) {
        return std::strong_ordering::greater;
      }
      if (nums_[i - 1] < other.nums_[i - 1]) {
        return std::strong_ordering::less;
      }
    }
    return std::strong_ordering::equal;
  }

  std::strong_ordering operator<=>(const BigInteger& other) const {
    if (sign_ < other.sign_) {
      return std::strong_ordering::less;
    }
    if (sign_ > other.sign_) {
      return std::strong_ordering::greater;
    }
    if (sign_ == zero) {
      return std::strong_ordering::equal;
    }
    std::strong_ordering answer = Comparison(other);
    if (sign_ == plus) {
      return answer;
    }
    if (answer == std::strong_ordering::equal) {
      return std::strong_ordering::equal;
    }
    if (answer == std::strong_ordering::less) {
      return std::strong_ordering::greater;
    }
    return std::strong_ordering::less;
  }

  std::string toString() const {
    std::string ans;
    if (sign_ == minus) {
      ans += "-";
    }
    std::string temp = std::to_string(nums_[nums_.size() - 1]);
    ans += temp;
    if (nums_.size() >= 2) {
      for (size_t i = nums_.size() - 1; i > 0; --i) {
        temp = std::to_string(nums_[i - 1]);
        std::string nulls(static_cast<size_t>(log10(ns_)) - temp.size(), '0');
        ans += nulls;
        ans += temp;
      }
    }
    return ans;
  }

  explicit operator bool() const {
    return sign_ != zero;
  }

  BigInteger Abs() const {
    BigInteger ans = *this;
    if (ans.sign_ == minus) {
      ans.sign_ = plus;
    }
    return ans;
  }

  friend std::istream& operator>>(std::istream& in, BigInteger& other);

  void Swap(BigInteger& other) {
    std::swap(sign_, other.sign_);
    std::swap(nums_, other.nums_);
  }
};

int Atoi(std::string& str) {
  int ans = 0;
  int deg = 1;
  for (size_t i = str.size(); i > 0; --i) {
    ans += deg * (str[i - 1] - '0');
    deg *= 10;
  }
  return ans;
}

std::istream& operator>>(std::istream& in, BigInteger& other) {
  other.nums_.clear();
  std::string str;
  in >> str;
  size_t len = str.size();
  if (str[0] == '-') {
    other.sign_ = minus;
    --len;
  } else {
    other.sign_ = plus;
  }
  if (str[0] == '0') {
    other = 0;
    return in;
  }
  size_t sz = static_cast<size_t>(log10(other.ns_));
  size_t index = -1;
  for (size_t i = str.size(); i >= str.size() - len + sz; i -= sz) {
    index = i - sz;
    std::string temp = str.substr(i - sz, sz);
    other.nums_.push_back(Atoi(temp));
  }
  if (index != 0) {
    std::string temp = str.substr(str.size() - len, index - (str.size() - len));
    other.nums_.push_back(Atoi(temp));
  }
  return in;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& other) {
  out << other.toString();
  return out;
}

BigInteger operator""_bi(unsigned long long number) {
  BigInteger ans(number);
  return ans;
}

void swap(BigInteger& first, BigInteger& second) {
  first.Swap(second);
}

BigInteger operator+(const BigInteger& first, const BigInteger& other) {
  BigInteger temp = first;
  temp += other;
  return temp;
}

BigInteger operator-(const BigInteger& first, const BigInteger& other) {
  BigInteger temp = first;
  temp -= other;
  return temp;
}

BigInteger operator*(const BigInteger& first, const BigInteger& other) {
  BigInteger temp = first;
  temp *= other;
  return temp;
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
  if (other.sign_ == zero || sign_ == zero) {
    return *this;
  }
  BigInteger positive_other = other.Abs();
  BigInteger ten = 10;
  BigInteger deg_ten = 1;
  int count = 0;
  BigInteger abs_this = Abs();
  while (positive_other * ten <= abs_this) {
    positive_other *= ten;
    deg_ten *= ten;
    ++count;
  }
  BigInteger copy = abs_this;
  Sign sign = sign_;
  *this = 0;
  for (int i = 0; i < count; ++i) {
    while (copy >= positive_other) {
      copy -= positive_other;
      *this += deg_ten;
    }
    positive_other.DivideTen();
    deg_ten.DivideTen();
  }
  while (copy >= positive_other) {
    copy -= positive_other;
    *this += deg_ten;
  }
  if (*this == 0) {
    return *this;
  }
  if ((sign == minus && other.sign_ == plus) || (sign == plus && other.sign_ == minus)) {
    sign_ = minus;
  } else {
    sign_ = plus;
  }
  return *this;
}

BigInteger operator/(const BigInteger& first, const BigInteger& other) {
  BigInteger temp = first;
  temp /= other;
  return temp;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
  BigInteger temp = *this;
  temp /= other;
  *this -= temp * other;
  return *this;
}

BigInteger operator%(const BigInteger& first, const BigInteger& other) {
  BigInteger temp = first;
  temp %= other;
  return temp;
}


class Rational {
private:
  BigInteger up_;
  BigInteger low_;

  void MakeMutuallySimple() {
    BigInteger null = 0;
    if (up_ == null) {
      low_ = 1;
      return;
    }
    BigInteger up_copy = up_;
    BigInteger low_copy = low_;
    while (up_copy != null && low_copy != null) {
      if (up_copy.Abs() > low_copy.Abs()) {
        up_copy %= low_copy;
      } else {
        low_copy %= up_copy;
      }
    }
    BigInteger gcd;
    if (up_copy != null) {
      gcd = up_copy;
    } else {
      gcd = low_copy;
    }
    up_ /= gcd;
    low_ /= gcd;
    if (low_ < null) {
      low_ = - low_;
      up_ = -up_;
    }
  }

public:
  Rational(const BigInteger& other) {
    up_ = other;
    low_ = 1;
  }

  Rational(int other) : up_(other), low_(1) {}

  Rational() : up_(0), low_(1) {}

  Rational& operator+=(const Rational& other) {
    up_ = up_ * other.low_ + low_ * other.up_;
    low_ = low_ * other.low_;
    MakeMutuallySimple();
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    *this += -other;
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    up_ *= other.up_;
    low_ *= other.low_;
    MakeMutuallySimple();
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    up_ *= other.low_;
    low_ *= other.up_;
    MakeMutuallySimple();
    return *this;
  }

  bool operator==(const Rational& other) const {
    return up_ == other.up_ && low_ == other.low_;
  }

  bool operator!=(const Rational& other) const {
    return !(*this == other);
  }

  std::strong_ordering operator<=>(const Rational& other) const {
    BigInteger first = up_ * other.low_;
    BigInteger second = low_ * other.up_;
    if (first < second) {
      return std::strong_ordering::less;
    }
    if (first > second) {
      return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
  }

  std::string toString() const {
    std::string ans;
    ans += up_.toString();
    BigInteger one = 1;
    if (low_ != one) {
      ans += "/";
      ans += low_.toString();
    }
    return ans;
  }

  std::string asDecimal(const size_t precision = 0) const {
    BigInteger temp = up_;
    for (size_t i = 0; i < precision; ++i) {
      temp *= 10;
    }
    temp /= low_;
    std::string ans = temp.toString();

    if (ans[0] == '-') {
      ans = ans.substr(1, ans.size() - 1);
    }
    std::string ans2;
    if (ans.size() > precision) {
      if (temp < 0) {
        ans2 += "-";
      }
      ans2 += ans.substr(0, ans.size() - precision);
      ans2 += ".";
      ans2 += ans.substr(ans.size() - precision, precision);

      return ans2;
    }
    if (temp < 0) {
      ans2 += "-";
    }
    ans2 += "0.";
    for (size_t i = 0; i < precision - ans.size(); ++i) {
      ans2 += "0";
    }
    ans2 += ans;
    return ans2;
  }

  explicit operator double() const {
    Rational copy = *this;
    BigInteger ten_deg = static_cast<int>(1e9);
    ten_deg *= static_cast<int>(1e9);
    double deg = 1;
    while (copy < ten_deg) {
      deg *= 10;
      copy *= 10;
    }
    std::string str = (copy.up_ / copy.low_).toString();
    double ans = 0;
    double deg_ten = 1;
    for (size_t i = str.size(); i > 0; --i) {
      ans += deg_ten * (str[i - 1] - '0');
      deg_ten *= 10;
    }
    ans /= deg;
    return ans;
  }

  Rational operator-() const {
    Rational answer = *this;
    answer.up_ = -answer.up_;
    return answer;
  }
};

Rational operator+(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans += second;
  return ans;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans -= second;
  return ans;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans *= second;
  return ans;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans /= second;
  return ans;
}

