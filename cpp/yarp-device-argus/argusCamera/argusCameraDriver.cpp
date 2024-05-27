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
using namespace Argus;
using namespace EGLStream;

// VERY IMPORTANT ABOUT WHITE BALANCE: the YARP interfaces cannot allow to set a feature with
// 3 values, 2 is maximum and until now we always used blue and red in this order. Then we ignore
// green

static const std::vector<cameraFeature_id_t> supported_features{YARP_FEATURE_BRIGHTNESS, YARP_FEATURE_EXPOSURE, YARP_FEATURE_SHARPNESS, YARP_FEATURE_WHITE_BALANCE,
                                                                // YARP_FEATURE_GAMMA, // it seems not writable
                                                                YARP_FEATURE_GAIN,
                                                                // YARP_FEATURE_TRIGGER, // not sure how to use it
                                                                YARP_FEATURE_FRAME_RATE, YARP_FEATURE_HUE, YARP_FEATURE_FOCUS};

static const std::vector<cameraFeature_id_t> features_with_auto{YARP_FEATURE_EXPOSURE, YARP_FEATURE_WHITE_BALANCE, YARP_FEATURE_GAIN};

// Values taken from the balser documentation for IMX415-C FIXME! TO BE CHECKED!
static const std::map<cameraFeature_id_t, std::pair<double, double>> featureMinMax{{YARP_FEATURE_BRIGHTNESS, {-1.0, 1.0}}, //not supported (?)
                                                                                   {YARP_FEATURE_EXPOSURE, {-2.0, 2.0}}, //fixed
                                                                                   {YARP_FEATURE_SHARPNESS, {0.0, 1.0}}, //not supported (?)
                                                                                   {YARP_FEATURE_WHITE_BALANCE, {0.0, 1.0}},  //fixed // not sure about it, the doc is not clear, found empirically
                                                                                   //{YARP_FEATURE_GAMMA, {0.0, 4.0}},
                                                                                   {YARP_FEATURE_GAIN, {1.0, 3981.07}}}; //fixed

