/**
 * @file PlayerRole.h
 *
 * This file declares a representation of a player's role.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Tools/Streams/AutoStreamable.h"
#include "Tools/Streams/Enum.h"
#include <string>

STREAMABLE(PlayerRole,
  {
    ENUM(RoleType,
    {,
      none,
      goalkeeper,
      ballPlayer,
      goalkeeperAndBallPlayer,

      // legacy roles from 2019
      firstSupporterRole,
      supporter0 = firstSupporterRole,
      supporter1,
      supporter2,
      supporter3,
      supporter4,
    });

  /**
   * Compatibility function for 2019 behavior.
   * @return Whether the robot is goalkeeper.
   */
  bool isGoalkeeper() const
  {
    return (goalkeeper == role) || (goalkeeperAndBallPlayer == role);
  }

  /**
   * Compatibility function for 2019 behavior.
   * @return Whether the robot plays the ball.
   * 
   * 
   * Adrian, R2K: re-activated for our code base
   * Since v1.3 we store the number of the bot in "captain"
   */
  bool playsTheBall() const
  {
    return (ballPlayer == role) || (goalkeeperAndBallPlayer == role);
  }

  /**
   * Compatibility function for 2019 behavior.
   * @return The robot's supporter index.
   */
  int supporterIndex() const
  {
    return role < firstSupporterRole ? -1 : (role - firstSupporterRole);
  },

  (RoleType)(none) role, /**< The role type. */
  (int)(0) numOfActiveSupporters, /**< The number of not penalized supporters (i.e. robots that have a supporterIndex >= 0). */
});
