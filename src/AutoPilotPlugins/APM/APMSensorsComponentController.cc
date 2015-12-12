/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "APMSensorsComponentController.h"
#include "QGCMAVLink.h"
#include "UAS.h"
#include "QGCApplication.h"

#include <QVariant>
#include <QQmlProperty>

QGC_LOGGING_CATEGORY(APMSensorsComponentControllerLog, "APMSensorsComponentControllerLog")

APMSensorsComponentController::APMSensorsComponentController(void) :
    _statusLog(NULL),
    _progressBar(NULL),
    _compassButton(NULL),
    _accelButton(NULL),
    _nextButton(NULL),
    _cancelButton(NULL),
    _showOrientationCalArea(false),
    _gyroCalInProgress(false),
    _magCalInProgress(false),
    _accelCalInProgress(false),
    _orientationCalDownSideDone(false),
    _orientationCalUpsideDownSideDone(false),
    _orientationCalLeftSideDone(false),
    _orientationCalRightSideDone(false),
    _orientationCalNoseDownSideDone(false),
    _orientationCalTailDownSideDone(false),
    _orientationCalDownSideVisible(false),
    _orientationCalUpsideDownSideVisible(false),
    _orientationCalLeftSideVisible(false),
    _orientationCalRightSideVisible(false),
    _orientationCalNoseDownSideVisible(false),
    _orientationCalTailDownSideVisible(false),
    _orientationCalDownSideInProgress(false),
    _orientationCalUpsideDownSideInProgress(false),
    _orientationCalLeftSideInProgress(false),
    _orientationCalRightSideInProgress(false),
    _orientationCalNoseDownSideInProgress(false),
    _orientationCalTailDownSideInProgress(false),
    _orientationCalDownSideRotate(false),
    _orientationCalUpsideDownSideRotate(false),
    _orientationCalLeftSideRotate(false),
    _orientationCalRightSideRotate(false),
    _orientationCalNoseDownSideRotate(false),
    _orientationCalTailDownSideRotate(false),
    _waitingForCancel(false)
{

}

/// Appends the specified text to the status log area in the ui
void APMSensorsComponentController::_appendStatusLog(const QString& text)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void APMSensorsComponentController::_startLogCalibration(void)
{
    _hideAllCalAreas();
    
    connect(_uas, &UASInterface::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _nextButton->setEnabled(true);
    _cancelButton->setEnabled(false);
}

void APMSensorsComponentController::_startVisualCalibration(void)
{
    _compassButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _cancelButton->setEnabled(true);

    _resetInternalState();
    
    _progressBar->setProperty("value", 0);
}

void APMSensorsComponentController::_resetInternalState(void)
{
    _orientationCalDownSideDone = true;
    _orientationCalUpsideDownSideDone = true;
    _orientationCalLeftSideDone = true;
    _orientationCalRightSideDone = true;
    _orientationCalTailDownSideDone = true;
    _orientationCalNoseDownSideDone = true;
    _orientationCalDownSideInProgress = false;
    _orientationCalUpsideDownSideInProgress = false;
    _orientationCalLeftSideInProgress = false;
    _orientationCalRightSideInProgress = false;
    _orientationCalNoseDownSideInProgress = false;
    _orientationCalTailDownSideInProgress = false;
    _orientationCalDownSideRotate = false;
    _orientationCalUpsideDownSideRotate = false;
    _orientationCalLeftSideRotate = false;
    _orientationCalRightSideRotate = false;
    _orientationCalNoseDownSideRotate = false;
    _orientationCalTailDownSideRotate = false;

    emit orientationCalSidesRotateChanged();
    emit orientationCalSidesDoneChanged();
    emit orientationCalSidesInProgressChanged();
}

void APMSensorsComponentController::_stopCalibration(APMSensorsComponentController::StopCalibrationCode code)
{
    disconnect(_uas, &UASInterface::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(true);
    _accelButton->setEnabled(true);
    _nextButton->setEnabled(false);
    _cancelButton->setEnabled(false);
    
    if (code == StopCalibrationSuccess) {
        _resetInternalState();
        _progressBar->setProperty("value", 1);
    } else {
        _progressBar->setProperty("value", 0);
    }
    
    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();
    
    switch (code) {
        case StopCalibrationSuccess:
            _orientationCalAreaHelpText->setProperty("text", "Calibration complete");
            emit resetStatusTextArea();
            if (_magCalInProgress) {
                emit setCompassRotations();
            }
            break;
            
        case StopCalibrationCancelled:
            emit resetStatusTextArea();
            _hideAllCalAreas();
            break;
            
        default:
            // Assume failed
            _hideAllCalAreas();
            qgcApp()->showMessage("Calibration failed. Calibration log will be displayed.");
            break;
    }
    
    _magCalInProgress = false;
    _accelCalInProgress = false;
    _gyroCalInProgress = false;
}

void APMSensorsComponentController::calibrateGyro(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationGyro);
}

void APMSensorsComponentController::calibrateCompass(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationMag);
}

void APMSensorsComponentController::calibrateAccel(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAccel);
}

