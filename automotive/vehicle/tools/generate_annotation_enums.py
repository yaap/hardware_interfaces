#!/usr/bin/python3

# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""A script to generate Java files and CPP header files based on annotations in VehicleProperty.aidl

   Need ANDROID_BUILD_TOP environmental variable to be set. This script will update
   ChangeModeForVehicleProperty.h and AccessForVehicleProperty.h under generated_lib/cpp and
   ChangeModeForVehicleProperty.java, AccessForVehicleProperty.java, EnumForVehicleProperty.java
   UnitsForVehicleProperty.java under generated_lib/java.

   Usage:
   $ python generate_annotation_enums.py
"""
import argparse
import filecmp
import os
import re
import sys
import tempfile

PROP_AIDL_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl_property/android/hardware/' +
    'automotive/vehicle/VehicleProperty.aidl')
CHANGE_MODE_CPP_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/cpp/' +
    'ChangeModeForVehicleProperty.h')
ACCESS_CPP_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/cpp/' +
    'AccessForVehicleProperty.h')
CHANGE_MODE_JAVA_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/java/' +
    'ChangeModeForVehicleProperty.java')
ACCESS_JAVA_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/java/' +
    'AccessForVehicleProperty.java')
ENUM_JAVA_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/java/' +
                         'EnumForVehicleProperty.java')
UNITS_JAVA_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/java/' +
                       'UnitsForVehicleProperty.java')
VERSION_CPP_FILE_PATH = ('hardware/interfaces/automotive/vehicle/aidl/generated_lib/cpp/' +
    'VersionForVehicleProperty.h')
SCRIPT_PATH = 'hardware/interfaces/automotive/vehicle/tools/generate_annotation_enums.py'

TAB = '    '
RE_ENUM_START = re.compile('\s*enum VehicleProperty \{')
RE_ENUM_END = re.compile('\s*\}\;')
RE_COMMENT_BEGIN = re.compile('\s*\/\*\*?')
RE_COMMENT_END = re.compile('\s*\*\/')
RE_CHANGE_MODE = re.compile('\s*\* @change_mode (\S+)\s*')
RE_VERSION = re.compile('\s*\* @version (\S+)\s*')
RE_ACCESS = re.compile('\s*\* @access (\S+)\s*')
RE_DATA_ENUM = re.compile('\s*\* @data_enum (\S+)\s*')
RE_UNIT = re.compile('\s*\* @unit (\S+)\s+')
RE_VALUE = re.compile('\s*(\w+)\s*=(.*)')

LICENSE = """/*
 * Copyright (C) 2023 The Android Open Source Project
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

/**
 * DO NOT EDIT MANUALLY!!!
 *
 * Generated by tools/generate_annotation_enums.py.
 */

// clang-format off

"""

CHANGE_MODE_CPP_HEADER = """#pragma once

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyChangeMode.h>

#include <unordered_map>

