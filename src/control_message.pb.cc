// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: control_message.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include <mimc/control_message.pb.h>

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
// @@protoc_insertion_point(includes)

namespace protocol {

void protobuf_ShutdownFile_control_5fmessage_2eproto() {
  delete PushServiceConfigMsg::default_instance_;
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
void protobuf_AddDesc_control_5fmessage_2eproto_impl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#else
void protobuf_AddDesc_control_5fmessage_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#endif
  PushServiceConfigMsg::default_instance_ = new PushServiceConfigMsg();
  PushServiceConfigMsg::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_control_5fmessage_2eproto);
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AddDesc_control_5fmessage_2eproto_once_);
void protobuf_AddDesc_control_5fmessage_2eproto() {
  ::google::protobuf::::google::protobuf::GoogleOnceInit(&protobuf_AddDesc_control_5fmessage_2eproto_once_,
                 &protobuf_AddDesc_control_5fmessage_2eproto_impl);
}
#else
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_control_5fmessage_2eproto {
  StaticDescriptorInitializer_control_5fmessage_2eproto() {
    protobuf_AddDesc_control_5fmessage_2eproto();
  }
} static_descriptor_initializer_control_5fmessage_2eproto_;
#endif

// ===================================================================

#ifndef _MSC_VER
const int PushServiceConfigMsg::kFetchBucketFieldNumber;
const int PushServiceConfigMsg::kUseBucketV2FieldNumber;
const int PushServiceConfigMsg::kClientVersionFieldNumber;
const int PushServiceConfigMsg::kCloudVersionFieldNumber;
const int PushServiceConfigMsg::kDotsFieldNumber;
#endif  // !_MSC_VER

PushServiceConfigMsg::PushServiceConfigMsg()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
}

void PushServiceConfigMsg::InitAsDefaultInstance() {
}

PushServiceConfigMsg::PushServiceConfigMsg(const PushServiceConfigMsg& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
}

void PushServiceConfigMsg::SharedCtor() {
  _cached_size_ = 0;
  fetchbucket_ = false;
  usebucketv2_ = false;
  clientversion_ = 0;
  cloudversion_ = 0;
  dots_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

PushServiceConfigMsg::~PushServiceConfigMsg() {
  SharedDtor();
}

void PushServiceConfigMsg::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void PushServiceConfigMsg::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const PushServiceConfigMsg& PushServiceConfigMsg::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_control_5fmessage_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_control_5fmessage_2eproto();
#endif
  return *default_instance_;
}

PushServiceConfigMsg* PushServiceConfigMsg::default_instance_ = NULL;

PushServiceConfigMsg* PushServiceConfigMsg::New() const {
  return new PushServiceConfigMsg;
}

void PushServiceConfigMsg::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    fetchbucket_ = false;
    usebucketv2_ = false;
    clientversion_ = 0;
    cloudversion_ = 0;
    dots_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

bool PushServiceConfigMsg::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional bool fetchBucket = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &fetchbucket_)));
          set_has_fetchbucket();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_useBucketV2;
        break;
      }

      // optional bool useBucketV2 = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_useBucketV2:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &usebucketv2_)));
          set_has_usebucketv2();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_clientVersion;
        break;
      }

      // optional int32 clientVersion = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_clientVersion:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &clientversion_)));
          set_has_clientversion();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(32)) goto parse_cloudVersion;
        break;
      }

      // optional int32 cloudVersion = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_cloudVersion:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &cloudversion_)));
          set_has_cloudversion();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(40)) goto parse_dots;
        break;
      }

      // optional int32 dots = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_dots:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &dots_)));
          set_has_dots();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }

      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void PushServiceConfigMsg::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional bool fetchBucket = 1;
  if (has_fetchbucket()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(1, this->fetchbucket(), output);
  }

  // optional bool useBucketV2 = 2;
  if (has_usebucketv2()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(2, this->usebucketv2(), output);
  }

  // optional int32 clientVersion = 3;
  if (has_clientversion()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->clientversion(), output);
  }

  // optional int32 cloudVersion = 4;
  if (has_cloudversion()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(4, this->cloudversion(), output);
  }

  // optional int32 dots = 5;
  if (has_dots()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(5, this->dots(), output);
  }

}

int PushServiceConfigMsg::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional bool fetchBucket = 1;
    if (has_fetchbucket()) {
      total_size += 1 + 1;
    }

    // optional bool useBucketV2 = 2;
    if (has_usebucketv2()) {
      total_size += 1 + 1;
    }

    // optional int32 clientVersion = 3;
    if (has_clientversion()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->clientversion());
    }

    // optional int32 cloudVersion = 4;
    if (has_cloudversion()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->cloudversion());
    }

    // optional int32 dots = 5;
    if (has_dots()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->dots());
    }

  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void PushServiceConfigMsg::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const PushServiceConfigMsg*>(&from));
}

void PushServiceConfigMsg::MergeFrom(const PushServiceConfigMsg& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_fetchbucket()) {
      set_fetchbucket(from.fetchbucket());
    }
    if (from.has_usebucketv2()) {
      set_usebucketv2(from.usebucketv2());
    }
    if (from.has_clientversion()) {
      set_clientversion(from.clientversion());
    }
    if (from.has_cloudversion()) {
      set_cloudversion(from.cloudversion());
    }
    if (from.has_dots()) {
      set_dots(from.dots());
    }
  }
}

void PushServiceConfigMsg::CopyFrom(const PushServiceConfigMsg& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool PushServiceConfigMsg::IsInitialized() const {

  return true;
}

void PushServiceConfigMsg::Swap(PushServiceConfigMsg* other) {
  if (other != this) {
    std::swap(fetchbucket_, other->fetchbucket_);
    std::swap(usebucketv2_, other->usebucketv2_);
    std::swap(clientversion_, other->clientversion_);
    std::swap(cloudversion_, other->cloudversion_);
    std::swap(dots_, other->dots_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string PushServiceConfigMsg::GetTypeName() const {
  return "protocol.PushServiceConfigMsg";
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace protocol

// @@protoc_insertion_point(global_scope)
