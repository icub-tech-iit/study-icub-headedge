/*
 * Copyright (C) 2006-2024 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef ARGUS_DRIVER_H
#define ARGUS_DRIVER_H

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>

#include "argusCameraDriver_ParamsParser.h"

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/IFrameGrabberControls.h>
#include <yarp/dev/IFrameGrabberImage.h>
#include <yarp/dev/IRgbVisualParams.h>
#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Stamp.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/all.h>

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <typeinfo>


namespace
{
YARP_LOG_COMPONENT(ARGUS_CAMERA, "yarp.device.argusCamera")
}

class argusCameraDriver : public yarp::dev::DeviceDriver,
                          public yarp::dev::IFrameGrabberControls,
                          public yarp::dev::IFrameGrabberImage,
                          public yarp::dev::IRgbVisualParams,
                          public argusCameraDriver_ParamsParser
{
   public:
    argusCameraDriver() = default;
    ~argusCameraDriver() override = default;

    // DeviceDriver
    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    // IRgbVisualParams
    int getRgbHeight() override;
    int getRgbWidth() override;
    bool getRgbSupportedConfigurations(yarp::sig::VectorOf<yarp::dev::CameraConfig>& configurations) override;
    bool getRgbResolution(int& width, int& height) override;
    bool setRgbResolution(int width, int height) override;
    bool getRgbFOV(double& horizontalFov, double& verticalFov) override;
    bool setRgbFOV(double horizontalFov, double verticalFov) override;
    bool getRgbMirroring(bool& mirror) override;
    bool setRgbMirroring(bool mirror) override;
    bool getRgbIntrinsicParam(yarp::os::Property& intrinsic) override;

    // IFrameGrabberControls
    bool getCameraDescription(CameraDescriptor* camera) override;
    bool hasFeature(int feature, bool* hasFeature) override;
    bool setFeature(int feature, double value) override;
    bool getFeature(int feature, double* value) override;
    bool setFeature(int feature, double value1, double value2) override;
    bool getFeature(int feature, double* value1, double* value2) override;
    bool hasOnOff(int feature, bool* HasOnOff) override;
    bool setActive(int feature, bool onoff) override;
    bool getActive(int feature, bool* isActive) override;
    bool hasAuto(int feature, bool* hasAuto) override;
    bool hasManual(int feature, bool* hasManual) override;
    bool hasOnePush(int feature, bool* hasOnePush) override;
    bool setMode(int feature, FeatureMode mode) override;
    bool getMode(int feature, FeatureMode* mode) override;
    bool setOnePush(int feature) override;

    // IFrameGrabberImage
    bool getImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>& image) override;
    int height() const override;
    int width() const override;

   private:
    // method
    // inline bool setParams();
    bool setFramerate(const float _fps);
    template <class T>
    bool setOption(const std::string& option, T value, bool isEnum = false)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        // in some cases it is not used, suppressing the warning
        YARP_UNUSED(isEnum);
        bool ok{true};
        stopCamera();
        yCDebug(ARGUS_CAMERA) << "Setting " << option << "to" << value;
        if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value)
        {
            // FIXME
        }
        else if constexpr (std::is_same<T, bool>::value)
        {
            // FIXME
        }
        else if constexpr (std::is_same<T, int>::value)
        {
            // FIXME
        }
        else if constexpr (std::is_same<T, const char*>::value)
        {
            if (isEnum)
            {
                // FIXME
            }
            else
            {
                // FIXME
            }
        }
        else
        {
            yCError(ARGUS_CAMERA) << "Option" << option << "has a type not supported, type" << typeid(T).name();
            startCamera();
            return false;
            }
        return startCamera() && ok;
    }

    template <class T>
    bool getOption(const std::string& option, T& value, bool isEnum = false)
    {
        // in some cases it is not used, suppressing the warning
        YARP_UNUSED(isEnum);
        if constexpr (std::is_same<T, float*>::value || std::is_same<T, double*>::value)
        {
            // FIXME
            yCDebug(ARGUS_CAMERA) << "Getting" << option << "value:" << *value;
        }
        else if constexpr (std::is_same<T, bool*>::value)
        {
            // FIXME
            yCDebug(ARGUS_CAMERA) << "Getting" << option << "value:" << *value;
        }
        else if constexpr (std::is_same<T, int*>::value)
        {
            // FIXME
            yCDebug(ARGUS_CAMERA) << "Getting" << option << "value:" << *value;
        }
        else if constexpr (std::is_same<T, std::string>::value)
        {
            if (isEnum)
            {
                // FIXME
                yCDebug(ARGUS_CAMERA) << "Getting" << option << "value:" << value;
            }
            else
            {
                // FIXME
                yCDebug(ARGUS_CAMERA) << "Getting" << option << "value:" << value;
            }
        }
        else
        {
            yCError(ARGUS_CAMERA) << "Option" << option << "has a type not supported, type" << typeid(T).name();
            return false;
        }
        return true;
    }

    bool startCamera();
    bool stopCamera();

    mutable std::mutex m_mutex;

    yarp::os::Stamp m_rgb_stamp;
    mutable std::string m_lastError{""};
    bool m_verbose{false};
    bool m_initialized{false};
    float m_fps{30.0};

    Argus::UniqueObj<Argus::CameraProvider> m_cameraProvider;
    Argus::UniqueObj<Argus::OutputStream> m_stream;
    Argus::UniqueObj<Argus::Request> m_request;
    Argus::UniqueObj<Argus::CaptureSession> m_captureSession;
    Argus::UniqueObj<EGLStream::FrameConsumer> m_consumer;
    std::vector<Argus::CameraDevice*> m_cameraDevices;
};
#endif  // ARGUS_DRIVER_H
