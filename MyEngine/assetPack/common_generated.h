// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_COMMON_ASSETPACK_H_
#define FLATBUFFERS_GENERATED_COMMON_ASSETPACK_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");

namespace AssetPack {

struct vec2;

struct vec4;

struct Transform;

struct SpriteRenderer;

struct ColorRenderer;

struct TextRenderer;
struct TextRendererBuilder;

struct Collider;

struct Rigidbody;

struct Staticbody;

struct ParticleSystemConfiguration;

struct ParticleSystemRenderer;

struct Entity;
struct EntityBuilder;

struct U_int;
struct U_intBuilder;

struct U_float;
struct U_floatBuilder;

struct U_vec2;
struct U_vec2Builder;

struct SerializableProperty;
struct SerializablePropertyBuilder;

struct Behaviour;
struct BehaviourBuilder;

enum FilterMode : int32_t {
  FilterMode_Nearest = 0,
  FilterMode_Linear = 1,
  FilterMode_MIN = FilterMode_Nearest,
  FilterMode_MAX = FilterMode_Linear
};

inline const FilterMode (&EnumValuesFilterMode())[2] {
  static const FilterMode values[] = {
    FilterMode_Nearest,
    FilterMode_Linear
  };
  return values;
}

inline const char * const *EnumNamesFilterMode() {
  static const char * const names[3] = {
    "Nearest",
    "Linear",
    nullptr
  };
  return names;
}

inline const char *EnumNameFilterMode(FilterMode e) {
  if (::flatbuffers::IsOutRange(e, FilterMode_Nearest, FilterMode_Linear)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesFilterMode()[index];
}

enum Shape : int32_t {
  Shape_Rectangle = 0,
  Shape_Circle = 1,
  Shape_MIN = Shape_Rectangle,
  Shape_MAX = Shape_Circle
};

inline const Shape (&EnumValuesShape())[2] {
  static const Shape values[] = {
    Shape_Rectangle,
    Shape_Circle
  };
  return values;
}

inline const char * const *EnumNamesShape() {
  static const char * const names[3] = {
    "Rectangle",
    "Circle",
    nullptr
  };
  return names;
}

inline const char *EnumNameShape(Shape e) {
  if (::flatbuffers::IsOutRange(e, Shape_Rectangle, Shape_Circle)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesShape()[index];
}

enum ParticleSystemSize : int32_t {
  ParticleSystemSize_Small = 0,
  ParticleSystemSize_Large = 1,
  ParticleSystemSize_MIN = ParticleSystemSize_Small,
  ParticleSystemSize_MAX = ParticleSystemSize_Large
};

inline const ParticleSystemSize (&EnumValuesParticleSystemSize())[2] {
  static const ParticleSystemSize values[] = {
    ParticleSystemSize_Small,
    ParticleSystemSize_Large
  };
  return values;
}

inline const char * const *EnumNamesParticleSystemSize() {
  static const char * const names[3] = {
    "Small",
    "Large",
    nullptr
  };
  return names;
}

inline const char *EnumNameParticleSystemSize(ParticleSystemSize e) {
  if (::flatbuffers::IsOutRange(e, ParticleSystemSize_Small, ParticleSystemSize_Large)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesParticleSystemSize()[index];
}

enum SerializableType : int32_t {
  SerializableType_INT = 0,
  SerializableType_FLOAT = 1,
  SerializableType_VEC2 = 2,
  SerializableType_MIN = SerializableType_INT,
  SerializableType_MAX = SerializableType_VEC2
};

inline const SerializableType (&EnumValuesSerializableType())[3] {
  static const SerializableType values[] = {
    SerializableType_INT,
    SerializableType_FLOAT,
    SerializableType_VEC2
  };
  return values;
}

inline const char * const *EnumNamesSerializableType() {
  static const char * const names[4] = {
    "INT",
    "FLOAT",
    "VEC2",
    nullptr
  };
  return names;
}

inline const char *EnumNameSerializableType(SerializableType e) {
  if (::flatbuffers::IsOutRange(e, SerializableType_INT, SerializableType_VEC2)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesSerializableType()[index];
}

enum SerializableValue : uint8_t {
  SerializableValue_NONE = 0,
  SerializableValue_U_int = 1,
  SerializableValue_U_float = 2,
  SerializableValue_U_vec2 = 3,
  SerializableValue_MIN = SerializableValue_NONE,
  SerializableValue_MAX = SerializableValue_U_vec2
};

inline const SerializableValue (&EnumValuesSerializableValue())[4] {
  static const SerializableValue values[] = {
    SerializableValue_NONE,
    SerializableValue_U_int,
    SerializableValue_U_float,
    SerializableValue_U_vec2
  };
  return values;
}

inline const char * const *EnumNamesSerializableValue() {
  static const char * const names[5] = {
    "NONE",
    "U_int",
    "U_float",
    "U_vec2",
    nullptr
  };
  return names;
}

inline const char *EnumNameSerializableValue(SerializableValue e) {
  if (::flatbuffers::IsOutRange(e, SerializableValue_NONE, SerializableValue_U_vec2)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesSerializableValue()[index];
}

template<typename T> struct SerializableValueTraits {
  static const SerializableValue enum_value = SerializableValue_NONE;
};

template<> struct SerializableValueTraits<AssetPack::U_int> {
  static const SerializableValue enum_value = SerializableValue_U_int;
};

template<> struct SerializableValueTraits<AssetPack::U_float> {
  static const SerializableValue enum_value = SerializableValue_U_float;
};

template<> struct SerializableValueTraits<AssetPack::U_vec2> {
  static const SerializableValue enum_value = SerializableValue_U_vec2;
};

bool VerifySerializableValue(::flatbuffers::Verifier &verifier, const void *obj, SerializableValue type);
bool VerifySerializableValueVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) vec2 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;

 public:
  vec2()
      : x_(0),
        y_(0) {
  }
  vec2(float _x, float _y)
      : x_(::flatbuffers::EndianScalar(_x)),
        y_(::flatbuffers::EndianScalar(_y)) {
  }
  float x() const {
    return ::flatbuffers::EndianScalar(x_);
  }
  float y() const {
    return ::flatbuffers::EndianScalar(y_);
  }
};
FLATBUFFERS_STRUCT_END(vec2, 8);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) vec4 FLATBUFFERS_FINAL_CLASS {
 private:
  float r_;
  float g_;
  float b_;
  float a_;

 public:
  vec4()
      : r_(0),
        g_(0),
        b_(0),
        a_(0) {
  }
  vec4(float _r, float _g, float _b, float _a)
      : r_(::flatbuffers::EndianScalar(_r)),
        g_(::flatbuffers::EndianScalar(_g)),
        b_(::flatbuffers::EndianScalar(_b)),
        a_(::flatbuffers::EndianScalar(_a)) {
  }
  float r() const {
    return ::flatbuffers::EndianScalar(r_);
  }
  float g() const {
    return ::flatbuffers::EndianScalar(g_);
  }
  float b() const {
    return ::flatbuffers::EndianScalar(b_);
  }
  float a() const {
    return ::flatbuffers::EndianScalar(a_);
  }
};
FLATBUFFERS_STRUCT_END(vec4, 16);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Transform FLATBUFFERS_FINAL_CLASS {
 private:
  AssetPack::vec2 position_;
  float rotation_;
  AssetPack::vec2 scale_;

 public:
  Transform()
      : position_(),
        rotation_(0),
        scale_() {
  }
  Transform(const AssetPack::vec2 &_position, float _rotation, const AssetPack::vec2 &_scale)
      : position_(_position),
        rotation_(::flatbuffers::EndianScalar(_rotation)),
        scale_(_scale) {
  }
  const AssetPack::vec2 &position() const {
    return position_;
  }
  float rotation() const {
    return ::flatbuffers::EndianScalar(rotation_);
  }
  const AssetPack::vec2 &scale() const {
    return scale_;
  }
};
FLATBUFFERS_STRUCT_END(Transform, 20);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) SpriteRenderer FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t atlasIndex_;
  uint32_t entityID_;
  uint32_t spriteID_;

 public:
  SpriteRenderer()
      : atlasIndex_(0),
        entityID_(0),
        spriteID_(0) {
  }
  SpriteRenderer(int32_t _atlasIndex, uint32_t _entityID, uint32_t _spriteID)
      : atlasIndex_(::flatbuffers::EndianScalar(_atlasIndex)),
        entityID_(::flatbuffers::EndianScalar(_entityID)),
        spriteID_(::flatbuffers::EndianScalar(_spriteID)) {
  }
  int32_t atlasIndex() const {
    return ::flatbuffers::EndianScalar(atlasIndex_);
  }
  uint32_t entityID() const {
    return ::flatbuffers::EndianScalar(entityID_);
  }
  uint32_t spriteID() const {
    return ::flatbuffers::EndianScalar(spriteID_);
  }
};
FLATBUFFERS_STRUCT_END(SpriteRenderer, 12);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) ColorRenderer FLATBUFFERS_FINAL_CLASS {
 private:
  AssetPack::vec4 color_;
  int32_t shape_;
  uint32_t entityID_;

 public:
  ColorRenderer()
      : color_(),
        shape_(0),
        entityID_(0) {
  }
  ColorRenderer(const AssetPack::vec4 &_color, AssetPack::Shape _shape, uint32_t _entityID)
      : color_(_color),
        shape_(::flatbuffers::EndianScalar(static_cast<int32_t>(_shape))),
        entityID_(::flatbuffers::EndianScalar(_entityID)) {
  }
  const AssetPack::vec4 &color() const {
    return color_;
  }
  AssetPack::Shape shape() const {
    return static_cast<AssetPack::Shape>(::flatbuffers::EndianScalar(shape_));
  }
  uint32_t entityID() const {
    return ::flatbuffers::EndianScalar(entityID_);
  }
};
FLATBUFFERS_STRUCT_END(ColorRenderer, 24);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Collider FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t type_;
  AssetPack::vec2 scale_;

 public:
  Collider()
      : type_(0),
        scale_() {
  }
  Collider(int32_t _type, const AssetPack::vec2 &_scale)
      : type_(::flatbuffers::EndianScalar(_type)),
        scale_(_scale) {
  }
  int32_t type() const {
    return ::flatbuffers::EndianScalar(type_);
  }
  const AssetPack::vec2 &scale() const {
    return scale_;
  }
};
FLATBUFFERS_STRUCT_END(Collider, 12);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Rigidbody FLATBUFFERS_FINAL_CLASS {
 private:
  uint32_t entityID_;
  AssetPack::Collider collider_;
  float linearDamping_;
  float angularDamping_;
  uint8_t fixedRotation_;
  uint8_t bullet_;
  int16_t padding0__;
  float gravityScale_;
  float friction_;
  float density_;
  float restitution_;

 public:
  Rigidbody()
      : entityID_(0),
        collider_(),
        linearDamping_(0),
        angularDamping_(0),
        fixedRotation_(0),
        bullet_(0),
        padding0__(0),
        gravityScale_(0),
        friction_(0),
        density_(0),
        restitution_(0) {
    (void)padding0__;
  }
  Rigidbody(uint32_t _entityID, const AssetPack::Collider &_collider, float _linearDamping, float _angularDamping, bool _fixedRotation, bool _bullet, float _gravityScale, float _friction, float _density, float _restitution)
      : entityID_(::flatbuffers::EndianScalar(_entityID)),
        collider_(_collider),
        linearDamping_(::flatbuffers::EndianScalar(_linearDamping)),
        angularDamping_(::flatbuffers::EndianScalar(_angularDamping)),
        fixedRotation_(::flatbuffers::EndianScalar(static_cast<uint8_t>(_fixedRotation))),
        bullet_(::flatbuffers::EndianScalar(static_cast<uint8_t>(_bullet))),
        padding0__(0),
        gravityScale_(::flatbuffers::EndianScalar(_gravityScale)),
        friction_(::flatbuffers::EndianScalar(_friction)),
        density_(::flatbuffers::EndianScalar(_density)),
        restitution_(::flatbuffers::EndianScalar(_restitution)) {
    (void)padding0__;
  }
  uint32_t entityID() const {
    return ::flatbuffers::EndianScalar(entityID_);
  }
  const AssetPack::Collider &collider() const {
    return collider_;
  }
  float linearDamping() const {
    return ::flatbuffers::EndianScalar(linearDamping_);
  }
  float angularDamping() const {
    return ::flatbuffers::EndianScalar(angularDamping_);
  }
  bool fixedRotation() const {
    return ::flatbuffers::EndianScalar(fixedRotation_) != 0;
  }
  bool bullet() const {
    return ::flatbuffers::EndianScalar(bullet_) != 0;
  }
  float gravityScale() const {
    return ::flatbuffers::EndianScalar(gravityScale_);
  }
  float friction() const {
    return ::flatbuffers::EndianScalar(friction_);
  }
  float density() const {
    return ::flatbuffers::EndianScalar(density_);
  }
  float restitution() const {
    return ::flatbuffers::EndianScalar(restitution_);
  }
};
FLATBUFFERS_STRUCT_END(Rigidbody, 44);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Staticbody FLATBUFFERS_FINAL_CLASS {
 private:
  uint32_t entityID_;
  AssetPack::Collider collider_;

 public:
  Staticbody()
      : entityID_(0),
        collider_() {
  }
  Staticbody(uint32_t _entityID, const AssetPack::Collider &_collider)
      : entityID_(::flatbuffers::EndianScalar(_entityID)),
        collider_(_collider) {
  }
  uint32_t entityID() const {
    return ::flatbuffers::EndianScalar(entityID_);
  }
  const AssetPack::Collider &collider() const {
    return collider_;
  }
};
FLATBUFFERS_STRUCT_END(Staticbody, 16);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) ParticleSystemConfiguration FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t particleCount_;
  uint8_t burstMode_;
  int8_t padding0__;  int16_t padding1__;
  float spawnRate_;
  float particleLifeSpan_;
  float gravity_;
  float startSize_;
  float endSize_;
  AssetPack::vec4 startColor_;
  AssetPack::vec4 endColor_;

 public:
  ParticleSystemConfiguration()
      : particleCount_(0),
        burstMode_(0),
        padding0__(0),
        padding1__(0),
        spawnRate_(0),
        particleLifeSpan_(0),
        gravity_(0),
        startSize_(0),
        endSize_(0),
        startColor_(),
        endColor_() {
    (void)padding0__;
    (void)padding1__;
  }
  ParticleSystemConfiguration(int32_t _particleCount, bool _burstMode, float _spawnRate, float _particleLifeSpan, float _gravity, float _startSize, float _endSize, const AssetPack::vec4 &_startColor, const AssetPack::vec4 &_endColor)
      : particleCount_(::flatbuffers::EndianScalar(_particleCount)),
        burstMode_(::flatbuffers::EndianScalar(static_cast<uint8_t>(_burstMode))),
        padding0__(0),
        padding1__(0),
        spawnRate_(::flatbuffers::EndianScalar(_spawnRate)),
        particleLifeSpan_(::flatbuffers::EndianScalar(_particleLifeSpan)),
        gravity_(::flatbuffers::EndianScalar(_gravity)),
        startSize_(::flatbuffers::EndianScalar(_startSize)),
        endSize_(::flatbuffers::EndianScalar(_endSize)),
        startColor_(_startColor),
        endColor_(_endColor) {
    (void)padding0__;
    (void)padding1__;
  }
  int32_t particleCount() const {
    return ::flatbuffers::EndianScalar(particleCount_);
  }
  bool burstMode() const {
    return ::flatbuffers::EndianScalar(burstMode_) != 0;
  }
  float spawnRate() const {
    return ::flatbuffers::EndianScalar(spawnRate_);
  }
  float particleLifeSpan() const {
    return ::flatbuffers::EndianScalar(particleLifeSpan_);
  }
  float gravity() const {
    return ::flatbuffers::EndianScalar(gravity_);
  }
  float startSize() const {
    return ::flatbuffers::EndianScalar(startSize_);
  }
  float endSize() const {
    return ::flatbuffers::EndianScalar(endSize_);
  }
  const AssetPack::vec4 &startColor() const {
    return startColor_;
  }
  const AssetPack::vec4 &endColor() const {
    return endColor_;
  }
};
FLATBUFFERS_STRUCT_END(ParticleSystemConfiguration, 60);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) ParticleSystemRenderer FLATBUFFERS_FINAL_CLASS {
 private:
  uint32_t entityID_;
  int32_t size_;
  AssetPack::ParticleSystemConfiguration configuration_;

 public:
  ParticleSystemRenderer()
      : entityID_(0),
        size_(0),
        configuration_() {
  }
  ParticleSystemRenderer(uint32_t _entityID, AssetPack::ParticleSystemSize _size, const AssetPack::ParticleSystemConfiguration &_configuration)
      : entityID_(::flatbuffers::EndianScalar(_entityID)),
        size_(::flatbuffers::EndianScalar(static_cast<int32_t>(_size))),
        configuration_(_configuration) {
  }
  uint32_t entityID() const {
    return ::flatbuffers::EndianScalar(entityID_);
  }
  AssetPack::ParticleSystemSize size() const {
    return static_cast<AssetPack::ParticleSystemSize>(::flatbuffers::EndianScalar(size_));
  }
  const AssetPack::ParticleSystemConfiguration &configuration() const {
    return configuration_;
  }
};
FLATBUFFERS_STRUCT_END(ParticleSystemRenderer, 68);

