/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.tv.mediaquality.ColorRange;
import android.hardware.tv.mediaquality.ColorSpace;
import android.hardware.tv.mediaquality.ColorTemperature;
import android.hardware.tv.mediaquality.Gamma;
import android.hardware.tv.mediaquality.PictureQualityEventType;
import android.hardware.tv.mediaquality.QualityLevel;

/**
 * The parameters for Picture Profile.
 */
@VintfStability
union PictureParameter {
    /*
     * Brightness field represents the brightness level of the TV.
     * Brightness value range are from 0.0 to 1.0, where 0.0 represents the minimum brightness and
     * 1.0 represents the maximum brightness. The content-unmodified value is 0.5.
     *
     * note: when a picture profile is applied to the entire display, the media quality framework
     * will synchronize the brightness field.
     */
    float brightness;

    /*
     * This value represents the image contrast on an arbitrary scale from 0 to 100,
     * where 0 represents the darkest black (black screen) and 100 represents the brightest
     * white (brighter).
     * The default/unmodified value for contrast is 50.
     */
    int contrast;

    /*
     * Control that increases edge contrast so that objects become more distinct.
     * Sharpness value range are from 0 to 100, where 0 represents the minimum sharpness that
     * makes the image appear softer with less defined edges, 100 represents the maximum
     * sharpness that makes the image appear halos around objects due to excessive edges.
     * The default/unmodified value for sharpness is 50.
     */
    int sharpness;

    /*
     * Saturation value controls the intensity or purity of colors.
     * Saturation values are from 0 to 100, where 0 represents grayscale (no color) and 100
     * represents the most vivid colors.
     * The default/unmodified value for saturation is 50.
     */
    int saturation;

    /*
     * Hue affects the balance between red, green and blue primary colors on the screen.
     * Hue values are from -50 to 50, where -50 represents cooler and 50 represents warmer.
     * The default/unmodified value for hue is 0.
     */
    int hue;

    /*
     * Adjust brightness in advance color engine. Similar to a "brightness" control on a TV
     * but acts at a lower level.
     *
     * The range is from 0 to 100, where 0 represents the minimum brightness and 100 represents
     * the maximum brightness. The default/unmodified value is 50.
     */
    int colorTunerBrightness;

    /*
     * Adjust saturation in advance color engine. Similar to a "saturation" control on a TV
     * but acts at a lower level.
     *
     * The range is from 0 to 100, where 0 being completely desaturated/grayscale and 100 being
     * the most saturated. The default/unmodified value is 50.
     */
    int colorTunerSaturation;

    /*
     * Adjust hue in advance color engine. Similar to a "hue" control on a TV but acts at a lower
     * level.
     *
     * The range is from -50 to 50, where -50 represents cooler setting for a specific color and 50
     * represents warmer setting for a specific color. The default/unmodified value is 0.
     */
    int colorTunerHue;

    /*
     * Advance setting for red offset. Adjust the black level of red color channels, it control
     * the minimum intensity of each color, affecting the shadows and dark areas of the image.
     *
     * The range is from 0 to 100, where 0 makes shadows darker and 100 makes shadows brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerRedOffset;

    /*
     * Advance setting for green offset. Adjust the black level of green color channels, it control
     * the minimum intensity of each color, affecting the shadows and dark areas of the image.
     *
     * The range is from 0 to 100, where 0 makes shadows darker and 100 makes shadows brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerGreenOffset;

    /*
     * Advance setting for blue offset. Adjust the black level of blue color channels, it control
     * the minimum intensity of each color, affecting the shadows and dark areas of the image.
     *
     * The range is from 0 to 100, where 0 makes shadows darker and 100 makes shadows brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerBlueOffset;

    /*
     * Advance setting for red gain. Adjust the gain or amplification of the red color channels.
     * They control the overall intensity and white balance of red.
     *
     * The range is from 0 to 100, where 0 makes the red dimmer and 100 makes the red brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerRedGain;

    /*
     * Advance setting for green gain. Adjust the gain or amplification of the green color channels.
     * They control the overall intensity and white balance of green.
     *
     * The range is from 0 to 100, where 0 makes the green dimmer and 100 makes the green brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerGreenGain;

    /*
     * Advance setting for blue gain. Adjust the gain or amplification of the blue color channels.
     * They control the overall intensity and white balance of blue.
     *
     * The range is from 0 to 100, where 0 makes the blue dimmer and 100 makes the blue brighter.
     * The default/unmodified value is 50.
     */
    int colorTunerBlueGain;

