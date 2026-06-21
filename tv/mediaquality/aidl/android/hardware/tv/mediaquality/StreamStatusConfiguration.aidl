/*
 * Copyright 2025 The Android Open Source Project
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

package android.hardware.tv.mediaquality;
import android.hardware.tv.mediaquality.PictureParameters;

/**
 * A collection of PictureParameters associated with specific content stream statuses.
 *
 * <p>This structure allows the framework to provide specific picture settings (brightness,
 * contrast, etc.) for different content types (e.g., HDR10, Dolby Vision) within a single
 * Picture Profile.
 *
 * <p>All fields are {@code @nullable}. If a field is null, the HAL should fall back to the
 * {@code parameters} defined in the parent {@link PictureProfile}.
 */
@VintfStability
parcelable StreamStatusConfiguration {
    /** Picture parameters for Standard Dynamic Range (SDR) content. */
    @nullable PictureParameters sdrPictureParameters;

    /** Picture parameters for Dolby Vision content. */
    @nullable PictureParameters dolbyVisionParameters;

    /** Picture parameters for HDR10 content. */
    @nullable PictureParameters hdr10PictureParameters;

    /** Picture parameters for Technicolor (TCH) content. */
    @nullable PictureParameters tchPictureParameters;

    /** Picture parameters for Hybrid Log-Gamma (HLG) content. */
    @nullable PictureParameters hlgPictureParameters;

    /** Picture parameters for HDR10+ content. */
    @nullable PictureParameters hdr10PlusPictureParameters;

    /** Picture parameters for HDR Vivid content. */
    @nullable PictureParameters hdrVividPictureParameters;

    /** Picture parameters for IMAX Enhanced SDR content. */
    @nullable PictureParameters imaxSdr;

    /** Picture parameters for IMAX Enhanced HDR10 content. */
    @nullable PictureParameters imaxHdr10;

    /** Picture parameters for IMAX Enhanced HDR10+ content. */
    @nullable PictureParameters imaxHdr10Plus;

    /** Picture parameters for Filmmaker Mode in SDR. */
    @nullable PictureParameters fmmSdr;

    /** Picture parameters for Filmmaker Mode in HDR10. */
    @nullable PictureParameters fmmHdr10;

    /** Picture parameters for Filmmaker Mode in HDR10+. */
    @nullable PictureParameters fmmHdr10Plus;

    /** Picture parameters for Filmmaker Mode in HLG. */
    @nullable PictureParameters fmmHlg;

    /** Picture parameters for Filmmaker Mode in Dolby Vision. */
    @nullable PictureParameters fmmDolby;

    /** Picture parameters for Filmmaker Mode in Technicolor. */
    @nullable PictureParameters fmmTch;

    /** Picture parameters for Filmmaker Mode in HDR Vivid. */
    @nullable PictureParameters fmmHdrVivid;
}
