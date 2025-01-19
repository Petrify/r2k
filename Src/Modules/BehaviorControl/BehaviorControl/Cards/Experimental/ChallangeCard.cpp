/**
 * @file ChallangeCard.cpp
 * @author Dennis Fuhrmann
 * @brief Card for R2k Ball Challange 2025
 *        In this Card the Ball is not aktually kicked,
 *        instead a walk into the Ball is performed to "fake" a kick forward.
 *        TODO Live Test with Real robots
 * @version 1.0
 * @date 2025-19-01
 *
 *
 */

 // Skills - Must be included BEFORE Card Base
#include "Representations/BehaviorControl/Skills.h"

// Card Base
#include "Tools/BehaviorControl/Framework/Card/Card.h"
#include "Tools/BehaviorControl/Framework/Card/CabslCard.h"


// Representations
#include "Representations/Configuration/FieldDimensions.h"
#include "Representations/BehaviorControl/FieldBall.h"
#include "Representations/Modeling/ObstacleModel.h"
#include "Representations/Communication/TeamData.h"
#include "Representations/Modeling/BallModel.h"
#include "Representations/Configuration/BallSpecification.h"


//#include <filesystem>
#include "Tools/Modeling/BallPhysics.h"
#include "Tools/Math/Eigen.h"
#include "Tools/BehaviorControl/Interception.h"



CARD(ChallangeCard,
  {
       ,
       CALLS(Activity),
       CALLS(WalkToPoint),
       CALLS(LookAtBall),
       CALLS(WalkToKickoffPose),
       CALLS(TurnAngle),
       REQUIRES(FieldBall),
       REQUIRES(RobotPose),
       REQUIRES(FieldDimensions),
       REQUIRES(BallModel),
       REQUIRES(BallSpecification),


       DEFINES_PARAMETERS(
      {,
         (Vector2f)(Vector2f(200.f,0.f)) interceptPoint,
        (bool)(false) pointIsSelected, // InterceptPoint wird nur einmal berechnet
      }),
    });

class ChallangeCard : public ChallangeCardBase
{

    //always active
    bool preconditions() const override
    {
        return theFieldBall.timeSinceBallWasSeen < 7000;
    }

    bool postconditions() const override
    {
      return  !(theFieldBall.timeSinceBallWasSeen < 7000) && false;

    }

    void execute() override
    {
      theActivitySkill(BehaviorStatus::testingBehavior);

      Vector2f intersectionwithownYAxis = theFieldBall.intersectionPositionWithOwnYAxis;
      Vector2f inersectionwithOwnXAxis = Vector2f::Zero();

      //Calculate Distance to Ball for Kick depedndant on current Position of Ball and ints Speed
      float minDistance = calcMinDistance();
      
        if (calcDisrtacetoBall() <= minDistance) {
             
            //// InterceptPoint wird nur einmal berechnet
          if (!pointIsSelected) {
            interceptPoint = calcInterceptPoint();
            pointIsSelected = true;
          }
              //Die Kick Skills sind nicht schnell genug um einen rollenden Ball zu intercepten stadesssen nutzen wir einen Lauf in den Ball rein um einen Kick zu simulieren
             theWalkToPointSkill(Pose2f(0_deg, interceptPoint), 1.f, true, true, true);
             theLookAtBallSkill(); // HeadMotion controll

        }else
        {
          theTurnAngleSkill(calcAngleToGoal() + 30_deg, 2_deg);
          theLookAtBallSkill();
        }
      
      
    }

    Angle calcAngleToGoal() const
    {
      return (theRobotPose.inversePose * Vector2f(theFieldDimensions.xPosOpponentGroundLine, 0.f)).angle();
    }

    //Altenativ TimetoReachBall benutzen (konnte nicht herausfinden wie)  
    float calcDisrtacetoBall() const
    {
      Vector2f temp1 = theFieldBall.recentBallPositionRelative();
      float temp2 = temp1.x() * temp1.x();
      float temp3 = temp1.y() * temp1.y();
      return std::sqrt(temp2 + temp3);
    }

    //relative InterceptPoint 
    Vector2f calcInterceptPoint() const
    {
      return BallPhysics::propagateBallPosition(theFieldBall.recentBallPositionOnField(), theBallModel.estimate.velocity, 0.6f, theBallSpecification.friction);
    }


    //berechnet den Schwellwert für die Distance zum Ball anhand der Geschwnidkeit des Balls
    float calcMinDistance() const
    {
      Vector2f temp1 = theBallModel.estimate.velocity;
      float temp2 = temp1.x() * temp1.x();
      float temp3 = temp1.y() * temp1.y();
      float result = std::sqrt(temp2 + temp3);
      return result * 0.8;
    }
};

MAKE_CARD(ChallangeCard);
