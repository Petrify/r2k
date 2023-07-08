/**
 * @file PlayerOneForwardPassCard.cpp
 * @author Asrar
 * @version 1.1
 * @date 21-06-2023
 *
 *
 * Functions, values, side effects:
 *Passing from player one to player two
 *
 * Details:
 * Purpose of this card is to walk to the ball and kick it to the front teammate.
 * v.1.1: increased the shoot strength from KickInfo::walkForwardsLeft to   KickInfo::walkForwardsLeftLong);
 
 */

// B-Human includes
#include "Representations/BehaviorControl/FieldBall.h"
#include "Representations/BehaviorControl/Skills.h"
#include "Representations/Configuration/FieldDimensions.h"
#include "Representations/Modeling/RobotPose.h"
#include "Tools/BehaviorControl/Framework/Card/Card.h"
#include "Tools/BehaviorControl/Framework/Card/CabslCard.h"
#include "Tools/Math/BHMath.h"
#include "Representations/Communication/TeamData.h"

// this is the R2K specific stuff
#include "Representations/BehaviorControl/TeamBehaviorStatus.h"
#include "Representations/BehaviorControl/TeammateRoles.h"
#include "Representations/BehaviorControl/PlayerRole.h"
#include "Representations/Communication/RobotInfo.h"
#include "Representations/Infrastructure/ExtendedGameInfo.h"

CARD(PlayerOneForwardPassCard,
     {
    ,
    CALLS(Activity),
    CALLS(GoToBallAndKick),
    CALLS(LookForward),
    CALLS(Stand),
    CALLS(WalkAtRelativeSpeed),
    REQUIRES(FieldBall),
    REQUIRES(FieldDimensions),
    REQUIRES(RobotPose),
    REQUIRES(TeamData),
    REQUIRES(TeamBehaviorStatus),   // R2K
    REQUIRES(TeammateRoles),        // R2K
    REQUIRES(PlayerRole),           // R2K
    REQUIRES(RobotInfo),            // R2K
    REQUIRES(ExtendedGameInfo),
    DEFINES_PARAMETERS(
                       {,
                           (float)(0.8f) walkSpeed,
                       }),
    
    /*
     //Optionally, Load Config params here. DEFINES and LOADS can not be used together
     LOADS_PARAMETERS(
     {,
     //Load Params here
     }),
     
     */
    
});

class PlayerOneForwardPassCard : public PlayerOneForwardPassCardBase
{
    
    bool preconditions() const override
    {
        
      return
        // theTeammateRoles.playsTheBall(&theRobotInfo, true) &&   // I am the striker
        theRobotInfo.number == 1
        && theFieldBall.positionOnField.x() < 0 
              // theTeammateRoles.isTacticalGoalKeeper(theRobotInfo.number);
        ;

        
    }
    
    bool postconditions() const override
    {
        return !preconditions();
    }
    
    void execute() override
    {
        
        theActivitySkill(BehaviorStatus::playerOneForwardPass);
        
        float x =  1000.f;
        float y = -1500.f;


/*
        for (const auto& buddy : theTeamData.teammates)
        {
            if (!buddy.isPenalized && buddy.isUpright)
            {
                
                if(buddy.number==2)
                {
                   // OUTPUT_TEXT("actual target " << x << "  " << y);
                    x = buddy.theRobotPose.translation.x() + x ;
                    y = buddy.theRobotPose.translation.y() + y;
                    break;
                }
            }
        }
*/
        // theGoToBallAndKickSkill(calcAngleToOffense(x,y), KickInfo::walkForwardsLeft);
        // walkForwardsLeftLong: ~250
        // @param targetDirection The (robot-relative) direction in which the ball should go
        // @param kickType The type of kick that should be used
        // @param alignPrecisely Whether the robot should align more precisely than usual
        // @param kickPower The amount of power (in [0, 1]) that the kick should use
    
    if(theExtendedGameInfo.timeSincePlayingStarted < 10000)
        theGoToBallAndKickSkill(calcAngleToOffense(x, y), KickInfo::forwardFastLeft);
        else
        theGoToBallAndKickSkill(calcAngleToOffense(x, y), KickInfo::walkForwardsLeft);
        
    }
    
    Angle calcAngleToOffense(float xPos, float yPos) const
    {
        return (theRobotPose.inversePose * Vector2f(xPos, yPos)).angle();
    }
};

MAKE_CARD(PlayerOneForwardPassCard);
