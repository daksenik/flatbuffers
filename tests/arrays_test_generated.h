// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_ARRAYSTEST_MYGAME_EXAMPLE_H_
#define FLATBUFFERS_GENERATED_ARRAYSTEST_MYGAME_EXAMPLE_H_

#include "flatbuffers/flatbuffers.h"

namespace MyGame {
namespace Example {

struct ArrayStruct;

struct ArrayTable;

MANUALLY_ALIGNED_STRUCT(4) ArrayStruct FLATBUFFERS_FINAL_CLASS {
 private:
  float a_;
  int32_t b_[15];
  int8_t c_;
  int8_t __padding0;
  int16_t __padding1;

 public:
  ArrayStruct() { memset(this, 0, sizeof(ArrayStruct)); }
  ArrayStruct(const ArrayStruct &_o) { memcpy(this, &_o, sizeof(ArrayStruct)); }
  ArrayStruct(float _a, const int32_t *_b, int8_t _c)
    : a_(flatbuffers::EndianScalar(_a)), c_(flatbuffers::EndianScalar(_c)), __padding0(0), __padding1(0) { memcpy(b_, _b, 60); (void)__padding0; (void)__padding1; }

  float a() const { return flatbuffers::EndianScalar(a_); }
  void mutate_a(float _a) { flatbuffers::WriteScalar(&a_, _a); }
  int32_t b(uint16_t idx) const { return flatbuffers::EndianScalar(b_[idx]); }
  int16_t b_length() const { return 15; }
  void mutate_b(uint16_t idx, int32_t _b) { flatbuffers::WriteScalar(&b_[idx], _b); }
  int8_t c() const { return flatbuffers::EndianScalar(c_); }
  void mutate_c(int8_t _c) { flatbuffers::WriteScalar(&c_, _c); }
};
STRUCT_END(ArrayStruct, 68);

struct ArrayTable FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_A = 4
  };
  const ArrayStruct *a() const { return GetStruct<const ArrayStruct *>(VT_A); }
  ArrayStruct *mutable_a() { return GetStruct<ArrayStruct *>(VT_A); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<ArrayStruct>(verifier, VT_A) &&
           verifier.EndTable();
  }
};

struct ArrayTableBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_a(const ArrayStruct *a) { fbb_.AddStruct(ArrayTable::VT_A, a); }
  ArrayTableBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  ArrayTableBuilder &operator=(const ArrayTableBuilder &);
  flatbuffers::Offset<ArrayTable> Finish() {
    auto o = flatbuffers::Offset<ArrayTable>(fbb_.EndTable(start_, 1));
    return o;
  }
};

inline flatbuffers::Offset<ArrayTable> CreateArrayTable(flatbuffers::FlatBufferBuilder &_fbb,
    const ArrayStruct *a = 0) {
  ArrayTableBuilder builder_(_fbb);
  builder_.add_a(a);
  return builder_.Finish();
}

inline const MyGame::Example::ArrayTable *GetArrayTable(const void *buf) { return flatbuffers::GetRoot<MyGame::Example::ArrayTable>(buf); }

inline ArrayTable *GetMutableArrayTable(void *buf) { return flatbuffers::GetMutableRoot<ArrayTable>(buf); }

inline bool VerifyArrayTableBuffer(flatbuffers::Verifier &verifier) { return verifier.VerifyBuffer<MyGame::Example::ArrayTable>(nullptr); }

inline void FinishArrayTableBuffer(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<MyGame::Example::ArrayTable> root) { fbb.Finish(root); }

}  // namespace Example
}  // namespace MyGame

#endif  // FLATBUFFERS_GENERATED_ARRAYSTEST_MYGAME_EXAMPLE_H_