namespace aidl {
namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

std::unordered_map<VehicleProperty, VehiclePropertyChangeMode> ChangeModeForVehicleProperty = {
"""

CPP_FOOTER = """
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
}  // aidl
"""

ACCESS_CPP_HEADER = """#pragma once

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyAccess.h>

#include <unordered_map>

namespace aidl {
namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

std::unordered_map<VehicleProperty, VehiclePropertyAccess> AccessForVehicleProperty = {
"""

VERSION_CPP_HEADER = """#pragma once

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>

#include <unordered_map>

namespace aidl {
namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

std::unordered_map<VehicleProperty, int32_t> VersionForVehicleProperty = {
"""

CHANGE_MODE_JAVA_HEADER = """package android.hardware.automotive.vehicle;

import java.util.Map;

public final class ChangeModeForVehicleProperty {

    public static final Map<Integer, Integer> values = Map.ofEntries(
"""

JAVA_FOOTER = """
    );

}
"""

ACCESS_JAVA_HEADER = """package android.hardware.automotive.vehicle;

import java.util.Map;

public final class AccessForVehicleProperty {

    public static final Map<Integer, Integer> values = Map.ofEntries(
"""

ENUM_JAVA_HEADER = """package android.hardware.automotive.vehicle;

import java.util.List;
import java.util.Map;

public final class EnumForVehicleProperty {

    public static final Map<Integer, List<Class<?>>> values = Map.ofEntries(
"""

UNITS_JAVA_HEADER = """package android.hardware.automotive.vehicle;

import java.util.Map;

public final class UnitsForVehicleProperty {

    public static final Map<Integer, Integer> values = Map.ofEntries(
"""


class PropertyConfig:
    """Represents one VHAL property definition in VehicleProperty.aidl."""

    def __init__(self):
        self.name = None
        self.description = None
        self.comment = None
        self.change_mode = None
        self.access_modes = []
        self.enum_types = []
        self.unit_type = None
        self.version = None

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return ('PropertyConfig{{' +
            'name: {}, description: {}, change_mode: {}, access_modes: {}, enum_types: {}' +
            ', unit_type: {}, version: {}, comment: {}}}').format(self.name, self.description,
                self.change_mode, self.access_modes, self.enum_types, self.unit_type,
                self.version, self.comment)


class FileParser:

    def __init__(self):
        self.configs = None

    def parseFile(self, input_file):
        """Parses the input VehicleProperty.aidl file into a list of property configs."""
        processing = False
        in_comment = False
        configs = []
        config = None
        with open(input_file, 'r') as f:
            for line in f.readlines():
                if RE_ENUM_START.match(line):
                    processing = True
                elif RE_ENUM_END.match(line):
                    processing = False
                if not processing:
                    continue
                if RE_COMMENT_BEGIN.match(line):
                    in_comment = True
                    config = PropertyConfig()
                    description = ''
                    continue

                if RE_COMMENT_END.match(line):
                    in_comment = False
                if in_comment:
                    match = RE_CHANGE_MODE.match(line)
                    if match:
                        config.change_mode = match.group(1).replace('VehiclePropertyChangeMode.', '')
                        continue
                    match = RE_ACCESS.match(line)
                    if match:
                        config.access_modes.append(match.group(1).replace('VehiclePropertyAccess.', ''))
                        continue
                    match = RE_UNIT.match(line)
                    if match:
                        config.unit_type = match.group(1)
                        continue
                    match = RE_DATA_ENUM.match(line)
                    if match:
                        config.enum_types.append(match.group(1))
                        continue
                    match = RE_VERSION.match(line)
                    if match:
                        if config.version != None:
                            raise Exception('Duplicate version annotation for property: ' + prop_name)
                        config.version = match.group(1)
                        continue

                    sline = line.strip()
                    if sline.startswith('*'):
                        # Remove the '*'.
                        sline = sline[1:].strip()

                    if not config.description:
                        # We reach an empty line of comment, the description part is ending.
                        if sline == '':
                            config.description = description
                        else:
                            if description != '':
                                description += ' '
                            description += sline
                    else:
                        if not config.comment:
                            if sline != '':
                                # This is the first line for comment.
                                config.comment = sline
                        else:
                            if sline != '':
                                # Concat this line with the previous line's comment with a space.
                                config.comment += ' ' + sline
                            else:
                                # Treat empty line comment as a new line.
                                config.comment += '\n'
                else:
                    match = RE_VALUE.match(line)
                    if match:
                        prop_name = match.group(1)
                        if prop_name == 'INVALID':
                            continue
                        if not config.change_mode:
                            raise Exception(
                                    'No change_mode annotation for property: ' + prop_name)
                        if not config.access_modes:
                            raise Exception(
                                    'No access_mode annotation for property: ' + prop_name)
                        if not config.version:
                            raise Exception(
                                    'no version annotation for property: ' + prop_name)
                        config.name = prop_name
                        configs.append(config)

        self.configs = configs

    def convert(self, output, header, footer, cpp, field):
        """Converts the property config file to C++/Java output file."""
        counter = 0
        content = LICENSE + header
        for config in self.configs:
            if field == 'change_mode':
                if cpp:
                    annotation = "VehiclePropertyChangeMode::" + config.change_mode
                else:
                    annotation = "VehiclePropertyChangeMode." + config.change_mode
            elif field == 'access_mode':
                if cpp:
                    annotation = "VehiclePropertyAccess::" + config.access_modes[0]
                else:
                    annotation = "VehiclePropertyAccess." + config.access_modes[0]
            elif field == 'enum_types':
                if len(config.enum_types) < 1:
                    continue;
                if not cpp:
                    annotation = "List.of(" + ', '.join([class_name + ".class" for class_name in config.enum_types]) + ")"
            elif field == 'unit_type':
                if not config.unit_type:
                    continue
                if not cpp:
                    annotation = config.unit_type

            elif field == 'version':
                if cpp:
                    annotation = config.version
            else:
                raise Exception('Unknown field: ' + field)
            if counter != 0:
                content += '\n'
            if cpp:
                content += (TAB + TAB + '{VehicleProperty::' + config.name + ', ' +
                            annotation + '},')
            else:
                content += (TAB + TAB + 'Map.entry(VehicleProperty.' + config.name + ', ' +
                            annotation + '),')
            counter += 1

        # Remove the additional ',' at the end for the Java file.
        if not cpp:
            content = content[:-1]

        content += footer

        with open(output, 'w') as f:
            f.write(content)

    def outputAsCsv(self, output):
        content = 'name,description,change mode,access mode,enum type,unit type,comment\n'
        for config in self.configs:
            enum_types = None
            if not config.enum_types:
                enum_types = '/'
            else:
                enum_types = '/'.join(config.enum_types)
            unit_type = config.unit_type
            if not unit_type:
                unit_type = '/'
            access_modes = ''
            comment = config.comment
            if not comment:
                comment = ''
            content += '"{}","{}","{}","{}","{}","{}", "{}"\n'.format(
                    config.name,
                    # Need to escape quote as double quote.
                    config.description.replace('"', '""'),
                    config.change_mode,
                    '/'.join(config.access_modes),
                    enum_types,
                    unit_type,
                    comment.replace('"', '""'))

        with open(output, 'w+') as f:
            f.write(content)


def createTempFile():
    f = tempfile.NamedTemporaryFile(delete=False);
    f.close();
    return f.name


class GeneratedFile:

    def __init__(self, type):
        self.type = type
        self.cpp_file_path = None
        self.java_file_path = None
        self.cpp_header = None
        self.java_header = None
        self.cpp_footer = None
        self.java_footer = None
        self.cpp_output_file = None
        self.java_output_file = None

    def setCppFilePath(self, cpp_file_path):
        self.cpp_file_path = cpp_file_path

    def setJavaFilePath(self, java_file_path):
        self.java_file_path = java_file_path

    def setCppHeader(self, cpp_header):
        self.cpp_header = cpp_header

    def setCppFooter(self, cpp_footer):
        self.cpp_footer = cpp_footer

    def setJavaHeader(self, java_header):
        self.java_header = java_header

    def setJavaFooter(self, java_footer):
        self.java_footer = java_footer

    def convert(self, file_parser, check_only, temp_files):
        if self.cpp_file_path:
            output_file = GeneratedFile._getOutputFile(self.cpp_file_path, check_only, temp_files)
            file_parser.convert(output_file, self.cpp_header, self.cpp_footer, True, self.type)
            self.cpp_output_file = output_file

        if self.java_file_path:
            output_file = GeneratedFile._getOutputFile(self.java_file_path, check_only, temp_files)
            file_parser.convert(output_file, self.java_header, self.java_footer, False, self.type)
            self.java_output_file = output_file

    def cmp(self):
        if self.cpp_file_path:
            if not filecmp.cmp(self.cpp_output_file, self.cpp_file_path):
                return False

        if self.java_file_path:
            if not filecmp.cmp(self.java_output_file, self.java_file_path):
                return False

        return True

    @staticmethod
    def _getOutputFile(file_path, check_only, temp_files):
        if not check_only:
            return file_path

        temp_file = createTempFile()
        temp_files.append(temp_file)
        return temp_file


def main():
    parser = argparse.ArgumentParser(
            description='Generate Java and C++ enums based on annotations in VehicleProperty.aidl')
    parser.add_argument('--android_build_top', required=False, help='Path to ANDROID_BUILD_TOP')
    parser.add_argument('--preupload_files', nargs='*', required=False, help='modified files')
    parser.add_argument('--check_only', required=False, action='store_true',
            help='only check whether the generated files need update')
    parser.add_argument('--output_csv', required=False,
            help='Path to the parsing result in CSV style, useful for doc generation')
    args = parser.parse_args();
    android_top = None
    output_folder = None
    if args.android_build_top:
        android_top = args.android_build_top
        vehiclePropertyUpdated = False
        for preuload_file in args.preupload_files:
            if preuload_file.endswith('VehicleProperty.aidl'):
                vehiclePropertyUpdated = True
                break
        if not vehiclePropertyUpdated:
            return
    else:
        android_top = os.environ['ANDROID_BUILD_TOP']
    if not android_top:
        print('ANDROID_BUILD_TOP is not in environmental variable, please run source and lunch ' +
            'at the android root')

    aidl_file = os.path.join(android_top, PROP_AIDL_FILE_PATH)
    f = FileParser();
    f.parseFile(aidl_file)

    if args.output_csv:
        f.outputAsCsv(args.output_csv)
        return

    generated_files = []

    change_mode = GeneratedFile('change_mode')
    change_mode.setCppFilePath(os.path.join(android_top, CHANGE_MODE_CPP_FILE_PATH))
    change_mode.setJavaFilePath(os.path.join(android_top, CHANGE_MODE_JAVA_FILE_PATH))
    change_mode.setCppHeader(CHANGE_MODE_CPP_HEADER)
    change_mode.setCppFooter(CPP_FOOTER)
    change_mode.setJavaHeader(CHANGE_MODE_JAVA_HEADER)
    change_mode.setJavaFooter(JAVA_FOOTER)
    generated_files.append(change_mode)

    access_mode = GeneratedFile('access_mode')
    access_mode.setCppFilePath(os.path.join(android_top, ACCESS_CPP_FILE_PATH))
    access_mode.setJavaFilePath(os.path.join(android_top, ACCESS_JAVA_FILE_PATH))
    access_mode.setCppHeader(ACCESS_CPP_HEADER)
    access_mode.setCppFooter(CPP_FOOTER)
    access_mode.setJavaHeader(ACCESS_JAVA_HEADER)
    access_mode.setJavaFooter(JAVA_FOOTER)
    generated_files.append(access_mode)

    enum_types = GeneratedFile('enum_types')
    enum_types.setJavaFilePath(os.path.join(android_top, ENUM_JAVA_FILE_PATH))
    enum_types.setJavaHeader(ENUM_JAVA_HEADER)
    enum_types.setJavaFooter(JAVA_FOOTER)
    generated_files.append(enum_types)

    unit_type = GeneratedFile('unit_type')
    unit_type.setJavaFilePath(os.path.join(android_top, UNITS_JAVA_FILE_PATH))
    unit_type.setJavaHeader(UNITS_JAVA_HEADER)
    unit_type.setJavaFooter(JAVA_FOOTER)
    generated_files.append(unit_type)

    version = GeneratedFile('version')
    version.setCppFilePath(os.path.join(android_top, VERSION_CPP_FILE_PATH))
    version.setCppHeader(VERSION_CPP_HEADER)
    version.setCppFooter(CPP_FOOTER)
    generated_files.append(version)

    temp_files = []

    try:
        for generated_file in generated_files:
            generated_file.convert(f, args.check_only, temp_files)

        if not args.check_only:
            return

        for generated_file in generated_files:
            if not generated_file.cmp():
                print('The generated enum files for VehicleProperty.aidl requires update, ')
                print('Run \npython ' + android_top + '/' + SCRIPT_PATH)
                sys.exit(1)
    except Exception as e:
        print('Error parsing VehicleProperty.aidl')
        print(e)
        sys.exit(1)
    finally:
        for file in temp_files:
            os.remove(file)


if __name__ == '__main__':
    main()