void APMSensorsComponentController::calibrateLevel(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationLevel);
}

void APMSensorsComponentController::calibrateAirspeed(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAirspeed);
}

void APMSensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->vehicle()->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }

    if (text.startsWith("PreArm:") || text.startsWith("EKF") || text.startsWith("Arm") || text.startsWith("Initialising")) {
        return;
    }

    QString anyKey("and press any");
    if (text.contains(anyKey)) {
        text = text.left(text.indexOf(anyKey)) + "and click Next to continue.";
    }

    _appendStatusLog(text);
    qCDebug(APMSensorsComponentControllerLog) << text << severity;

    if (text.contains("Calibration successful")) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }

    if (text.contains("FAILED")) {
        _stopCalibration(StopCalibrationFailed);
        return;
    }

/*
    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());
        
        _startVisualCalibration();
        
        text = parts[1];
        if (text == "accel" || text == "mag" || text == "gyro") {
            // Reset all progress indication
            _orientationCalDownSideDone = false;
            _orientationCalUpsideDownSideDone = false;
            _orientationCalLeftSideDone = false;
            _orientationCalRightSideDone = false;
            _orientationCalTailDownSideDone = false;
            _orientationCalNoseDownSideDone = false;
            _orientationCalDownSideInProgress = false;
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalLeftSideInProgress = false;
            _orientationCalRightSideInProgress = false;
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalTailDownSideInProgress = false;
            
            // Reset all visibility
            _orientationCalDownSideVisible = false;
            _orientationCalUpsideDownSideVisible = false;
            _orientationCalLeftSideVisible = false;
            _orientationCalRightSideVisible = false;
            _orientationCalTailDownSideVisible = false;
            _orientationCalNoseDownSideVisible = false;
            
            _orientationCalAreaHelpText->setProperty("text", "Place your vehicle into one of the Incomplete orientations shown below and hold it still");
            
            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {
                _magCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "gyro") {
                _gyroCalInProgress = true;
                _orientationCalDownSideVisible = true;
            } else {
                Q_ASSERT(false);
            }
            emit orientationCalSidesDoneChanged();
            emit orientationCalSidesVisibleChanged();
            emit orientationCalSidesInProgressChanged();
            _updateAndEmitShowOrientationCalArea(true);
        }
        return;
    }
    
    if (text.endsWith("orientation detected")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side started" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalDownSideRotate = true;
            }
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalUpsideDownSideRotate = true;
            }
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalLeftSideRotate = true;
            }
        } else if (side == "right") {
            _orientationCalRightSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalRightSideRotate = true;
            }
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalNoseDownSideRotate = true;
            }
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalTailDownSideRotate = true;
            }
        }
        
        if (_magCalInProgress) {
            _orientationCalAreaHelpText->setProperty("text", "Rotate the vehicle continuously as shown in the diagram until marked as Completed");
        } else {
            _orientationCalAreaHelpText->setProperty("text", "Hold still in the current orientation");
        }
        
        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.endsWith("side done, rotate to a different side")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = false;
            _orientationCalDownSideDone = true;
            _orientationCalDownSideRotate = false;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalUpsideDownSideDone = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = false;
            _orientationCalLeftSideDone = true;
            _orientationCalLeftSideRotate = false;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = false;
            _orientationCalRightSideDone = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalNoseDownSideDone = true;
            _orientationCalNoseDownSideRotate = false;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = false;
            _orientationCalTailDownSideDone = true;
        }
        
        _orientationCalAreaHelpText->setProperty("text", "Place you vehicle into one of the orientations shown below and hold it still");

        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.startsWith("calibration cancelled")) {
        _stopCalibration(_waitingForCancel ? StopCalibrationCancelled : StopCalibrationFailed);
        return;
    }

    */
}

void APMSensorsComponentController::_refreshParams(void)
{
    QStringList fastRefreshList;
    
    fastRefreshList << "COMPASS_OFS_X" << "COMPASS_OFS_X" << "COMPASS_OFS_X"
                    << "INS_ACCOFFS_X" << "INS_ACCOFFS_Y" << "INS_ACCOFFS_Z";
    foreach (QString paramName, fastRefreshList) {
        _autopilot->refreshParameter(FactSystem::defaultComponentId, paramName);
    }
    
    // Now ask for all to refresh
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, "COMPASS_");
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, "INS_");
}

bool APMSensorsComponentController::fixedWing(void)
{
    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
            return true;
        default:
            return false;
    }
}

void APMSensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void APMSensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void APMSensorsComponentController::cancelCalibration(void)
{
    // The firmware doesn't allow us to cancel calibration. The best we can do is wait
    // for it to timeout.
    _waitingForCancel = true;
    emit waitingForCancelChanged();
    _cancelButton->setEnabled(false);
    _uas->stopCalibration();
}

void APMSensorsComponentController::nextClicked(void)
{
    mavlink_message_t       msg;
    mavlink_command_ack_t   ack;

    ack.command = 0;
    ack.result = 1;
    mavlink_msg_command_ack_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &msg, &ack);

    _vehicle->sendMessage(msg);
}
