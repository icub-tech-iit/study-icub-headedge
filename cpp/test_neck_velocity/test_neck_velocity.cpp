/******************************************************************************
 *                                                                            *
 * Copyright (C) 2024 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

#include <cstdlib>
#include <string>
#include <limits>
#include <vector>
#include <cmath>
#include <iostream>
#include <condition_variable>

#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Time.h>
#include <yarp/os/Value.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Property.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/ControlBoardInterfaces.h>

int main(int argc, char* argv[]) {
    yarp::os::Network yarp;
    if (!yarp.checkNetwork()) {
        yError() << "Unable to find YARP server!";
        return EXIT_FAILURE;
    }
    
    yarp::os::ResourceFinder rf;
    rf.configure(argc, argv);

    auto remote = rf.check("remote", yarp::os::Value("/icub/head")).asString();
    
    yarp::dev::PolyDriver driver;
    yarp::dev::IControlMode* iCtrl{nullptr};
    yarp::dev::IPositionControl* iPos{nullptr};
    yarp::dev::IControlLimits* iLim{nullptr};
    yarp::dev::IEncoders* iEnc{nullptr};
    
    yarp::os::Property conf;
    conf.put("device", "remote_controlboard");
    conf.put("remote", remote);
    conf.put("local", "/logger");
    if (!driver.open(conf)) {
        yError() << "Failed to connect to " << remote;
        return EXIT_FAILURE;
    }
    if (!(driver.view(iCtrl)&&
          driver.view(iPos) && driver.view(iLim) &&
          driver.view(iEnc))) {
        yError() << "Failed to open interfaces";
        driver.close();
        return EXIT_FAILURE;
    }
    
    int axes;
    double min, max, range;
    iPos->getAxes(&axes);

    for (int joint = 0; joint < axes; joint++)
    {
        iCtrl->setControlMode(joint, VOCAB_CM_POSITION);
        iPos->setRefSpeed(joint, 10.);
        iPos->setRefAcceleration(joint, std::numeric_limits<double>::max());
        iPos->positionMove(joint, 0.0);
        bool done = false;
        while (!done) {
            iPos->checkMotionDone(joint, &done);
            yarp::os::Time::delay(.1);
        }
    }

    for (int joint = 0; joint < axes; joint++) 
    {
        iLim->getLimits(joint, &min, &max);
        range = max - min;

        bool doneU = false;
        bool doneL = false;
        // std::vector<double> velocities = {20.0, 40.0, 60.0, 80.0, 100.0};
        double velocity = 100.0;
        int cycles = 10;

        for (int i = 0; i < cycles; i++)
        {
            yarp::os::Time::delay(2.);

            iPos->setRefSpeed(joint, velocity);
            iPos->setRefAcceleration(joint, std::numeric_limits<double>::max());
            iPos->positionMove(joint, max - 0.1 *range);
            while (!doneU) {
                iPos->checkMotionDone(joint, &doneU);
                yarp::os::Time::delay(.1);
            }

            yarp::os::Time::delay(2.);
            iPos->positionMove(joint, min + 0.1 * range);
            while (!doneL) {
                iPos->checkMotionDone(joint, &doneL);
                yarp::os::Time::delay(.1);
            }
        }

        iPos->positionMove(joint, 0.0);
    }
    
    yInfo() << "Done.";
    
    return EXIT_SUCCESS;
}