struct TextRenderer FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef TextRendererBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ENTITYID = 4,
    VT_TEXT = 6,
    VT_FONTID = 8,
    VT_COLOR = 10
  };
  uint32_t entityID() const {
    return GetField<uint32_t>(VT_ENTITYID, 0);
  }
  const ::flatbuffers::String *text() const {
    return GetPointer<const ::flatbuffers::String *>(VT_TEXT);
  }
  uint32_t fontID() const {
    return GetField<uint32_t>(VT_FONTID, 0);
  }
  const AssetPack::vec4 *color() const {
    return GetStruct<const AssetPack::vec4 *>(VT_COLOR);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ENTITYID, 4) &&
           VerifyOffset(verifier, VT_TEXT) &&
           verifier.VerifyString(text()) &&
           VerifyField<uint32_t>(verifier, VT_FONTID, 4) &&
           VerifyField<AssetPack::vec4>(verifier, VT_COLOR, 4) &&
           verifier.EndTable();
  }
};

struct TextRendererBuilder {
  typedef TextRenderer Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_entityID(uint32_t entityID) {
    fbb_.AddElement<uint32_t>(TextRenderer::VT_ENTITYID, entityID, 0);
  }
  void add_text(::flatbuffers::Offset<::flatbuffers::String> text) {
    fbb_.AddOffset(TextRenderer::VT_TEXT, text);
  }
  void add_fontID(uint32_t fontID) {
    fbb_.AddElement<uint32_t>(TextRenderer::VT_FONTID, fontID, 0);
  }
  void add_color(const AssetPack::vec4 *color) {
    fbb_.AddStruct(TextRenderer::VT_COLOR, color);
  }
  explicit TextRendererBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<TextRenderer> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<TextRenderer>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<TextRenderer> CreateTextRenderer(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t entityID = 0,
    ::flatbuffers::Offset<::flatbuffers::String> text = 0,
    uint32_t fontID = 0,
    const AssetPack::vec4 *color = nullptr) {
  TextRendererBuilder builder_(_fbb);
  builder_.add_color(color);
  builder_.add_fontID(fontID);
  builder_.add_text(text);
  builder_.add_entityID(entityID);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<TextRenderer> CreateTextRendererDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t entityID = 0,
    const char *text = nullptr,
    uint32_t fontID = 0,
    const AssetPack::vec4 *color = nullptr) {
  auto text__ = text ? _fbb.CreateString(text) : 0;
  return AssetPack::CreateTextRenderer(
      _fbb,
      entityID,
      text__,
      fontID,
      color);
}

struct Entity FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef EntityBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ID = 4,
    VT_NAME = 6,
    VT_TRANSFORM = 8,
    VT_PARENT = 10,
    VT_CHILDREN = 12
  };
  uint32_t id() const {
    return GetField<uint32_t>(VT_ID, 0);
  }
  const ::flatbuffers::String *name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_NAME);
  }
  const AssetPack::Transform *transform() const {
    return GetStruct<const AssetPack::Transform *>(VT_TRANSFORM);
  }
  uint32_t parent() const {
    return GetField<uint32_t>(VT_PARENT, 0);
  }
  const ::flatbuffers::Vector<uint32_t> *children() const {
    return GetPointer<const ::flatbuffers::Vector<uint32_t> *>(VT_CHILDREN);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ID, 4) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<AssetPack::Transform>(verifier, VT_TRANSFORM, 4) &&
           VerifyField<uint32_t>(verifier, VT_PARENT, 4) &&
           VerifyOffset(verifier, VT_CHILDREN) &&
           verifier.VerifyVector(children()) &&
           verifier.EndTable();
  }
};

