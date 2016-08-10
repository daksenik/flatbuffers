/*
 * Copyright 2015 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLATBUFFERS_SCHEMALESS_H_
#define FLATBUFFERS_SCHEMALESS_H_

// We use the basic binary writing functions from the regular FlatBuffers.
#include "flatbuffers/flatbuffers.h"

namespace flatbuffers {

enum BitWidth {
  BIT_WIDTH_8 = 0,
  BIT_WIDTH_16,
  BIT_WIDTH_32,
  BIT_WIDTH_64
};

enum SchemaLessType {
  SL_INT = 0,
  SL_FLOAT,
  SL_STRING,
  SL_VECTOR,
};

class SchemaLessBuilder FLATBUFFERS_FINAL_CLASS {
 public:
  SchemaLessBuilder(uoffset_t initial_size = 1024,
                    const simple_allocator *allocator = nullptr)
      : minalign_(1),
        buf_(initial_size, allocator ? *allocator : default_allocator_),
        finished_(false)
      {}

  /// @brief The current size of the serialized buffer, counting from the end.
  size_t GetSize() const { return buf_.size(); }

  /// @brief Get the serialized buffer (after you call `Finish()`).
  /// @return Returns an `uint8_t` pointer to the data inside the buffer.
  uint8_t *GetBufferPointer() const {
    Finished();
    return buf_.data();
  }

  void Int(int64_t i) { stack_.push_back(Value(i, SL_INT, BIT_WIDTH_8)); }

  void Float(float f) { stack_.push_back(Value(f)); }
  void Double(double f) { stack_.push_back(Value(f)); }

  size_t String(const char *str) {
    auto len = strlen(str);
    auto bit_width = Width(len);
    auto byte_width = 1 << bit_width;
    PreAlign(len + 1, byte_width);
    buf_.push(reinterpret_cast<const uint8_t *>(str), len + 1);
    Write(len, byte_width);
    auto sloc = buf_.size();
    stack_.push_back(Value(sloc, SL_STRING, bit_width));
    return sloc;
  }

  size_t StartVector() { return stack_.size(); }

  size_t EndVector(size_t start) {
    // Figure out smallest bit width we can store this vector with.
    auto vec_len = stack_.size() - start;
    auto bit_width = Width(vec_len);
    for (size_t i = start; i < stack_.size(); i++) {
      auto elem_width = stack_[i].Width();
      bit_width = bit_width > elem_width ? bit_width : elem_width;
    }
    size_t byte_width = 1 << bit_width;
    // Write vector, in reverse. First the types.
    auto len = stack_.size() - start;
    PreAlign(len, byte_width);
    for (size_t i = stack_.size(); i > start; i--) {
      *buf_.make_space(1) = stack_[i - 1].Type();
    }
    // Then the actual data.
    for (size_t i = stack_.size(); i > start; i--) {
      auto &elem = stack_[i - 1];
      switch (elem.type_) {
        case SL_INT:
          Write(elem.i_, byte_width);
          break;
        case SL_FLOAT:
          WriteFloat(elem.f_, byte_width);
          break;
        case SL_STRING:
          WriteOffset(elem.i_, byte_width);
          break;
        default:
          assert(0);
      }
    }
    // Then the length.
    Write(vec_len, byte_width);
    stack_.resize(start);
    auto vloc = buf_.size();
    stack_.push_back(Value(vloc, SL_VECTOR, bit_width));
    return vloc;
  }

  void Finish() {
    // If you hit this assert, you likely have objects that were never included
    // in a parent. You need to have exactly one root to finish a buffer.
    // Check your Start/End calls are matched, and all objects are inside
    // some other object.
    assert(stack_.size() == 1);

    assert(stack_[0].type_ == SL_VECTOR);  // FIXME: other types too.
    PreAlign(2, minalign_);
    WriteOffset(stack_[0].i_, 1);
    Write(stack_[0].Type(), 1);
    finished_ = true;
  }

 private:
  void Finished() const {
    // If you get this assert, you're attempting to get access a buffer
    // which hasn't been finished yet. Be sure to call
    // SchemaLessBuilder::Finish with your root object.
    assert(finished_);
  }

  // Aligns such that when "len" bytes are written, an object can be written
  // after it with "alignment" without padding.
  void PreAlign(size_t len, size_t alignment) {
    buf_.fill(PaddingBytes(buf_.size() + len, alignment));
  }

  // For integer values T >= bit_width
  template<typename T> void Write(T val, int byte_width) {
    val = EndianScalar(val);
    buf_.push(reinterpret_cast<const uint8_t *>(&val), byte_width);
    minalign_ = std::max(minalign_, byte_width);
  }

  void WriteFloat(double f, size_t byte_width) {
    switch (byte_width) {
      case 8: Write(f, byte_width); break;
      case 4: Write(static_cast<float>(f), byte_width); break;
      // TODO: 16-bit floats.
      default: assert(0);
    }
  }

  void WriteOffset(size_t o, size_t byte_width) {
    auto reloff = buf_.size() - o + byte_width;
    assert(reloff > 0 && (reloff < 1ULL << (byte_width * 8) || byte_width == 8));
    Write(reloff, byte_width);
  }

  static uint8_t Type(BitWidth bit_width, SchemaLessType type) {
    return static_cast<uint8_t>(bit_width | (type << 2));
  }

  static BitWidth Width(int64_t i) {
    assert(i >= 0);  // FIXME
    if (i & 0xFFFFFFFF00000000) return BIT_WIDTH_64;
    if (i & 0xFFFFFFFFFFFF0000) return BIT_WIDTH_32;
    if (i & 0xFFFFFFFFFFFFFF00) return BIT_WIDTH_16;
    return BIT_WIDTH_8;
  }

  struct Value {
    union {
      int64_t i_;
      double f_;
    };

    SchemaLessType type_;
    BitWidth bit_width_;

    Value() : i_(-1), type_(SL_INT), bit_width_(BIT_WIDTH_8) {}

    Value(int64_t i, SchemaLessType t, BitWidth bw)
      : i_(i), type_(t), bit_width_(bw) {}

    Value(float f) : f_(f), type_(SL_FLOAT), bit_width_(BIT_WIDTH_32) {}
    Value(double f) : f_(f), type_(SL_FLOAT), bit_width_(BIT_WIDTH_64) {}

    uint8_t Type() { return SchemaLessBuilder::Type(bit_width_, type_); }

    BitWidth Width() {
      switch (type_) {
        case SL_INT: {
          if (i_ < 0) {
            assert(false);
            return BIT_WIDTH_64;
          } else {
            return SchemaLessBuilder::Width(i_);
          }
          break;
        }
        case SL_FLOAT:
          return bit_width_;
        case SL_STRING:
          return SchemaLessBuilder::Width(i_);  // FIXME: incorrect offset!
        default:
          assert(false);
          return BIT_WIDTH_64;
      }
    }
  };

  // You shouldn't really be copying instances of this class.
  SchemaLessBuilder(const FlatBufferBuilder &);
  SchemaLessBuilder &operator=(const SchemaLessBuilder &);

  simple_allocator default_allocator_;

  int minalign_;
  vector_downward buf_;  // FIXME: this has 2GB limit, which we don't need here.
  std::vector<Value> stack_;

  bool finished_;
};

// FIXME: make these private.
inline BitWidth GetBitWidth(uint8_t type) {
  return static_cast<BitWidth>(type & 3);
}

inline SchemaLessType GetSchemaLessType(uint8_t type) {
  return static_cast<SchemaLessType>(type >> 2);
}

inline int64_t ReadInt(const uint8_t *data, size_t byte_width) {
  int64_t i = 0;
  memcpy(&i, data, byte_width);  // FIXME: can we do this quicker?
  // FIXME: negative values sign extend.
  return i;
}

inline double ReadFloat(const uint8_t *data, size_t byte_width) {
  switch (byte_width) {
    case 8: return *reinterpret_cast<const double *>(data);
    case 4: return *reinterpret_cast<const float *>(data);
    default: assert(0); return 0;
  }
}

class SLObject {
 public:
  SLObject(const uint8_t *data, size_t byte_width)
    : data_(data + byte_width), byte_width_(byte_width) {}

  size_t size() { return ReadInt(data_ - byte_width_, byte_width_); }

 protected:
  const uint8_t *data_;
  size_t byte_width_;
};

class SLString : public SLObject {
 public:
  SLString(const uint8_t *data, size_t byte_width)
    : SLObject(data, byte_width) {}

  size_t length() { return size(); }
  const char *c_str() { return reinterpret_cast<const char *>(data_); }

  static SLString EmptyString() {
    static const uint8_t empty_string[] = { 0, 0 };
    return SLString(empty_string, BIT_WIDTH_8);
  }

  bool IsTheEmptyString() {
    return data_ == EmptyString().data_;
  }
};

class SLVector : public SLObject {
 public:
  SLVector(const uint8_t *data, size_t byte_width)
    : SLObject(data, byte_width) {}

  int64_t GetAsInt(size_t idx) {
    auto type = GetType(idx);
    if (GetSchemaLessType(type) == SL_INT) {
      return ReadInt(data_ + idx * byte_width_, byte_width_);
    } else {
      // Convert floats, strings and other things to int.
      assert(0);
      return 0;
    }
  }

  double GetAsFloat(size_t idx) {
    auto type = GetType(idx);
    if (GetSchemaLessType(type) == SL_FLOAT) {
      return ReadFloat(data_ + idx * byte_width_, byte_width_);
    } else {
      // Convert ints, strings and other things to float.
      assert(0);
      return 0;
    }
  }

  // TODO: also have a version of this function that returns std::string, so
  // we can convert other types.
  // This function returns the empty string if you try to read a not-string.
  SLString GetAsString(size_t idx) {
    auto type = GetType(idx);
    if (GetSchemaLessType(type) == SL_STRING) {
      auto str_elem = data_ + idx * byte_width_;
      auto off = ReadInt(str_elem, byte_width_);
      return SLString(str_elem + off, 1 << GetBitWidth(type));
    } else {
      return SLString::EmptyString();
    }
  }

 private:
  uint8_t GetType(size_t idx) {
    auto len = size();
    assert(idx < len);
    return (data_ + len * byte_width_)[idx];
  }
};

inline SLVector GetSchemaLessRootAsVector(const uint8_t *buffer) {
  if (GetSchemaLessType(buffer[0]) == SL_VECTOR) {
    auto bit_width = GetBitWidth(buffer[0]);
    auto byte_width = 1 << bit_width;
    return SLVector(buffer + 1 + buffer[1], byte_width);
  } else {
    // Can interpret maps etc as vector.
    // For now:
    assert(0);
    return SLVector(nullptr, 1);
  }
}

}  // namespace flatbuffers

#endif  // FLATBUFFERS_SCHEMALESS_H_
