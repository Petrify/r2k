/**
 * @file TestCard.cpp
 * @author Andy Hobelsberger    
 * @brief This card's preconditions are always true. 
 *        Edit it for testing
 * @version 0.1
 * @date 2021-05-23
 * 
 * 
 */

// Skills - Must be included BEFORE Card Base
#include "Representations/BehaviorControl/Skills.h"

// Card Base
#include "Tools/BehaviorControl/Framework/Card/Card.h"
#include "Tools/BehaviorControl/Framework/Card/CabslCard.h"
#include "Modules\Infrastructure\WhistleHandler\WhistleHandler.h"

// Representations
#include "Representations/BehaviorControl/FieldBall.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/Communication/RobotInfo.h"
#include "Representations/Communication/GameInfo.h"
#include "Representations/BehaviorControl/TeamBehaviorStatus.h"
#include "Representations/Modeling/RobotPose.h"
#include "Platform/SystemCall.h"

//#include <filesystem>

// Modify this card but don't commit changes to keep it clean for other developers
// Also don't forget to put this card at the top of your Card Stack!
CARD(TestCard,
     {
        ,
        CALLS(Activity),
        CALLS(LookForward),
        CALLS(Stand),
        REQUIRES(FrameInfo),
        REQUIRES(Whistle),

        DEFINES_PARAMETERS(
             {,
                //Define Params here
          
             }),

        /*
        //Optionally, Load Config params here. DEFINES and LOADS can not be used together
        LOADS_PARAMETERS(
             {,
                //Load Params here
             }),
             
        */

     });

class TestCard : public TestCardBase
{

  //always active
  bool preconditions() const override
  {
    return true;
  }

  bool postconditions() const override
  {
    return true;   // set to true, when used as default card, ie, lowest card on stack
  }

  void execute() override
  {

    theActivitySkill(BehaviorStatus::testingBehavior);
    // std::string s = "testingBehavior";
    // OUTPUT_TEXT(s);

    // Override these skills with the skills you wish to test
    theLookForwardSkill(); // Head Motion Request
    theStandSkill(); // Standard Motion Request

    SystemCall::playSound("Whistle.wav");

      const int whistleTimeout = 5000;         // Timeout 
      const float confidenceThreshold = 0.5f;  // Schwellwert 

      // Überprüfung der Pfeifenerkennung
      if (theFrameInfo.getTimeSince(theWhistle.lastTimeWhistleDetected) < whistleTimeout &&
        theWhistle.confidenceOfLastWhistleDetection >= confidenceThreshold)
      {
        OUTPUT_TEXT("I heard a whistle " << theWhistle.confidenceOfLastWhistleDetection);
        SystemCall::say("I heard a whistle");
      }
      else
      {
        OUTPUT_TEXT("No whistle detected " << theWhistle.lastTimeWhistleDetected);
        SystemCall::say("No whistle detected");
      }
    
  }
};

MAKE_CARD(TestCard);