struct EntityBuilder {
  typedef Entity Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_id(uint32_t id) {
    fbb_.AddElement<uint32_t>(Entity::VT_ID, id, 0);
  }
  void add_name(::flatbuffers::Offset<::flatbuffers::String> name) {
    fbb_.AddOffset(Entity::VT_NAME, name);
  }
  void add_transform(const AssetPack::Transform *transform) {
    fbb_.AddStruct(Entity::VT_TRANSFORM, transform);
  }
  void add_parent(uint32_t parent) {
    fbb_.AddElement<uint32_t>(Entity::VT_PARENT, parent, 0);
  }
  void add_children(::flatbuffers::Offset<::flatbuffers::Vector<uint32_t>> children) {
    fbb_.AddOffset(Entity::VT_CHILDREN, children);
  }
  explicit EntityBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Entity> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Entity>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Entity> CreateEntity(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    ::flatbuffers::Offset<::flatbuffers::String> name = 0,
    const AssetPack::Transform *transform = nullptr,
    uint32_t parent = 0,
    ::flatbuffers::Offset<::flatbuffers::Vector<uint32_t>> children = 0) {
  EntityBuilder builder_(_fbb);
  builder_.add_children(children);
  builder_.add_parent(parent);
  builder_.add_transform(transform);
  builder_.add_name(name);
  builder_.add_id(id);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<Entity> CreateEntityDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    const char *name = nullptr,
    const AssetPack::Transform *transform = nullptr,
    uint32_t parent = 0,
    const std::vector<uint32_t> *children = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto children__ = children ? _fbb.CreateVector<uint32_t>(*children) : 0;
  return AssetPack::CreateEntity(
      _fbb,
      id,
      name__,
      transform,
      parent,
      children__);
}

struct U_int FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef U_intBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VALUE = 4
  };
  int32_t value() const {
    return GetField<int32_t>(VT_VALUE, 0);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_VALUE, 4) &&
           verifier.EndTable();
  }
};

