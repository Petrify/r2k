#include "ManualHeadMotionProvider.h"
#include "Tools/Math/Transformation.h"
#include "Tools/Debugging/Debugging.h"

MAKE_MODULE(ManualHeadMotionProvider, Behavior Control)

ManualHeadMotionProvider::ManualHeadMotionProvider() : currentX(0), currentY(0)
{}

void ManualHeadMotionProvider::update(HeadMotionRequest& headMotionRequest)
{
  bool parametersChanged = xImg != currentX || yImg != currentY;
  if(parametersChanged && camera == theCameraInfo.camera)
  {
    currentX = xImg;
    currentY = yImg;

    Vector2<float> targetOnField;
    if(Transformation::imageToRobot(currentX, currentY, theCameraMatrix, theCameraInfo, targetOnField))
    {
      headMotionRequest.target.x = targetOnField.x;
      headMotionRequest.target.y = targetOnField.y;
      headMotionRequest.target.z = 0;
      headMotionRequest.mode = HeadMotionRequest::targetOnGroundMode;
      headMotionRequest.watchField = false;

      //Use the camera that the user is seeing right now.
      switch(camera)
      {
        case CameraInfo::lower:
          headMotionRequest.cameraControlMode = HeadMotionRequest::lowerCamera;
          break;
        case CameraInfo::upper:
          headMotionRequest.cameraControlMode = HeadMotionRequest::upperCamera;
          break;
        default:
          ASSERT(false);
      }

      headMotionRequest.speed = fromDegrees(150);
    }
  }
}