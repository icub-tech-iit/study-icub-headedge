/*
 * Copyright (C) 2006-2024 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

// #include <opencv2/core/core_c.h>
// #include <yarp/cv/Cv.h>
#include <yarp/os/LogComponent.h>
#include <yarp/os/Value.h>
#include <yarp/sig/ImageUtils.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
// #include <opencv2/opencv.hpp>
// #include <opencv2/videoio.hpp>
#if defined USE_CUDA
// #include <opencv2/cudawarping.hpp>
#endif  // USE_CUDA

#include "argusCameraDriver.h"

using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::os;

using namespace std;

// VERY IMPORTANT ABOUT WHITE BALANCE: the YARP interfaces cannot allow to set a feature with
// 3 values, 2 is maximum and until now we always used blue and red in this order. Then we ignore
// green

static const std::vector<cameraFeature_id_t> supported_features{YARP_FEATURE_BRIGHTNESS, YARP_FEATURE_EXPOSURE, YARP_FEATURE_SHARPNESS, YARP_FEATURE_WHITE_BALANCE,
                                                                // YARP_FEATURE_GAMMA, // it seems not writable
                                                                YARP_FEATURE_GAIN,
                                                                // YARP_FEATURE_TRIGGER, // not sure how to use it
                                                                YARP_FEATURE_FRAME_RATE};

static const std::vector<cameraFeature_id_t> features_with_auto{YARP_FEATURE_EXPOSURE, YARP_FEATURE_WHITE_BALANCE, YARP_FEATURE_GAIN};

// Values taken from the balser documentation for IMX415-C FIXME! TO BE CHECKED!
static const std::map<cameraFeature_id_t, std::pair<double, double>> featureMinMax{{YARP_FEATURE_BRIGHTNESS, {-1.0, 1.0}},
                                                                                   {YARP_FEATURE_EXPOSURE, {68.0, 2300000.0}},
                                                                                   {YARP_FEATURE_SHARPNESS, {0.0, 1.0}},
                                                                                   {YARP_FEATURE_WHITE_BALANCE, {1.0, 8.0}},  // not sure about it, the doc is not clear, found empirically
                                                                                   //{YARP_FEATURE_GAMMA, {0.0, 4.0}},
                                                                                   {YARP_FEATURE_GAIN, {0.0, 33.06}}};

//static const std::map<double, int> rotationToCVRot{{90.0, ROTATE_90_CLOCKWISE}, {-90.0, ROTATE_90_COUNTERCLOCKWISE}, {180.0, ROTATE_180}};

// We usually set the features through a range between 0 an 1, we have to translate it in meaninful value for the camera
double fromZeroOneToRange(cameraFeature_id_t feature, double value)
{
    return value * (featureMinMax.at(feature).second - featureMinMax.at(feature).first) + featureMinMax.at(feature).first;
}

// We want the features in the range 0 1
double fromRangeToZeroOne(cameraFeature_id_t feature, double value)
{
    return (value - featureMinMax.at(feature).first) / (featureMinMax.at(feature).second - featureMinMax.at(feature).first);
}

bool argusCameraDriver::setFramerate(const float _fps)
{
    auto res = setOption("AcquisitionFrameRate", _fps);
    if (res)
    {
        m_fps = _fps;
    }
    return res;
}

bool parseUint32Param(std::string param_name, std::uint32_t& param, yarp::os::Searchable& config)
{
    if (config.check(param_name) && config.find(param_name).isInt32())
    {
        param = config.find(param_name).asInt32();
        return true;
    }
    else
    {
        yCWarning(ARGUS_CAMERA) << param_name << "parameter not specifie, using" << param;
        return false;
    }
}

bool argusCameraDriver::startCamera()
{
    return true;
}

bool argusCameraDriver::stopCamera()
{
    return true;
}

bool argusCameraDriver::open(Searchable& config)
{
    bool ok{true};
    yCTrace(ARGUS_CAMERA) << "input params are " << config.toString();

    // FIXME parse generator part

    if (m_rotationWithCrop)
    {
        if (m_rotation == -90.0 || m_rotation == 90.0)
        {
            std::swap(m_width, m_height);
        }
        yCDebug(ARGUS_CAMERA) << "Rotation with crop";
    }

    double period = 0.0;  // FIXME we will parse it from the config

    if (period != 0.0)
    {
        m_fps = 1.0 / period;  // the fps has to be aligned with the nws period
    }

    return ok && startCamera();
}

bool argusCameraDriver::close()
{
    return true;
}

int argusCameraDriver::getRgbHeight()
{
    return m_height;
}

int argusCameraDriver::getRgbWidth()
{
    return m_width;
}

bool argusCameraDriver::getRgbSupportedConfigurations(yarp::sig::VectorOf<CameraConfig>& configurations)
{
    yCWarning(ARGUS_CAMERA) << "getRgbSupportedConfigurations not implemented yet";
    return false;
}

bool argusCameraDriver::getRgbResolution(int& width, int& height)
{
    width = m_width;
    height = m_height;
    return true;
}

bool argusCameraDriver::setRgbResolution(int width, int height)
{
    bool res = false;
    if (width > 0 && height > 0)
    {
        // FIXME change the resolution
        if (res)
        {
            m_width = width;
            m_height = height;
        }
    }
    return res;
}

bool argusCameraDriver::setRgbFOV(double horizontalFov, double verticalFov)
{
    yCWarning(ARGUS_CAMERA) << "setRgbFOV not supported";
    return false;
}

bool argusCameraDriver::getRgbFOV(double& horizontalFov, double& verticalFov)
{
    yCWarning(ARGUS_CAMERA) << "getRgbFOV not supported";
    return false;
}

bool argusCameraDriver::getRgbMirroring(bool& mirror)
{
    yCWarning(ARGUS_CAMERA) << "Mirroring not supported";
    return false;
}

bool argusCameraDriver::setRgbMirroring(bool mirror)
{
    yCWarning(ARGUS_CAMERA) << "Mirroring not supported";
    return false;
}

bool argusCameraDriver::getRgbIntrinsicParam(Property& intrinsic)
{
    yCWarning(ARGUS_CAMERA) << "getRgbIntrinsicParam not implemented yet";
    return false;
}

bool argusCameraDriver::getCameraDescription(CameraDescriptor* camera)
{
    yCWarning(ARGUS_CAMERA) << "getCameraDescription not implemented yet";
    return false;
}

bool argusCameraDriver::hasFeature(int feature, bool* hasFeature)
{
    cameraFeature_id_t f;
    f = static_cast<cameraFeature_id_t>(feature);
    if (f < YARP_FEATURE_BRIGHTNESS || f > YARP_FEATURE_NUMBER_OF - 1)
    {
        return false;
    }

    *hasFeature = std::find(supported_features.begin(), supported_features.end(), f) != supported_features.end();

    return true;
}

bool argusCameraDriver::setFeature(int feature, double value)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature not supported!";
        return false;
    }
    b = false;
    auto f = static_cast<cameraFeature_id_t>(feature);
    switch (f)
    {
        case YARP_FEATURE_BRIGHTNESS:
            //FIXME
            break;
        case YARP_FEATURE_EXPOSURE:
            //FIXME
            break;
        case YARP_FEATURE_SHARPNESS:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            break;
        case YARP_FEATURE_FRAME_RATE:
            //FIXME
            break;
        default:
            yCError(ARGUS_CAMERA) << "Feature not supported!";
            return false;
    }

    return b;
}

bool argusCameraDriver::getFeature(int feature, double* value)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature not supported!";
        return false;
    }
    b = false;
    auto f = static_cast<cameraFeature_id_t>(feature);
    switch (f)
    {
        case YARP_FEATURE_BRIGHTNESS:
            //FIXME
            break;
        case YARP_FEATURE_EXPOSURE:
            //FIXME
            break;
        case YARP_FEATURE_SHARPNESS:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            b = false;
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            break;
        case YARP_FEATURE_FRAME_RATE:
            b = true;
            //FIXME
            break;
        default:
            yCError(ARGUS_CAMERA) << "Feature not supported!";
            return false;
    }

    *value = fromRangeToZeroOne(f, *value);
    yCDebug(ARGUS_CAMERA) << "In 0-1" << *value;
    return b;
}

bool argusCameraDriver::setFeature(int feature, double value1, double value2)
{
    auto f = static_cast<cameraFeature_id_t>(feature);
    auto res = true;
    if (f != YARP_FEATURE_WHITE_BALANCE)
    {
        yCError(ARGUS_CAMERA) << YARP_FEATURE_WHITE_BALANCE << "is not a 2-values feature supported";
        return false;
    }

    //FIXME set the feature
    return res;
}

bool argusCameraDriver::getFeature(int feature, double* value1, double* value2)
{
    auto f = static_cast<cameraFeature_id_t>(feature);
    auto res = true;
    if (f != YARP_FEATURE_WHITE_BALANCE)
    {
        yCError(ARGUS_CAMERA) << "This is not a 2-values feature supported";
        return false;
    }

    // FIXME
    return res;
}

bool argusCameraDriver::hasOnOff(int feature, bool* HasOnOff)
{
    return hasAuto(feature, HasOnOff);
}

bool argusCameraDriver::setActive(int feature, bool onoff)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
        return false;
    }

    if (!hasOnOff(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "does not have OnOff.. call hasOnOff() to know if a specific feature support OnOff mode";
        return false;
    }

    std::string val_to_set = onoff ? "Continuous" : "Off";

    switch (feature)
    {
        //FIXME all the autos feature to be enabled or disabled
        case YARP_FEATURE_EXPOSURE:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            break;
        default:
            yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
            return false;
    }

    return b;
}

bool argusCameraDriver::getActive(int feature, bool* isActive)
{
    bool b = false;
    if (!hasFeature(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
        return false;
    }

    if (!hasOnOff(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "does not have OnOff.. call hasOnOff() to know if a specific feature support OnOff mode";
        return false;
    }

    std::string val_to_get{""};

    switch (feature)
    {
        case YARP_FEATURE_EXPOSURE:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            break;
        default:
            yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
            return false;
    }
    if (b)
    {
        if (val_to_get == "Continuous")
        {
            *isActive = true;
        }
        else if (val_to_get == "Off")
        {
            *isActive = false;
        }
    }
    return b;
}

bool argusCameraDriver::hasAuto(int feature, bool* hasAuto)
{
    cameraFeature_id_t f;
    f = static_cast<cameraFeature_id_t>(feature);
    if (f < YARP_FEATURE_BRIGHTNESS || f > YARP_FEATURE_NUMBER_OF - 1)
    {
        return false;
    }

    *hasAuto = std::find(features_with_auto.begin(), features_with_auto.end(), f) != features_with_auto.end();

    return true;
}

bool argusCameraDriver::hasManual(int feature, bool* hasManual)
{
    return hasFeature(feature, hasManual);
}

bool argusCameraDriver::hasOnePush(int feature, bool* hasOnePush)
{
    return hasAuto(feature, hasOnePush);
}

bool argusCameraDriver::setMode(int feature, FeatureMode mode)
{
    bool b{false};
    if (!hasAuto(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
        return false;
    }

    switch (mode)
    {
        case MODE_AUTO:
            return setActive(feature, true);
        case MODE_MANUAL:
            return setActive(feature, false);
        case MODE_UNKNOWN:
            return false;
        default:
            return false;
    }
    return b;
}

bool argusCameraDriver::getMode(int feature, FeatureMode* mode)
{
    bool b{false};
    if (!hasAuto(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "not supported!";
        return false;
    }
    bool get_active{false};
    b = b && getActive(feature, &get_active);

    if (b)
    {
        if (get_active)
        {
            *mode = MODE_AUTO;
        }
        else
        {
            *mode = MODE_MANUAL;
        }
    }
    return b;
}

bool argusCameraDriver::setOnePush(int feature)
{
    bool b = false;
    if (!hasOnePush(feature, &b) || !b)
    {
        yCError(ARGUS_CAMERA) << "Feature" << feature << "doesn't have OnePush";
        return false;
    }

    b = b && setMode(feature, MODE_AUTO);
    b = b && setMode(feature, MODE_MANUAL);

    return b;
}

bool argusCameraDriver::getImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>& image)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    //FIXME
    return true;
}

int argusCameraDriver::height() const
{
    return m_height;
}

int argusCameraDriver::width() const
{
    return m_width;
}
