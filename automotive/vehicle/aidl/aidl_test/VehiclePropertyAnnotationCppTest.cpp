/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <AccessForVehicleProperty.h>
#include <AnnotationsForVehicleProperty.h>
#include <ChangeModeForVehicleProperty.h>
#include <EnumForVehicleProperty.h>
#include <VehicleHalTypes.h>

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unordered_set>

namespace aidl_vehicle = ::aidl::android::hardware::automotive::vehicle;
using aidl_vehicle::AllowedAccessForVehicleProperty;
using aidl_vehicle::AnnotationsForVehicleProperty;
using aidl_vehicle::ChangeModeForVehicleProperty;
using aidl_vehicle::DefaultAccessForVehicleProperty;
using aidl_vehicle::VehicleProperty;
using testing::IsEmpty;

namespace {
    template<class T>
    bool doesAnnotationMapContainsAllProps(std::unordered_map<VehicleProperty, T> annotationMap) {
        for (const VehicleProperty& v : ::ndk::enum_range<VehicleProperty>()) {
            std::string name = aidl_vehicle::toString(v);
            if (name == "INVALID") {
                continue;
            }
            if (annotationMap.find(v) == annotationMap.end()) {
                return false;
            }
        }
        return true;
    }

}  // namespace

TEST(VehiclePropertyAnnotationCppTest, testChangeMode) {
    ASSERT_TRUE(doesAnnotationMapContainsAllProps(ChangeModeForVehicleProperty))
            << "Outdated annotation-generated AIDL files. Please run "
            << "generate_annotation_enums.py to update.";
}

TEST(VehiclePropertyAnnotationCppTest, testDefaultAccess) {
    ASSERT_TRUE(doesAnnotationMapContainsAllProps(DefaultAccessForVehicleProperty))
            << "Outdated annotation-generated AIDL files. Please run "
            << "generate_annotation_enums.py to update.";
}

TEST(VehiclePropertyAnnotationCppTest, testAllowedAccess) {
    ASSERT_TRUE(doesAnnotationMapContainsAllProps(AllowedAccessForVehicleProperty))
            << "Outdated annotation-generated AIDL files. Please run "
            << "generate_annotation_enums.py to update.";
}

TEST(VehiclePropertyAnnotationCppTest, testAnnotations) {
    for (const auto& [propertyId, annotations] : AnnotationsForVehicleProperty) {
        ASSERT_THAT(annotations, Not(IsEmpty()))
                << "annotations set for property: " << aidl_vehicle::toString(propertyId)
                << " must not be empty";
    }
}

TEST(VehiclePropertyAnnotationCppTest, testSupportedEnums) {
    auto supportedEnums =
            aidl_vehicle::getSupportedEnumValuesForProperty(VehicleProperty::INFO_FUEL_TYPE);
    // We only listed part of the fuel type values here. This is enough to verify that the
    // generated code is correct.
    ASSERT_THAT(supportedEnums,
                ::testing::IsSupersetOf(
                        {static_cast<int64_t>(aidl_vehicle::FuelType::FUEL_TYPE_UNLEADED),
                         static_cast<int64_t>(aidl_vehicle::FuelType::FUEL_TYPE_LEADED),
                         static_cast<int64_t>(aidl_vehicle::FuelType::FUEL_TYPE_DIESEL_1),
                         static_cast<int64_t>(aidl_vehicle::FuelType::FUEL_TYPE_DIESEL_2)}));
}
