#pragma once
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

class String {
public:
  String() = default;
  String(const char *s) : value_(s ? s : "") {}
  String(const std::string &s) : value_(s) {}
  String(int n) : value_(std::to_string(n)) {}
  String(unsigned n) : value_(std::to_string(n)) {}
  String(unsigned long n) : value_(std::to_string(n)) {}

  const char *c_str() const { return value_.c_str(); }
  size_t length() const { return value_.length(); }
  bool isEmpty() const { return value_.empty(); }
  String substring(size_t start, size_t end) const {
    if (start >= value_.size()) return "";
    return value_.substr(start, end > start ? end - start : 0);
  }
  void trim() {
    auto p = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value_.empty() && p(static_cast<unsigned char>(value_.front())))
      value_.erase(value_.begin());
    while (!value_.empty() && p(static_cast<unsigned char>(value_.back())))
      value_.pop_back();
  }
  bool equalsIgnoreCase(const String &other) const {
    if (value_.size() != other.value_.size()) return false;
    for (size_t i = 0; i < value_.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(value_[i])) !=
          std::tolower(static_cast<unsigned char>(other.value_[i]))) return false;
    return true;
  }
  bool operator==(const String &other) const { return value_ == other.value_; }
  bool operator!=(const String &other) const { return !(*this == other); }
  friend String operator+(const String &left, const String &right) {
    return String(left.value_ + right.value_);
  }

private:
  std::string value_;
};

struct MockSerial {
  void println(const char *) const {}
  template<typename... Args> void printf(const char *, Args...) const {}
};
inline MockSerial Serial;
