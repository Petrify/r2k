/**
* @file Representations/MotionControl/MotionRequest.h
* This file declares a class that represents the motions that can be requested from the robot.
* @author <A href="mailto:Thomas.Roefer@dfki.de">Thomas Röfer</A>
*/

#pragma once

#include "SpecialActionRequest.h"
#include "WalkRequest.h"
#include "KickRequest.h"

/**
* @class MotionRequest
* A class that represents the motions that can be requested from the robot.
*/
STREAMABLE(MotionRequest,
{
public:
  ENUM(Motion,
    walk,
    kick,
    specialAction,
    stand,
    getUp
  );

  /**
   * Prints the motion request to a readable string. (E.g. "walk: 100mm/s 0mm/s 0°/s")
   * @param destination The string to fill
   */
  void printOut(char* destination) const;

  /** Draws something*/
  void draw() const,

  (Motion)(specialAction) motion, /**< The selected motion. */
  (SpecialActionRequest) specialActionRequest, /**< The special action request, if it is the selected motion. */
  (WalkRequest) walkRequest, /**< The walk request, if it is the selected motion. */
  (KickRequest) kickRequest, /**< The kick request, if it is the selected motion. */
});