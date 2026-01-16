/*
 * Copyright (C) 2025 The Android Open Source Project
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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.contexthub;
@VintfStability
parcelable SharedDataRegion {
  int id;
  @nullable ParcelFileDescriptor sharedMemory;
  long sizeBytes;
  @nullable String[] permissions;
  const int OFFSET_INVALID = 0xFFFFFFFF;
  @FixedSize @VintfStability
  parcelable DataFlowMetadata {
    android.hardware.contexthub.SharedDataRegion.Version version;
    int sourceMetadataOffsetBytes;
    android.hardware.contexthub.SharedDataRegion.EndpointIdFixedSize sourceId;
    int blockListEpoch;
    int blockCapacityBytes;
    android.hardware.contexthub.SharedDataRegion.DataFlowElementConfig elementConfig;
    byte localNotify;
    byte[11] reserved;
  }
  @FixedSize @VintfStability
  union DataFlowElementConfig {
    android.hardware.contexthub.SharedDataRegion.DataFlowElementConfig.FixedSize fixedSize;
    android.hardware.contexthub.SharedDataRegion.DataFlowElementConfig.VariableSize variableSize;
    @FixedSize @VintfStability
    parcelable FixedSize {
      int elementSizeBytes;
      char elementAlignmentBytes;
      byte[2] reserved;
    }
    @FixedSize @VintfStability
    parcelable VariableSize {
      char elementAlignmentBytes;
      byte[6] reserved;
    }
  }
  @FixedSize @VintfStability
  parcelable DataFlowSourceMetadata {
    int writeIndex;
    int indexCorrection;
    int tailBlockOffsetBytes;
    byte[12] reserved;
  }
  @FixedSize @VintfStability
  parcelable DataFlowSinkMetadata {
    android.hardware.contexthub.SharedDataRegion.Version version;
    int readIndex;
    int indexCorrection;
    int sourceFlags;
    android.hardware.contexthub.SharedDataRegion.EndpointIdFixedSize id;
    int sinkFlags;
    int initialHeadBlockOffsetBytes;
    int initialBlockListEpoch;
    boolean isOverwritable;
    byte[11] reserved;
    @Backing(type="int") @VintfStability
    enum SourceFlags {
      NONE = 0,
      PENDING_INIT = 1,
      BLOCKING = (1 << 1) /* 2 */,
      OVERWRITE = (1 << 2) /* 4 */,
      FINISHED = (1 << 3) /* 8 */,
      DISCONNECTED = (1 << 4) /* 16 */,
    }
    @Backing(type="int") @VintfStability
    enum SinkFlags {
      CLEARED = 0,
      FINISHED = 1,
    }
  }
  @FixedSize @VintfStability
  parcelable DataFlowBlockHeader {
    android.hardware.contexthub.SharedDataRegion.DataFlowSourceMetadata sourceMetadata;
    int nextBlockOffsetBytes;
    int baseIndex;
    int skipIndex;
    byte[12] reserved;
  }
  @FixedSize @VintfStability
  parcelable DataFlowVariableSizeBlockHeader {
    android.hardware.contexthub.SharedDataRegion.DataFlowBlockHeader blockHeader;
    int firstElementIndex;
    byte[12] reserved;
  }
  @FixedSize @VintfStability
  parcelable DataFlowVariableSizeElementHeader {
    int sizeBytes;
  }
  @FixedSize @VintfStability
  parcelable EndpointIdFixedSize {
    long hubId;
    long endpointId;
  }
  @FixedSize @VintfStability
  parcelable Version {
    byte major;
    byte minor;
    char patch;
  }
}