    /* Noise reduction. (Off, Low, Medium, High) */
    QualityLevel noiseReduction;

    /* MPEG (moving picture experts group) noise reduction (Off, Low, Medium, High) */
    QualityLevel mpegNoiseReduction;

    /*
     * Refine the flesh colors in the pictures without affecting the other colors on the screen.
     * (Off, Low, Medium, High)
     */
    QualityLevel fleshTone;

    /* Contour noise reduction. (Off, Low, Medium, High) */
    QualityLevel deContour;

    /* Dynamically change picture luma to enhance contrast. (Off, Low, Medium, High) */
    QualityLevel dynamicLumaControl;

    /* Enable/disable film mode */
    boolean filmMode;

    /* Enable/disable blue color auto stretch */
    boolean blueStretch;

    /* Enable/disable the overall color tuning feature. */
    boolean colorTune;

    /* Adjust color temperature type */
    ColorTemperature colorTemperature;

    /* Enable/disable globe dimming. */
    boolean globeDimming;

    /* Enable/disable auto adjust picture parameter based on the TV content. */
    boolean autoPictureQualityEnabled;

    /*
     * Enable/disable auto upscaling the picture quality. It analyzes the lower-resolution
     * image and uses its knowledge to invent the missing pixel, make the image look sharper.
     */
    boolean autoSuperResolutionEnabled;

    /**
     * The color range of the content. This indicates the range of luminance values
     * used in the video signal.
     */
    ColorRange levelRange;

    /**
     * Enable/disable gamut mapping. Gamut mapping is a process that adjusts
     * the colors in the video signal to match the color gamut of the display.
     */
    boolean gamutMapping;

    /**
     * Enable/disable PC mode. PC mode is a display mode that is optimized for
     * use with computers.
     */
    boolean pcMode;

    /**
     * Enable/disable low latency mode. Low latency mode reduces the delay
     * between the video source and the display.
     */
    boolean lowLatency;

    /**
     * Enable/disable variable refresh rate (VRR) mode. VRR allows the display to
     * dynamically adjust its refresh rate to match the frame rate of the video
     * source, reducing screen tearing.
     */
    boolean vrr;

    /**
     * Enable/disable continuous variable refresh rate (CVRR) mode. CVRR is a type
     * of VRR that allows for a wider range of refresh rates.
     */
    boolean cvrr;

    /**
     * The color range of the HDMI input. This indicates the range of luminance
     * values used in the HDMI signal.
     */
    ColorRange hdmiRgbRange;

    /**
     * The color space of the content. This indicates the color gamut and
     * transfer function used in the video signal.
     */
    ColorSpace colorSpace;

    /**
     * The initial maximum luminance of the panel, in nits.
     */
    int panelInitMaxLuminceNits;

    /**
     * Whether the initial maximum luminance value is valid.
     */
    boolean panelInitMaxLuminceValid;

    /* The gamma curve used for the display. */
    Gamma gamma;

    /**
     * The color gain value for color temperature adjustment.
     * The value adjusts the intensity of color in the bright areas on the TV.
     *
     * The value range is from -100 to 100 where -100 would eliminate that color
     * and 100 would significantly boost that color.
     *
     * The default/unmodified value is 0. No adjustment is applied to that color.
     */
    int colorTemperatureRedGain;

    int colorTemperatureGreenGain;

    int colorTemperatureBlueGain;

    /**
     * The color offset value for color temperature adjustment.
     * This value adjusts the intensity of color in the dark areas on the TV.
     *
     * The value range is from -100 to 100 where -100 would eliminate that color
     * and 100 would significantly boost that color.
     *
     * The default/unmodified value is 0. No adjustment is applied to that color.
     */
    int colorTemperatureRedOffset;

    int colorTemperatureGreenOffset;

    int colorTemperatureBlueOffset;