struct U_intBuilder {
  typedef U_int Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_value(int32_t value) {
    fbb_.AddElement<int32_t>(U_int::VT_VALUE, value, 0);
  }
  explicit U_intBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<U_int> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<U_int>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<U_int> CreateU_int(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int32_t value = 0) {
  U_intBuilder builder_(_fbb);
  builder_.add_value(value);
  return builder_.Finish();
}

struct U_float FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef U_floatBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VALUE = 4
  };
  float value() const {
    return GetField<float>(VT_VALUE, 0.0f);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<float>(verifier, VT_VALUE, 4) &&
           verifier.EndTable();
  }
};

struct U_floatBuilder {
  typedef U_float Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_value(float value) {
    fbb_.AddElement<float>(U_float::VT_VALUE, value, 0.0f);
  }
  explicit U_floatBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<U_float> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<U_float>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<U_float> CreateU_float(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    float value = 0.0f) {
  U_floatBuilder builder_(_fbb);
  builder_.add_value(value);
  return builder_.Finish();
}

struct U_vec2 FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef U_vec2Builder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VALUE = 4
  };
  const AssetPack::vec2 *value() const {
    return GetStruct<const AssetPack::vec2 *>(VT_VALUE);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<AssetPack::vec2>(verifier, VT_VALUE, 4) &&
           verifier.EndTable();
  }
};

