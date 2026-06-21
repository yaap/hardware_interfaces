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

package android.hardware.contexthub;

import android.hardware.common.NativeHandle;

/**
 * SharedDataRegions represent blocks of shared memory that serve as an allocation pool for one or
 * more data flows, each with their own metadata and payload blocks. The underlying data structure
 * for a data flow involves a fully specified subset of the memory in the shared data region, which
 * enables reuse of the region for more than one data flow, as long as the access permissions and
 * memory visibility are the same. Future HAL versions may also add support for new data structures
 * within shared data regions, supporting use cases that do not map well into the strictly ordered
 * queue semantics of data flows.
 */
@VintfStability
parcelable SharedDataRegion {
    /** The HAL-assigned id of the shared data region. */
    int id;

    /**
     * A mappable file descriptor for accessing the shared data region. Must be null if sending to
     * the HAL which can look up region information by id. The HAL must populate this field when
     * sending to clients.
     *
     * NOTE: Shared memory access should be done through the library
     * '/system/chre/data_flow:contexthub_data_flow'. The shared memory structures are defined
     * below.
     */
    @nullable ParcelFileDescriptor sharedMemory;

    /**
     * The size of the shared data region in bytes.
     *
     * NOTE: As of the first major version, this will be limited to 4GB, however to allow for
     * expansion, the type is set to long instead of int.
     */
    long sizeBytes;

    /**
     * The Android permissions required to access the shared data region. Must be null if sending to
     * the HAL which can look up region information by id. The HAL must populate this field when
     * sending to clients.
     */
    @nullable String[] permissions;

    /// The following @FixedSize structures describe the format of shared memory structures in a
    /// SharedDataRegion. Vendors should use the library
    /// '/system/chre/data_flow:contexthub_data_flow' to interact with shared memory, NOT
    /// build custom implementations based on these descriptions.
    ///
    /// NOTE: All references to objects in shared memory use offsets against the base of the region.
    /// This addresses access to the same region from both 32-bit and 64-bit cores as well as
    /// different mappings.
    ///
    /// Different endpoints, in particular on offload cores, will likely be using different versions
    /// of the library. As such, the first field of the metadata structs whose offsets are shared
    /// over AIDL ({@link DataFlowMetadata} and {@link DataFlowSinkMetadata}) is a {@link Version}
    /// field that allows the endpoint on the other side to determine which fields in all of the
    /// structures, and thus which features, are supported. {@link DataFlowSourceMetadata} and
    /// {@link DataFlowBlockHeader}, which are nested within other structures and so have a fixed
    /// maximum size include reserved fields to enable future expansion without changing the major
    /// version. If the reserved fields are exhausted, the major version is incremented, and new
    /// structures are defined. Any replacements for {@link DataFlowMetadata} and {@link
    /// DataFlowSinkMetadata} in later versions will keep the same Version field at the start.

    /** Analog to nullptr for offsets in shared memory. All offsets are 32-bit unsigned values. */
    const int OFFSET_INVALID = 0xFFFFFFFF;

    /**
     * The metadata for a data flow, found at {@link DataFlowId#metadataOffset} from the base of
     * the region. It contains fixed fields describing the data sent over the flow, points to the
     * source descriptor (contains the write index and tail data block offset), and an epoch
     * counter for the data storage block list. Read-only to sinks if memory protection is
     * supported in the sink context.
     */
    @FixedSize
    @VintfStability
    parcelable DataFlowMetadata {
        /**
         * The source's version of the data flow implementation. Fixed during data flow creation.
         */
        Version version;

        /** ATOMIC. The region offset of the SourceMetadata, set by the source. */
        int sourceMetadataOffsetBytes;

        /** The source's endpoint ID. Fixed during data flow creation. */
        EndpointIdFixedSize sourceId;

        /**
         * ATOMIC. A combination of the current data storage block count and the epoch of the block
         * list. This is used to determine if the block list has been updated, which can affect how
         * an overwritten sink catches up. The epoch is incremented on any change to the block
         * list. The block count can be used to determine the current capacity of the data flow.
         * { 0-15: epoch counter | 16-31: block count }
         */
        int blockListEpoch;

        /** The block capacity in bytes. Fixed during data flow creation. */
        int blockCapacityBytes;

        /** Configuration of elements in this flow. Fixed during data flow creation. */
        DataFlowElementConfig elementConfig;

        /**
         * 1 iff this data flow is local to an execution context, i.e. alerts can be
         * implemented via a local function call. This must be 0 for data flows accessed via the
         * ContextHub HAL.
         */
        byte localNotify;

        /** Reserved for future use. */
        byte[11] reserved;
    }

    /**
     * Configuration of elements in a data flow.
     *
     * NOTE: New configurations must be <= 8 bytes and at most 4-byte aligned to not change the
     * layout of this union.
     */
    @FixedSize
    @VintfStability
    union DataFlowElementConfig {
        FixedSize fixedSize;
        VariableSize variableSize;

        /** Configuration for a data flow of fixed-size elements. */
        @FixedSize
        @VintfStability
        parcelable FixedSize {
            int elementSizeBytes;
            char elementAlignmentBytes;
            byte[2] reserved;
        }

        /** Configuration for a data flow of variable-size elements. */
        @FixedSize
        @VintfStability
        parcelable VariableSize {
            char elementAlignmentBytes;
            byte[6] reserved;
        }
    }

    /**
     * The current source state, including the write index and a correction used to determine
     * the array index within the tail block. Points to the current tail block. Read-only to
     * sinks if memory protection is supported in the sink context.
     */
    @FixedSize
    @VintfStability
    parcelable DataFlowSourceMetadata {
        /**
         * ATOMIC. The current write index. Incremented by 1 for each element (or byte for
         * variable-size data) written. Set by the source.
         */
        int writeIndex;

        /**
         * The correction used to determine the array index within the tail block. The block index
         * is calculated as (writeIndex + indexCorrection) % blockCapacity. This enables the
         * source to skip to a newly allocated block if it would be blocked by a slow sink.
         */
        int indexCorrection;

        /**
         * The region offset of the block containing the current write index. Initialized to point
         * to the current tail block whenever the source initializes its descriptor while entering
         * the block.
         */
        int tailBlockOffsetBytes;

        /** Reserved for future use. */
        byte[12] reserved;
    }

    /**
     * The current sink state. Contains the read index, flags used to indicate
     * exceptional state, and the id used to route alerts to the sink. Read-write to both
     * the source and sink.
     */
    @FixedSize
    @VintfStability
    parcelable DataFlowSinkMetadata {
        /**
         * The version of the sink implementation. Must be the first field so that the sink
         * version can be checked before accessing other fields.
         */
        Version version;

        /**
         * ATOMIC. The current read index. The distance to the source can be calculated by
         * subtracting the read index from the write index. Set by the sink.
         *
         * NOTE: The source initializes this field to {@link DataFlowSourceMetadata#writeIndex}
         * before sharing the data flow with the sink. This allows the sink to read from the data
         * flow at the point that the source has initialized the sink metadata rather than
         * losing all of the data written after descriptor initialization in shared memory and
         * remote sink initialization.
         */
        int readIndex;

        /**
         * The correction used to determine the array index within the tail block. The block index
         * calculation is the same as for the source's indexCorrection.
         *
         * NOTE: The source initializes this field to {@link DataFlowSourceMetadata#indexCorrection}
         * before sharing the data flow with the sink. See the note on {@link #readIndex} for more
         * details.
         */
        int indexCorrection;

        /**
         * ATOMIC. This field is used together with {@link #sinkFlags} to emulate a single flag
         * that would be set by the source to indicate exceptional state and would be atomically
         * cleared by the sink using a read-modify-write operation. As endpoints interacting
         * over a data flow may be on different core clusters, the consistency of state after
         * read-modify-write operations is not guaranteed. As such, the single logical flag is split
         * into two fields. The sourceFlags contains the latest value set by the source along
         * with a counter that the source increments each time it sets the flag. Each time the
         * sink wants to "clear" the flag, it sets a counter in sinkFlags to the count
         * associated with the flag value to be cleared. Only the latest sourceFlags value is
         * relevant. { 0-15: {@link SourceFlags} | 16-31: counter }
         */
        int sourceFlags;

        /** The id used to route alerts to the sink. */
        EndpointIdFixedSize id;

        /** See {@link #sourceFlags}. {0-15: {@link SinkFlags} | 16-31: counter } */
        int sinkFlags;

        /**
         * The source initializes this field to {@link DataFlowSourceMetadata#tailBlockOffsetBytes}
         * before sharing the data flow with the sink. See the note on {@link #readIndex} for more
         * details.
         */
        int initialHeadBlockOffsetBytes;

        /**
         * The source initializes this field to {@link DataFlowMetadata#blockListEpoch} before
         * sharing the data flow with the sink. This allows the sink to handle overwrite
         * correctly when initializing from the descriptor. See the note on {@link #readIndex} for
         * more details.
         */
        int initialBlockListEpoch;

        /**
         * ATOMIC. Set by the source to indicate that it may overwrite this sink's position
         * if required.
         */
        boolean isOverwritable;

        /** Reserved for future use. */
        byte[11] reserved;

        /**
         * The flags that can be set by the source. Non-overlapping bits to allow for the
         * possibility of multiple flags being set at the same time.
         *
         * NOTE: This enum should be backed by a 16-bit type, however AIDL does not support this.
         * The backing type is given as int instead. This isn't an issue as this enum is not
         * directly used in @FixedSize parcelable definitions but instead corresponds to 16-bits of
         * an int field that is updated atomically.
         */
        @VintfStability
        @Backing(type="int")
        enum SourceFlags {
            NONE = 0,

            /**
             * Set when the source initializes the DataFlowSinkMetadata. Cleared by the sink on
             * initialization.
             */
            PENDING_INIT = 1,

            /** The source is blocked on this sink. */
            BLOCKING = 1 << 1,

            /** The source overwrote this sink. */
            OVERWRITE = 1 << 2,

            /** The source has torn down and the data flow is no longer valid. */
            FINISHED = 1 << 3,

            /**
             * The source detected that the sink endpoint disconnected. The sink must
             * request access to the data flow again.
             */
            DISCONNECTED = 1 << 4
        }

        /**
         * The values the sink can set in {@link #sinkFlags}. This is backed by an int for
         * the same reason as {@link SourceFlags}.
         */
        @VintfStability
        @Backing(type="int")
        enum SinkFlags {
            /** The sink has cleared a {@link SourceFlags} value at some flag count. */
            CLEARED = 0,

            /** The sink has stopped reading on the data flow. */
            FINISHED = 1
        }
    }

    /**
     * The header preceding each block of data storage. The current active source metadata
     * ({@link DataFlowMetadata#sourceMetadataOffsetBytes}) is in the header of one of the current
     * data storage blocks, though it must be accessed via the {@link DataFlowMetadata}. Read-only
     * to sinks if memory protection is supported in the sink context.
     */
    @FixedSize
    @VintfStability
    parcelable DataFlowBlockHeader {
        /** The current source metadata. */
        DataFlowSourceMetadata sourceMetadata;

        /**
         * ATOMIC. The region offset of the next block block list. Set by the source whenever
         * adding or removing blocks.
         */
        int nextBlockOffsetBytes;

        /**
         * ATOMIC. The base index for reading/writing this block. Initialized to 0. Set by the
         * source when it returns to a block that was skipped from on the previous visit. The base
         * index is set to the current skip index and the skip index is reset to the block capacity.
         */
        int baseIndex;

        /**
         * ATOMIC. The index at which to jump to the next block. Initialized to the block capacity.
         * Set by the source whenever overwriting a slow sink is avoided by skipping to a
         * newly allocated block. It is set to what would have been the next write index within the
         * block when the source skipped forward. The source's {@link
         * DataFlowSourceMetadata#indexCorrection} is updated so that the writeIndex/readIndex
         * difference is unaffected.
         */
        int skipIndex;

        /** Reserved for future use. */
        byte[12] reserved;
    }

    /** The header preceding a block of variable-size element storage. */
    @FixedSize
    @VintfStability
    parcelable DataFlowVariableSizeBlockHeader {
        /**
         * The base DataFlowBlockHeader. Must be the first element so that DataFlowBlockHeader* and
         * DataFlowVariableSizeBlockHeader* can be statically cast to each other.
         */
        DataFlowBlockHeader blockHeader;

        /**
         * The index within the data storage byte array of the first element header in this block.
         * Used to seek to an element when fast-forwarding through the block list. Set to the block
         * capacity if no element begins in the block.
         */
        int firstElementIndex;

        /** Reserved for future use. */
        byte[12] reserved;
    }

    /** The header preceding a variable-size element. */
    @FixedSize
    @VintfStability
    parcelable DataFlowVariableSizeElementHeader {
        /** The size of the element following this header in bytes. */
        int sizeBytes;
    }

    /**
     * The id used to route alerts an endpoint. This is the same as {@link EndpointId} but
     * @FixedSize for use in these structures.
     */
    @FixedSize
    @VintfStability
    parcelable EndpointIdFixedSize {
        /** The hub ID of the endpoint. */
        long hubId;

        /** The ID of the endpoint. */
        long endpointId;
    }

    /**
     * Represents a version of support for shared data flows.
     *
     * Represents a version of support for shared data flows. The major, minor, and patch versions
     * are determined by the version of shared memory support library used by the endpoint (see
     * /system/chre/data_flow:contexthub_data_flow). The support library will be
     * backwards-compatible with all previous versions of the library and use this value in shared
     * memory at runtime to determine the version of the other endpoints and enable/disable newer
     * features accordingly.
     */
    @FixedSize
    @VintfStability
    parcelable Version {
        /**
         * Major version, which denotes compatibility-breaking changes. Different struct
         * definitions will be added for major version updates.
         *
         * NOTE: This is interpreted as an unsigned byte, so the value will be between 1 and 255.
         */
        byte major;

        /**
         * Minor version, which denotes backwards-compatible feature additions.
         *
         * NOTE: This is interpreted as an unsigned byte, so the value will be between 0 and 255.
         */
        byte minor;

        /** Patch version, which denotes backwards-compatible minor changes. */
        char patch;
    }
}