// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SPRITE_ASSETPACK_H_
#define FLATBUFFERS_GENERATED_SPRITE_ASSETPACK_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");

#include "common_generated.h"

namespace AssetPack {

struct AtlasEntry;
struct AtlasEntryBuilder;

struct AtlasLayout;

struct Sprite;
struct SpriteBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) AtlasLayout FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t xCount_;
  int32_t yCount_;

 public:
  AtlasLayout()
      : xCount_(0),
        yCount_(0) {
  }
  AtlasLayout(int32_t _xCount, int32_t _yCount)
      : xCount_(::flatbuffers::EndianScalar(_xCount)),
        yCount_(::flatbuffers::EndianScalar(_yCount)) {
  }
  int32_t xCount() const {
    return ::flatbuffers::EndianScalar(xCount_);
  }
  int32_t yCount() const {
    return ::flatbuffers::EndianScalar(yCount_);
  }
};
FLATBUFFERS_STRUCT_END(AtlasLayout, 8);

struct AtlasEntry FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AtlasEntryBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_UV_MIN = 6,
    VT_UV_MAX = 8
  };
  const ::flatbuffers::String *name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_NAME);
  }
  const AssetPack::vec2 *uv_min() const {
    return GetStruct<const AssetPack::vec2 *>(VT_UV_MIN);
  }
  const AssetPack::vec2 *uv_max() const {
    return GetStruct<const AssetPack::vec2 *>(VT_UV_MAX);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<AssetPack::vec2>(verifier, VT_UV_MIN, 4) &&
           VerifyField<AssetPack::vec2>(verifier, VT_UV_MAX, 4) &&
           verifier.EndTable();
  }
};

struct AtlasEntryBuilder {
  typedef AtlasEntry Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_name(::flatbuffers::Offset<::flatbuffers::String> name) {
    fbb_.AddOffset(AtlasEntry::VT_NAME, name);
  }
  void add_uv_min(const AssetPack::vec2 *uv_min) {
    fbb_.AddStruct(AtlasEntry::VT_UV_MIN, uv_min);
  }
  void add_uv_max(const AssetPack::vec2 *uv_max) {
    fbb_.AddStruct(AtlasEntry::VT_UV_MAX, uv_max);
  }
  explicit AtlasEntryBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AtlasEntry> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AtlasEntry>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AtlasEntry> CreateAtlasEntry(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> name = 0,
    const AssetPack::vec2 *uv_min = nullptr,
    const AssetPack::vec2 *uv_max = nullptr) {
  AtlasEntryBuilder builder_(_fbb);
  builder_.add_uv_max(uv_max);
  builder_.add_uv_min(uv_min);
  builder_.add_name(name);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<AtlasEntry> CreateAtlasEntryDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    const AssetPack::vec2 *uv_min = nullptr,
    const AssetPack::vec2 *uv_max = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return AssetPack::CreateAtlasEntry(
      _fbb,
      name__,
      uv_min,
      uv_max);
}

struct Sprite FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef SpriteBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_ID = 6,
    VT_RESOLUTION = 8,
    VT_IMAGEFILENAME = 10,
    VT_FILTERMODE = 12,
    VT_ATLAS = 14,
    VT_ATLASLAYOUT = 16
  };
  const ::flatbuffers::String *name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_NAME);
  }
  uint32_t ID() const {
    return GetField<uint32_t>(VT_ID, 0);
  }
  const AssetPack::vec2 *resolution() const {
    return GetStruct<const AssetPack::vec2 *>(VT_RESOLUTION);
  }
  const ::flatbuffers::String *imageFileName() const {
    return GetPointer<const ::flatbuffers::String *>(VT_IMAGEFILENAME);
  }
  AssetPack::FilterMode filterMode() const {
    return static_cast<AssetPack::FilterMode>(GetField<int32_t>(VT_FILTERMODE, 0));
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::AtlasEntry>> *atlas() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::AtlasEntry>> *>(VT_ATLAS);
  }
  const AssetPack::AtlasLayout *atlasLayout() const {
    return GetStruct<const AssetPack::AtlasLayout *>(VT_ATLASLAYOUT);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint32_t>(verifier, VT_ID, 4) &&
           VerifyField<AssetPack::vec2>(verifier, VT_RESOLUTION, 4) &&
           VerifyOffset(verifier, VT_IMAGEFILENAME) &&
           verifier.VerifyString(imageFileName()) &&
           VerifyField<int32_t>(verifier, VT_FILTERMODE, 4) &&
           VerifyOffset(verifier, VT_ATLAS) &&
           verifier.VerifyVector(atlas()) &&
           verifier.VerifyVectorOfTables(atlas()) &&
           VerifyField<AssetPack::AtlasLayout>(verifier, VT_ATLASLAYOUT, 4) &&
           verifier.EndTable();
  }
};