struct U_vec2Builder {
  typedef U_vec2 Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_value(const AssetPack::vec2 *value) {
    fbb_.AddStruct(U_vec2::VT_VALUE, value);
  }
  explicit U_vec2Builder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<U_vec2> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<U_vec2>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<U_vec2> CreateU_vec2(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const AssetPack::vec2 *value = nullptr) {
  U_vec2Builder builder_(_fbb);
  builder_.add_value(value);
  return builder_.Finish();
}

struct SerializableProperty FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef SerializablePropertyBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TYPE = 4,
    VT_NAME = 6,
    VT_VALUE_TYPE = 8,
    VT_VALUE = 10
  };
  AssetPack::SerializableType type() const {
    return static_cast<AssetPack::SerializableType>(GetField<int32_t>(VT_TYPE, 0));
  }
  const ::flatbuffers::String *name() const {
    return GetPointer<const ::flatbuffers::String *>(VT_NAME);
  }
  AssetPack::SerializableValue value_type() const {
    return static_cast<AssetPack::SerializableValue>(GetField<uint8_t>(VT_VALUE_TYPE, 0));
  }
  const void *value() const {
    return GetPointer<const void *>(VT_VALUE);
  }
  template<typename T> const T *value_as() const;
  const AssetPack::U_int *value_as_U_int() const {
    return value_type() == AssetPack::SerializableValue_U_int ? static_cast<const AssetPack::U_int *>(value()) : nullptr;
  }
  const AssetPack::U_float *value_as_U_float() const {
    return value_type() == AssetPack::SerializableValue_U_float ? static_cast<const AssetPack::U_float *>(value()) : nullptr;
  }
  const AssetPack::U_vec2 *value_as_U_vec2() const {
    return value_type() == AssetPack::SerializableValue_U_vec2 ? static_cast<const AssetPack::U_vec2 *>(value()) : nullptr;
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_TYPE, 4) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint8_t>(verifier, VT_VALUE_TYPE, 1) &&
           VerifyOffset(verifier, VT_VALUE) &&
           VerifySerializableValue(verifier, value(), value_type()) &&
           verifier.EndTable();
  }
};

