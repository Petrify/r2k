/**
 * @file Controller/GameController.h
 * This file declares a class that simulates a console-based GameController.
 * @author <a href="mailto:Thomas.Roefer@dfki.de">Thomas Röfer</a>
 */

#pragma once

#include <set>
#include <SimRobotCore2.h>
#include "Platform/Thread.h"
#include "Representations/Configuration/FieldDimensions.h"
#include "Representations/Infrastructure/GameInfo.h"
#include "Representations/Infrastructure/RobotInfo.h"
#include "Representations/Infrastructure/TeamInfo.h"
#include "Tools/Enum.h"
#include "Tools/Math/Pose2D.h"
#include "Tools/Streams/InOut.h"

class SimulatedRobot;

/**
 * The class simulates a console-based GameController.
 */
class GameController
{
private:
  class Robot
  {
  public:
    SimulatedRobot* simulatedRobot;
    RobotInfo info;
    unsigned timeWhenPenalized;
    Pose2D lastPose;
    bool manuallyPlaced;

    Robot() : simulatedRobot(0), timeWhenPenalized(0), manuallyPlaced(false) {}
  };

  ENUM(Penalty,
    none,
    ballHolding,
    playerPushing,
    obstruction, // deprecated
    inactivePlayer,
    illegalDefender,
    leavingTheField,
    playingWithHands,
    requestForPickup,
    manual
  );
  static const int numOfPenalties = numOfPenaltys; /**< Correct typo. */

  DECLARE_SYNC;
  static const int numOfRobots = 14;
  static const int numOfFieldPlayers = numOfRobots / 2 - 3; //Coach, Keeper, Substitute
  static const int durationOfHalf = 600;
  static const float footLength; /**< foot length for position check and manual placement at center circle. */
  static const float safeDistance; /**< safe distance from penalty areas for manual placement. */
  static const float dropHeight; /**< height at which robots are manually placed so the fall a little bit and recognize it. */
  static Pose2D lastBallContactPose; /**< Position were the last ball contact of a robot took place, orientation is toward opponent goal (0/180 degress). */
  static FieldDimensions fieldDimensions;
  GameInfo gameInfo;
  TeamInfo teamInfos[2];
  unsigned timeWhenHalfStarted;
  unsigned timeOfLastDropIn;
  unsigned timeWhenLastRobotMoved;
  unsigned timeWhenStateBegan;
  Robot robots[numOfRobots];

   /** enum which declares the different types of balls leaving the field */
  enum BallOut { NONE, GOAL_BY_RED, GOAL_BY_BLUE, OUT_BY_RED, OUT_BY_BLUE };

  /**
   * Handles the command "gc".
   * @param command The second part of the command (without "gc").
   */
  bool handleGlobalCommand(const std::string& command);

  /**
   * Handles the command "pr".
   * @param robot The number of the robot that received the command.
   * @param command The second part of the command (without "pr").
   */
  bool handleRobotCommand(int robot, const std::string& command);

  /**
   * Is a robot it its own penalty area or in its own goal area?
   * @param robot The number of the robot to check [0 ... numOfRobots-1].
   * @return Is it?
   */
  bool inOwnPenaltyArea(int robot) const;

  /**
   * Finds a free place for a (un)penalized robot.
   * @param robot The number of the robot to place [0 ... numOfRobots-1].
   * @param x The optimal x coordinate. Might be moved toward own goal.
   * @param y The y coordinate.
   * @param rotation The rotation when placed.
   */
  void placeForPenalty(int robot, float x, float y, float rotation);

  /**
   * Manually place a goalie if required.
   * @param robot The robot number of the goalie to place [0 ... numOfRobots-1].
   */
  void placeGoalie(int robot);

  /**
   * Move a field player to a new pose from a set of possible poses.
   * Pick the pose the teammates would not pick.
   * @param robot The number of the robot to place [0 ... numOfRobots-1].
   * @param minRobot The number of the first field player in the team [0 ... numOfRobots-1].
   * @param poses Possible placepent poses for the robot.
   * @param numOfPoses The number of possible poses.
   */
  void placeFromSet(int robot, int minRobot, const Pose2D* poses, int numOfPoses);

  /**
   * Manually place the field players of the offensive team if required.
   * @param minRobot The number of the first robot to place [0 ... numOfRobots-1].
   */
  void placeOffensivePlayers(int minRobot);

  /**
   * Manually place the field players of the defensive team if required.
   * @param minRobot The number of the first robot to place [0 ... numOfRobots-1].
   */
  void placeDefensivePlayers(int minRobot);

  /** Execute the manual placements decided before. */
  void executePlacement();

  /** Update the ball position based on the rules. */
  static BallOut updateBall();

public:
  bool automatic; /**< Are the automatic features active? */

  /** Constructor */
  GameController();

  /**
   * Each simulated robot must be registered.
   * @param robot The number of the robot [0 ... numOfRobots-1].
   * @param simulatedRobot The simulation interface of that robot.
   */
  void registerSimulatedRobot(int robot, SimulatedRobot& simulatedRobot);

  /**
   * Handles the parameters of the console command "gc".
   * @param stream The stream that provides the parameters.
   * @return Were the parameters correct?
   */
  bool handleGlobalConsole(In& stream);

  /**
   * Handles the parameters of the console command "pr".
   * @param robot The number of the robot that received the command.
   * @param stream The stream that provides the parameters.
   * @return Were the parameters correct?
   */
  bool handleRobotConsole(int robot, In& stream);

  /** Executes the automatic referee. */
  void referee();

  /**
  * Proclaims which robot touched the ball at last
  * @param robot The robot
  */
  static void setLastBallContactRobot(SimRobot::Object* robot);

  /**
   * Write the current game information to the stream provided.
   * @param stream The stream the game information is written to.
   */
  void writeGameInfo(Out& stream);

  /**
   * Write the current information of the team to the stream
   * provided.
   * @param robot A robot from the team.
   * @param stream The stream the team information is written to.
   */
  void writeOwnTeamInfo(int robot, Out& stream);

  /**
   * Write the current information of the opponent team to the
   * stream provided.
   * @param robot A robot from the team.
   * @param stream The stream the team information is written to.
   */
  void writeOpponentTeamInfo(int robot, Out& stream);

  /**
   * Write the current information of a certain robot to the
   * stream provided.
   * @param robot The robot the information is about.
   * @param stream The stream the robot information is written to.
   */
  void writeRobotInfo(int robot, Out& stream);

  /**
   * Adds all commands of this module to the set of tab completion
   * strings.
   * @param completion The set of tab completion strings.
   */
  void addCompletion(std::set<std::string>& completion) const;
};