struct SpriteBuilder {
  typedef Sprite Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_name(::flatbuffers::Offset<::flatbuffers::String> name) {
    fbb_.AddOffset(Sprite::VT_NAME, name);
  }
  void add_ID(uint32_t ID) {
    fbb_.AddElement<uint32_t>(Sprite::VT_ID, ID, 0);
  }
  void add_resolution(const AssetPack::vec2 *resolution) {
    fbb_.AddStruct(Sprite::VT_RESOLUTION, resolution);
  }
  void add_imageFileName(::flatbuffers::Offset<::flatbuffers::String> imageFileName) {
    fbb_.AddOffset(Sprite::VT_IMAGEFILENAME, imageFileName);
  }
  void add_filterMode(AssetPack::FilterMode filterMode) {
    fbb_.AddElement<int32_t>(Sprite::VT_FILTERMODE, static_cast<int32_t>(filterMode), 0);
  }
  void add_atlas(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::AtlasEntry>>> atlas) {
    fbb_.AddOffset(Sprite::VT_ATLAS, atlas);
  }
  void add_atlasLayout(const AssetPack::AtlasLayout *atlasLayout) {
    fbb_.AddStruct(Sprite::VT_ATLASLAYOUT, atlasLayout);
  }
  explicit SpriteBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Sprite> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Sprite>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Sprite> CreateSprite(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> name = 0,
    uint32_t ID = 0,
    const AssetPack::vec2 *resolution = nullptr,
    ::flatbuffers::Offset<::flatbuffers::String> imageFileName = 0,
    AssetPack::FilterMode filterMode = AssetPack::FilterMode_Nearest,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::AtlasEntry>>> atlas = 0,
    const AssetPack::AtlasLayout *atlasLayout = nullptr) {
  SpriteBuilder builder_(_fbb);
  builder_.add_atlasLayout(atlasLayout);
  builder_.add_atlas(atlas);
  builder_.add_filterMode(filterMode);
  builder_.add_imageFileName(imageFileName);
  builder_.add_resolution(resolution);
  builder_.add_ID(ID);
  builder_.add_name(name);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<Sprite> CreateSpriteDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    uint32_t ID = 0,
    const AssetPack::vec2 *resolution = nullptr,
    const char *imageFileName = nullptr,
    AssetPack::FilterMode filterMode = AssetPack::FilterMode_Nearest,
    const std::vector<::flatbuffers::Offset<AssetPack::AtlasEntry>> *atlas = nullptr,
    const AssetPack::AtlasLayout *atlasLayout = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto imageFileName__ = imageFileName ? _fbb.CreateString(imageFileName) : 0;
  auto atlas__ = atlas ? _fbb.CreateVector<::flatbuffers::Offset<AssetPack::AtlasEntry>>(*atlas) : 0;
  return AssetPack::CreateSprite(
      _fbb,
      name__,
      ID,
      resolution,
      imageFileName__,
      filterMode,
      atlas__,
      atlasLayout);
}

inline const AssetPack::Sprite *GetSprite(const void *buf) {
  return ::flatbuffers::GetRoot<AssetPack::Sprite>(buf);
}

inline const AssetPack::Sprite *GetSizePrefixedSprite(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<AssetPack::Sprite>(buf);
}

inline bool VerifySpriteBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<AssetPack::Sprite>(nullptr);
}

inline bool VerifySizePrefixedSpriteBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<AssetPack::Sprite>(nullptr);
}

inline void FinishSpriteBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<AssetPack::Sprite> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedSpriteBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<AssetPack::Sprite> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace AssetPack

#endif  // FLATBUFFERS_GENERATED_SPRITE_ASSETPACK_H_