    /**
     * The parameters in this section is for 11-point white balance in advanced TV picture setting.
     * 11-Point White Balance allows for very precise adjustment of the color temperature of the
     * TV. It aims to make sure white looks truly white, without any unwanted color tints, across
     * the entire range of brightness levels.
     *
     * The "11 points" refer to 11 different brightness levels from 0 (black) to 10 (white).
     * At each of these points, we can fine-tune the mixture of red, green and blue to achieve
     * neutral white.
     *
     * These parameters specifically control the amount of red, blue or green at each of the 11
     * brightness points. The parameter type is an int array with a fix size of 11. The indexes
     * 0 - 10 are the 11 different points. For example, elevenPointRed[0] adjusts the red level
     * at the darkest black level. elevenPointRed[1] adjusts red at the next brightness level up,
     * and so on.
     *
     * The value range is from 0 - 100 for each indexes, where 0 is the minimum intensity of
     * that color(red, green, blue) at a specific brightness point and 100 is the maximum intensity
     * of that color at that point.
     *
     * The default/unmodified value is 50. It can be other values depends on different TVs.
     */
    int[11] elevenPointRed;

    int[11] elevenPointGreen;

    int[11] elevenPointBlue;

    /**
     * Adjust gamma blue gain/offset.
     *
     * The default value is middle. Can be different depends on different TVs.
     */
    QualityLevel lowBlueLight;

    /**
     * Advance setting for local dimming level.
     *
     * The default value is off. Can be different depends on different TVs.
     */
    QualityLevel LdMode;

    /**
     * The parameter in this section is for on-screen display color gain and offset.
     *
     * Color gain is to adjust the intensity of that color (red, blue, green) in the brighter
     * part of the image.
     * Color offset is to adjust the intensity of that color in the darker part of the image.
     *
     * For example, increase OSD (on-screen display) red gain will make brighter reds even more
     * intense, while decreasing it will make them less vibrant. Increase OSD red offset will add
     * more red to the darker areas, while decreasing it will reduce the red in the shadows.
     *
     * The value range is from 0 to 2047. (11-bit resolution for the adjustment)
     * The default value depends on different TVs.
     */
    int osdRedGain;

    int osdGreenGain;

    int osdBlueGain;

    int osdRedOffset;

    int osdGreenOffset;

    int osdBlueOffset;

    /* The value range is 0-100 */
    int osdHue;

    /* The value range is 0-255 */
    int osdSaturation;

    int osdContrast;

    /**
     * Enable/disable color tuner.
     *
     * The color tuner can adjust color temperature and picture color.
     * The default is enabled.
     */
    boolean colorTunerSwitch;

    /**
     * The parameters in this section adjust the hue of each color.
     *
     * For example, increase colorTunerHueRed will make the image more purplish-red or more
     * orange-red. increase colorTunerHueGreen will make the image more yellowish-green or
     * more bluish-green. Flesh is a special one, it can make skin tones appear warmer (reddish)
     * or cooler (more yellowish).
     *
     * The value range is from 0 - 100, and the default value is 50.
     */
    int colorTunerHueRed;

    int colorTunerHueGreen;

    int colorTunerHueBlue;

    int colorTunerHueCyan;

    int colorTunerHueMagenta;

    int colorTunerHueYellow;

    int colorTunerHueFlesh;

    /**
     * The parameters in this section adjust the saturation of each color.
     *
     * For example, increase colorTunerSaturationBlue will make the color blue more deeper
     * and richer. Decrease will make the color blue more washed-out blues.
     *
     * The value range is from 0 -100, and the default value is 50.
     */
    int colorTunerSaturationRed;

    int colorTunerSaturationGreen;

    int colorTunerSaturationBlue;

    int colorTunerSaturationCyan;

    int colorTunerSaturationMagenta;

    int colorTunerSaturationYellow;

    int colorTunerSaturationFlesh;

    /**
     * The parameters in this section adjust the luminance (brightness) of each color.
     *
     * For example, increase colorTunerLuminanceRed will makes red appear brighter. Decrease
     * will makes red appear darker.
     *
     * The value range is from 0 -100, and the default value is 50.
     */
    int colorTunerLuminanceRed;

    int colorTunerLuminanceGreen;

    int colorTunerLuminanceBlue;

    int colorTunerLuminanceCyan;

    int colorTunerLuminanceMagenta;

    int colorTunerLuminanceYellow;

    int colorTunerLuminanceFlesh;

    /**
     * Determines whether the current profile is actively in use or not.
     */
    boolean activeProfile;

    /**
     * This indicates non picture parameter status change about a profile.
     *
     * Those status can be found in PictureQualityEventType.
     */
    PictureQualityEventType pictureQualityEventType;
}
