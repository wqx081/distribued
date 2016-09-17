#include "core/strings/string_piece.h"
#include <limits.h>

#include <algorithm>
#include <ostream>

#include <glog/logging.h>

namespace mr {

namespace {

inline void BuildLookupTable(const StringPiece& characters_wanted,
		             bool* table) {
  const size_t length = characters_wanted.length();
  const char* const data = characters_wanted.data();
  for (size_t i = 0; i < length; ++i) {
    table[static_cast<unsigned char>(data[i])] = true;
  }
}

} // namespace
// static
const StringPiece::size_type StringPiece::npos = StringPiece::size_type(-1);

bool operator==(const StringPiece& x, const StringPiece& y) {
  if (x.size() != y.size()) {
    return false;
  }
  return StringPiece::wordmemcmp(x.data(), y.data(), x.size()) == 0;
}

std::ostream& operator<<(std::ostream& o, const StringPiece& piece) {
  o.write(piece.data(), static_cast<std::streamsize>(piece.size()));
  return o;
}

void StringPiece::CopyToString(std::string* target) const {
  if (empty()) {
    target->clear();
  } else {
    target->assign(data(), size());
  }
}

void StringPiece::AppendToString(std::string* target) const {
  if (!empty()) {
    target->append(data(), size());
  }  
}

size_t StringPiece::copy(value_type* buf, size_type n, size_type pos) const {
  size_t ret = std::min(size() - pos, n);
  memcpy(buf, data() + pos, ret * sizeof(value_type));
  return ret;
}

StringPiece::size_type StringPiece::find(const StringPiece& s,
		                         size_type pos) const {
  if (pos > size()) {
    return StringPiece::npos;
  }  
  const_iterator result = std::search(begin() + pos, end(), s.begin(), s.end());
  const size_t xpos = static_cast<size_t>(result - begin());
  return xpos + s.size() <= size() ? xpos : npos;
}

StringPiece::size_type StringPiece::find(value_type c,
		                         size_t pos) const {
  if (pos >= size()) {
    return npos;
  }
  const_iterator result = std::find(begin() + pos, end(), c);
  return result != end() ?
	  static_cast<size_t>(result - begin()) : npos;
}

StringPiece::size_type StringPiece::rfind(const StringPiece& s, 
		                          size_type pos) const {
  if (size() < s.size()) {
    return npos;
  }

  if (s.empty()) {
    return std::min(size(), pos);
  }

  const_iterator last = begin() + std::min(size() - s.size(), pos) +
	                          s.size();
  const_iterator result = std::find_end(begin(), last, s.begin(), s.end());
  return result != last ?
	  (result - begin()) : npos;
}

StringPiece::size_type StringPiece::rfind(value_type c,
		                          size_type pos) const {
  if (size() == 0) {
    return npos;
  }

  for (size_type i = std::min(pos, size() -1) ; ; --i) {
    if (data()[i] == c) {
      return i;
    }  
    if (i == 0) {
      break;
    }
  }
  return npos;
}

StringPiece::size_type StringPiece::find_first_of(const StringPiece& s,
		                                  size_type pos) const {
  if (size() == 0 || s.size() == 0) {
    return npos;
  }

  if (s.size() == 1) {
    return find(s.data()[0], pos);
  }

  bool lookup[UCHAR_MAX + 1] = {false};
  BuildLookupTable(s, lookup);
  for (size_type i = pos; i < size(); ++i) {
    if (lookup[static_cast<unsigned char>(data()[i])]) {
      return i;
    }
  }
  return npos;
}

StringPiece::size_type StringPiece::find_first_of(value_type c,
		                                  size_type pos) const {
  return find(c, pos);
}

StringPiece::size_type StringPiece::find_first_not_of(const StringPiece& s,
		                                      size_type pos) const {
  if (size() == 0)
    return npos;

  if (s.size() == 0)
    return 0;

  if (s.size() == 1) {
    return find_first_not_of(s.data()[0], pos);
  }

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (size_type i = pos; i < size(); ++i) {
    if (!lookup[static_cast<unsigned char>(data()[i])]) {
      return i;
    }
  }
  return npos;
}

StringPiece::size_type StringPiece::find_first_not_of(value_type c,
		                                      size_type pos) const {
  if (size() == 0) {
    return npos;
  }

  for (; pos < size(); ++pos) {
    if (data()[pos] != c) {
      return pos;
    }
  }

  return npos;
}


StringPiece::size_type StringPiece::find_last_of(const StringPiece& s,
		                                 size_type pos) const {
  if (size() == 0 || s.size() == 0) {
    return npos;
  }

  if (s.size() == 1) {
    return rfind(s.data()[0], pos);
  }

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (size_type i = std::min(pos, size() -1); ; --i) {
    if (lookup[static_cast<unsigned char>(data()[i])]) {
      return i;
    }
    if (i == 0) {
      break;
    }
  }
  return npos;
}

StringPiece::size_type StringPiece::find_last_of(value_type c, 
		                                 size_type pos) const {
  return rfind(c, pos);
}

StringPiece::size_type StringPiece::find_last_not_of(const StringPiece& s,
		                                     size_type pos) const {
  if (size() == 0)
    return npos;

  size_t i = std::min(pos, size() - 1);
  if (s.size() == 0) {
    return i;
  }

  if (s.size() == 1) {
    return find_last_not_of(s.data()[0], pos);
  }

  bool lookup[UCHAR_MAX + 1] = { false };
  BuildLookupTable(s, lookup);
  for (; ; --i) {
    if (!lookup[static_cast<unsigned char>(data()[i])]) {
      return i;
    }
    if (i == 0) {
      break;
    }
  }
  return npos;
}

StringPiece::size_type StringPiece::find_last_not_of(value_type c,
		                                     size_type pos) const {
  if (size() == 0)
    return npos;

  for (size_type i = std::min(pos, size() - 1); ; --i) {
    if (data()[i] != c) {
      return i;
    }
    if (i == 0) {
      break;
    }
  }
  return npos;
}


StringPiece StringPiece::substr(size_type pos, size_type n) const {
  if (pos > size()) pos = size();
  if (n > size() - pos) n = size() - pos;
  return StringPiece(data() + pos, n);
}

} // namespace base