static const std::map<double, NV::Rotation> rotationToNVRot{{0.0, NV::ROTATION_0}, {90.0, NV::ROTATION_90}, {-90.0, NV::ROTATION_270}, {180.0, NV::ROTATION_180}};

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
    // auto res = setOption("AcquisitionFrameRate", _fps);
    bool res = true;
    if (res)
    {
        m_fps = _fps;
    }
    yCDebug(ARGUS_CAMERA) << "fps setFramerate" << m_fps;

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
    yCDebug(ARGUS_CAMERA) << "input params are " << config.toString();

    if(!parseParams(config)) {
        yCError(ARGUS_CAMERA) << "Error parsing parameters";
        return false;
    }

    if (m_period != 0.0)
    {
        // m_fps = 1.0 / m_period;  // the fps has to be aligned with the nws period

        m_fps = 60.0;
        // FIXME set_framerate
    }

    m_cameraProvider.reset(CameraProvider::create());
    ICameraProvider *iCameraProvider = interface_cast<ICameraProvider>(m_cameraProvider);
    if (!iCameraProvider)
    {
        yCError(ARGUS_CAMERA) << "Failed to create CameraProvider";
    }

    /* Get the camera devices */
    iCameraProvider->getCameraDevices(&m_cameraDevices);
    if (m_cameraDevices.size() == 0)
    {
        yCError(ARGUS_CAMERA) << "No cameras available";
    }

    ICameraProperties *iCameraProperties = interface_cast<ICameraProperties>(m_cameraDevices[m_d]);
    if (!iCameraProperties)
    {
        yCError(ARGUS_CAMERA) << "Failed to get ICameraProperties interface";
    }

    iCameraProperties->getBasicSensorModes(&sensorModes);
    if (sensorModes.size() == 0)
    {
        yCError(ARGUS_CAMERA) << "Failed to get sensor modes";
    }
    
    if (m_d >= m_cameraDevices.size())
    {
        yCError(ARGUS_CAMERA) << "Camera device index d =" << m_d << "is invalid.";
    }

    /* Create the capture session using the first device and get the core interface */
    m_captureSession.reset(iCameraProvider->createCaptureSession(m_cameraDevices[m_d]));
    ICaptureSession *iCaptureSession = interface_cast<ICaptureSession>(m_captureSession);
    if (!iCaptureSession)
    {
        yCError(ARGUS_CAMERA) << "Failed to get ICaptureSession interface";
    }

    m_streamSettings.reset(iCaptureSession->createOutputStreamSettings(STREAM_TYPE_EGL));
    IEGLOutputStreamSettings *iEglStreamSettings = interface_cast<IEGLOutputStreamSettings>(m_streamSettings);
    if (!iEglStreamSettings)
    {
        yCError(ARGUS_CAMERA) << "Failed to get IEGLOutputStreamSettings interface";
    }

    iSensorMode = interface_cast<ISensorMode>(sensorModes[0]);
    // m_width = iSensorMode->getResolution().width(); 
    // m_height = iSensorMode->getResolution().height();
    Size2D<uint32_t> resolution{m_width, m_height};
    iEglStreamSettings->setPixelFormat(PIXEL_FMT_YCbCr_420_888);
    iEglStreamSettings->setResolution(resolution);

    m_stream.reset(iCaptureSession->createOutputStream(m_streamSettings.get()));
    m_consumer.reset(FrameConsumer::create(m_stream.get()));

    if (!m_consumer)
    {
        yCError(ARGUS_CAMERA) << "Failed to create FrameConsumer";
    }

    m_request.reset(iCaptureSession->createRequest(Argus::CAPTURE_INTENT_PREVIEW));
    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(m_request);
    if (iRequest->enableOutputStream(m_stream.get()) != STATUS_OK)
    {
        yCError(ARGUS_CAMERA) << "Failed to enable output stream";
    }

    ISourceSettings *iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
    if (!iSourceSettings)
    {
        yCError(ARGUS_CAMERA) << "Failed to get ISourceSettings interface";
    }

    iCaptureSession->repeat(m_request.get());

    ok = ok && setRgbResolution(m_width, m_height);
    ok = ok && setFramerate(m_fps);

    IAutoControlSettings *iAutoControlSettings = interface_cast<IAutoControlSettings>(iRequest->getAutoControlSettings());
    yDebug() << "r:" << iAutoControlSettings->getWbGains().r();
    yDebug() << "b:" << iAutoControlSettings->getWbGains().b();
    yDebug() << "g odd:" << iAutoControlSettings->getWbGains().gOdd();
    yDebug() << "g even:" << iAutoControlSettings->getWbGains().gEven();
    yDebug() <<  "saturation"<< iAutoControlSettings->getColorSaturation();
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
    // bool res = false;
    // if (width > 0 && height > 0)
    // {
    //     // FIXME change the resolution
    //     if (res)
    //     {
    //         m_width = width;
    //         m_height = height;
    //     }
    // }

    bool res = true;
    if (res)
    {
        m_width = width;
        m_height = height;
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

    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(m_request);
    ISourceSettings *iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
    IAutoControlSettings *iAutoControlSettings = interface_cast<IAutoControlSettings>(iRequest->getAutoControlSettings());
    ICaptureSession *iCaptureSession = interface_cast<ICaptureSession>(m_captureSession);
    iCaptureSession->stopRepeat();

    switch (f)
    {
        case YARP_FEATURE_BRIGHTNESS:
            //FIXME
            break;
        case YARP_FEATURE_EXPOSURE:
            // iSourceSettings->setExposureTimeRange(fromZeroOneToRange(f, value));
            iAutoControlSettings->setExposureCompensation(fromZeroOneToRange(f, value));
            yDebug() << "exposure value:" << fromZeroOneToRange(f, value);
            b = true;
            break;
        case YARP_FEATURE_SHARPNESS:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            iSourceSettings->setGainRange(fromZeroOneToRange(f, value));
            break;
        case YARP_FEATURE_FRAME_RATE:
            b = setFramerate(value);
            yCDebug(ARGUS_CAMERA) << "fps setFeature" << value;
            break;
        default:
            yCError(ARGUS_CAMERA) << "Feature not supported!";
            return false;
    }

    iCaptureSession->repeat(m_request.get());

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

    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(m_request);
    ISourceSettings *iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
    IAutoControlSettings *iAutoControlSettings = interface_cast<IAutoControlSettings>(iRequest->getAutoControlSettings());

    switch (f)
    {
        case YARP_FEATURE_BRIGHTNESS:
            //FIXME
            break;
        case YARP_FEATURE_EXPOSURE:
            //FIXME
            // b = true;
            // *value = iSourceSettings->getExposureTimeRange().min();
            *value = iAutoControlSettings->getExposureCompensation();
            yDebug() << "get exposure value" << *value;
            b = true;
            break;
        case YARP_FEATURE_SHARPNESS:
            //FIXME
            break;
        case YARP_FEATURE_WHITE_BALANCE:
            //FIXME
            break;
        case YARP_FEATURE_GAIN:
            //FIXME
            *value = iSourceSettings->getGainRange().min();
            b = true;
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
    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(m_request);
    ISourceSettings *iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
    IAutoControlSettings *iAutoControlSettings = interface_cast<IAutoControlSettings>(iRequest->getAutoControlSettings());
    ICaptureSession *iCaptureSession = interface_cast<ICaptureSession>(m_captureSession);
    iCaptureSession->stopRepeat();

    if (f != YARP_FEATURE_WHITE_BALANCE)
    {
        yCError(ARGUS_CAMERA) << YARP_FEATURE_WHITE_BALANCE << "is not a 2-values feature supported";
        return false;
    }

    //FIXME set the feature
    // BayerTuple<float> wbGains(value1, iAutoControlSettings->getWbGains().gEven(), iAutoControlSettings->getWbGains().gOdd(), value2);
    // iAutoControlSettings->setWbGains(wbGains);
    // yDebug() << fromZeroOneToRange(f, value1);

    iCaptureSession->repeat(m_request.get());

    return res;
}

bool argusCameraDriver::getFeature(int feature, double* value1, double* value2)
{
    auto f = static_cast<cameraFeature_id_t>(feature);
    auto res = true;

    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(m_request);
    ISourceSettings *iSourceSettings = interface_cast<ISourceSettings>(iRequest->getSourceSettings());
    IAutoControlSettings *iAutoControlSettings = interface_cast<IAutoControlSettings>(iRequest->getAutoControlSettings());

    if (f != YARP_FEATURE_WHITE_BALANCE)
    {
        yCError(ARGUS_CAMERA) << "This is not a 2-values feature supported";
        return false;
    }

    // iAutoControlSettings->setAwbMode(AWB_MODE_MANUAL);
    // *value1 = fromRangeToZeroOne(f, iAutoControlSettings->getWbGains().r());
    // *value2 = fromRangeToZeroOne(f, iAutoControlSettings->getWbGains().b());;
    // yCDebug(ARGUS_CAMERA) << "In 0-1 r" << *value1;
    // yCDebug(ARGUS_CAMERA) << "In 0-1 b" << *value2;
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

    NvBufSurface* nvBufSurface = nullptr;

    IFrameConsumer *iFrameConsumer = interface_cast<IFrameConsumer>(m_consumer);
    UniqueObj<Frame> frame(iFrameConsumer->acquireFrame(TIMEOUT_INFINITE));
    IFrame *iFrame = interface_cast<IFrame>(frame);

    if(iFrame)
    {
        auto img = iFrame->getImage();
        auto image2d(Argus::interface_cast<EGLStream::IImage2D>(img));
        m_width = image2d->getSize()[0];
        m_height = image2d->getSize()[1];

        NV::IImageNativeBuffer *iNativeBuffer = interface_cast<NV::IImageNativeBuffer>(img);
        if (!iNativeBuffer)
        {
            yCError(ARGUS_CAMERA) << "IImageNativeBuffer not supported by IImage"; 
        }

        // Create NvBuffer
        int fd = iNativeBuffer->createNvBuffer(image2d->getSize(), NVBUF_COLOR_FORMAT_RGBA, NVBUF_LAYOUT_PITCH, rotationToNVRot.at(m_rotation));
        if (fd == -1)
        {
            yCError(ARGUS_CAMERA) << "Failed to create NvBuffer";
        }

        if (NvBufSurfaceFromFd(fd, (void**)(&nvBufSurface)) == -1)
        {
            yCError(ARGUS_CAMERA) << "Cannot get NvBufSurface from fd";
        }

        if (NvBufSurfaceMap(nvBufSurface, 0, 0, NVBUF_MAP_READ) != STATUS_OK)
        {
            yCError(ARGUS_CAMERA) << "Failed to map NvBufSurface";
        }

        // Create OpenCV Mat from the buffer
        cv::Mat rgba_img(m_height, m_width, CV_8UC4, nvBufSurface->surfaceList->mappedAddr.addr[0]);

        // Convert color from RGBA to BGR
        cv::Mat bgr_img;
        cv::cvtColor(rgba_img, bgr_img, cv::COLOR_RGBA2BGR);
        
        image.resize(m_width, m_height);
        image.copy(yarp::cv::fromCvMat<yarp::sig::PixelRgb>(bgr_img));

        // Unmap the buffer memory
        if (NvBufSurfaceUnMap(nvBufSurface, 0, 0) != STATUS_OK) 
        {
            yCError(ARGUS_CAMERA) << "Failed to unmap NvBufSurface";
        }

        if (NvBufSurfaceDestroy(nvBufSurface) != STATUS_OK) 
        {
            yCError(ARGUS_CAMERA) << "Failed to free the NvBufSurface";
        }
    }

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