template<> inline const AssetPack::U_int *SerializableProperty::value_as<AssetPack::U_int>() const {
  return value_as_U_int();
}

template<> inline const AssetPack::U_float *SerializableProperty::value_as<AssetPack::U_float>() const {
  return value_as_U_float();
}

template<> inline const AssetPack::U_vec2 *SerializableProperty::value_as<AssetPack::U_vec2>() const {
  return value_as_U_vec2();
}

struct SerializablePropertyBuilder {
  typedef SerializableProperty Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_type(AssetPack::SerializableType type) {
    fbb_.AddElement<int32_t>(SerializableProperty::VT_TYPE, static_cast<int32_t>(type), 0);
  }
  void add_name(::flatbuffers::Offset<::flatbuffers::String> name) {
    fbb_.AddOffset(SerializableProperty::VT_NAME, name);
  }
  void add_value_type(AssetPack::SerializableValue value_type) {
    fbb_.AddElement<uint8_t>(SerializableProperty::VT_VALUE_TYPE, static_cast<uint8_t>(value_type), 0);
  }
  void add_value(::flatbuffers::Offset<void> value) {
    fbb_.AddOffset(SerializableProperty::VT_VALUE, value);
  }
  explicit SerializablePropertyBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<SerializableProperty> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<SerializableProperty>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<SerializableProperty> CreateSerializableProperty(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    AssetPack::SerializableType type = AssetPack::SerializableType_INT,
    ::flatbuffers::Offset<::flatbuffers::String> name = 0,
    AssetPack::SerializableValue value_type = AssetPack::SerializableValue_NONE,
    ::flatbuffers::Offset<void> value = 0) {
  SerializablePropertyBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_name(name);
  builder_.add_type(type);
  builder_.add_value_type(value_type);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<SerializableProperty> CreateSerializablePropertyDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    AssetPack::SerializableType type = AssetPack::SerializableType_INT,
    const char *name = nullptr,
    AssetPack::SerializableValue value_type = AssetPack::SerializableValue_NONE,
    ::flatbuffers::Offset<void> value = 0) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return AssetPack::CreateSerializableProperty(
      _fbb,
      type,
      name__,
      value_type,
      value);
}

