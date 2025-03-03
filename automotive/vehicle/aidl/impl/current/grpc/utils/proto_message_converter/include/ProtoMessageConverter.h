/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef android_hardware_automotive_vehicle_aidl_impl_grpc_utils_proto_message_converter_include_ProtoMessageConverter_H_
#define android_hardware_automotive_vehicle_aidl_impl_grpc_utils_proto_message_converter_include_ProtoMessageConverter_H_

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>
#include <android/hardware/automotive/vehicle/GetMinMaxSupportedValuesTypes.pb.h>
#include <android/hardware/automotive/vehicle/GetSupportedValuesListsTypes.pb.h>
#include <android/hardware/automotive/vehicle/PropIdAreaId.pb.h>
#include <android/hardware/automotive/vehicle/RawPropValues.pb.h>
#include <android/hardware/automotive/vehicle/SubscribeOptions.pb.h>
#include <android/hardware/automotive/vehicle/VehicleAreaConfig.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropConfig.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropValue.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropertyAccess.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropertyChangeMode.pb.h>
#include <android/hardware/automotive/vehicle/VehiclePropertyStatus.pb.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace proto_msg_converter {

// Convert AIDL VehiclePropConfig to Protobuf VehiclePropConfig.
void aidlToProto(
        const ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig& inAidlConfig,
        ::android::hardware::automotive::vehicle::proto::VehiclePropConfig* outProtoConfig);
// Convert Protobuf VehiclePropConfig to AIDL VehiclePropConfig.
void protoToAidl(
        const ::android::hardware::automotive::vehicle::proto::VehiclePropConfig& inProtoConfig,
        ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig* outAidlConfig);
// Convert AIDL VehiclePropValue to Protobuf VehiclePropValue.
void aidlToProto(const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& inAidlVal,
                 ::android::hardware::automotive::vehicle::proto::VehiclePropValue* outProtoVal);
// Convert Protobuf VehiclePropValue to AIDL VehiclePropValue.
void protoToAidl(
        const ::android::hardware::automotive::vehicle::proto::VehiclePropValue& inProtoVal,
        ::aidl::android::hardware::automotive::vehicle::VehiclePropValue* outAidlVal);
// Convert AIDL SubscribeOptions to Protobuf SubscribeOptions.
void aidlToProto(const ::aidl::android::hardware::automotive::vehicle::SubscribeOptions& in,
                 ::android::hardware::automotive::vehicle::proto::SubscribeOptions* out);
// Convert Protobuf SubscribeOptions to AIDL SubscribeOptions.
void protoToAidl(const ::android::hardware::automotive::vehicle::proto::SubscribeOptions& in,
                 ::aidl::android::hardware::automotive::vehicle::SubscribeOptions* out);
// Convert VehicleUtils PropIdAreaId to Protobuf PropIdAreaId.
void aidlToProto(const PropIdAreaId& in,
                 ::android::hardware::automotive::vehicle::proto::PropIdAreaId* out);
// Convert Protobuf PropIdAreaId to VehicleUtils PropIdAreaId.
void protoToAidl(const ::android::hardware::automotive::vehicle::proto::PropIdAreaId& in,
                 PropIdAreaId* out);
// Convert AIDL RawPropValues to Protobuf RawPropValues.
void aidlToProto(const ::aidl::android::hardware::automotive::vehicle::RawPropValues& in,
                 ::android::hardware::automotive::vehicle::proto::RawPropValues* out);
// Convert Protobuf RawPropValues to AIDL RawPropValues.
void protoToAidl(const ::android::hardware::automotive::vehicle::proto::RawPropValues& in,
                 ::aidl::android::hardware::automotive::vehicle::RawPropValues* out);
// Convert AIDL MinMaxSupportedValueResult to Protobuf MinMaxSupportedValueResult.
void aidlToProto(
        const ::aidl::android::hardware::automotive::vehicle::MinMaxSupportedValueResult& in,
        ::android::hardware::automotive::vehicle::proto::MinMaxSupportedValueResult* out);
// Convert Protobuf MinMaxSupportedValueResult to AIDL MinMaxSupportedValueResult.
void protoToAidl(
        const ::android::hardware::automotive::vehicle::proto::MinMaxSupportedValueResult& in,
        ::aidl::android::hardware::automotive::vehicle::MinMaxSupportedValueResult* out);
// Convert AIDL SupportedValuesListResult to Protobuf SupportedValuesListResult.
void aidlToProto(
        const ::aidl::android::hardware::automotive::vehicle::SupportedValuesListResult& in,
        ::android::hardware::automotive::vehicle::proto::SupportedValuesListResult* out);
// Convert Protobuf SupportedValuesListResult to AIDL SupportedValuesListResult.
void protoToAidl(
        const ::android::hardware::automotive::vehicle::proto::SupportedValuesListResult& in,
        ::aidl::android::hardware::automotive::vehicle::SupportedValuesListResult* out);

}  // namespace proto_msg_converter
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_grpc_utils_proto_message_converter_include_ProtoMessageConverter_H_