struct Behaviour FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef BehaviourBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ENTITYID = 4,
    VT_HASH = 6,
    VT_PROPERTIES = 8
  };
  uint32_t entityID() const {
    return GetField<uint32_t>(VT_ENTITYID, 0);
  }
  uint32_t hash() const {
    return GetField<uint32_t>(VT_HASH, 0);
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::SerializableProperty>> *properties() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::SerializableProperty>> *>(VT_PROPERTIES);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ENTITYID, 4) &&
           VerifyField<uint32_t>(verifier, VT_HASH, 4) &&
           VerifyOffset(verifier, VT_PROPERTIES) &&
           verifier.VerifyVector(properties()) &&
           verifier.VerifyVectorOfTables(properties()) &&
           verifier.EndTable();
  }
};

struct BehaviourBuilder {
  typedef Behaviour Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_entityID(uint32_t entityID) {
    fbb_.AddElement<uint32_t>(Behaviour::VT_ENTITYID, entityID, 0);
  }
  void add_hash(uint32_t hash) {
    fbb_.AddElement<uint32_t>(Behaviour::VT_HASH, hash, 0);
  }
  void add_properties(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::SerializableProperty>>> properties) {
    fbb_.AddOffset(Behaviour::VT_PROPERTIES, properties);
  }
  explicit BehaviourBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Behaviour> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Behaviour>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Behaviour> CreateBehaviour(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t entityID = 0,
    uint32_t hash = 0,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<AssetPack::SerializableProperty>>> properties = 0) {
  BehaviourBuilder builder_(_fbb);
  builder_.add_properties(properties);
  builder_.add_hash(hash);
  builder_.add_entityID(entityID);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<Behaviour> CreateBehaviourDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t entityID = 0,
    uint32_t hash = 0,
    const std::vector<::flatbuffers::Offset<AssetPack::SerializableProperty>> *properties = nullptr) {
  auto properties__ = properties ? _fbb.CreateVector<::flatbuffers::Offset<AssetPack::SerializableProperty>>(*properties) : 0;
  return AssetPack::CreateBehaviour(
      _fbb,
      entityID,
      hash,
      properties__);
}

inline bool VerifySerializableValue(::flatbuffers::Verifier &verifier, const void *obj, SerializableValue type) {
  switch (type) {
    case SerializableValue_NONE: {
      return true;
    }
    case SerializableValue_U_int: {
      auto ptr = reinterpret_cast<const AssetPack::U_int *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case SerializableValue_U_float: {
      auto ptr = reinterpret_cast<const AssetPack::U_float *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case SerializableValue_U_vec2: {
      auto ptr = reinterpret_cast<const AssetPack::U_vec2 *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
  }
}

inline bool VerifySerializableValueVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<uint8_t> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (::flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifySerializableValue(
        verifier,  values->Get(i), types->GetEnum<SerializableValue>(i))) {
      return false;
    }
  }
  return true;
}

}  // namespace AssetPack

#endif  // FLATBUFFERS_GENERATED_COMMON_ASSETPACK_H_
