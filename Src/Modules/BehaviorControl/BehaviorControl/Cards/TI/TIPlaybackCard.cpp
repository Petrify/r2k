/**
 * @file TIPlaybackCard.cpp
 * @author Jonas Lambing
 * @brief This card executes skills from a playback sequence
 * 
 * This card executes skills with parameters specified by a playback sequence.
 * The execution happens on a per-action basis.
 * That means, that the next action will trigger as soon as the current action
 * is either finished (isDone()) or exceeded the maximum time specified in the playback file.
 * 
 * MaxTime is calculated during the teach in process.
 * 
 * This card is a collection for all available teach in cards.
 *
 * @version 1.0
 * @date 2021-08-26
 *
 * Changes for 
 * @version 2.0
 * @date 2022-01-24
 * 
 * @author Adrian Müller
 * Major revision: Works now together with the enhanced TIPlaybackProvider and TIExecute:
 * a) pre-cond checks wether at least on card playback000x.csv is qualified (the sequence in it triggers)
 * b) indexOfBestTeachInScore() loops over all cards, ie. all available playback sequences (ie., playback0001 ... playback000MAX), MAX >=1.
 *    Selects sequence with best trigger point, registered in parameter card_index 
 * c) set_next_action() loops over actions 0 .. size-1, stores it in actionIndex
 *    So we have: currentAction = theTIPlayback.data[cardIndex].actions[actionIndex];
 * d) generic call for each action with theTIExecuteSkill(currentAction);
 *	  Next action: either time limit is exceeded OR skill.isDone()
 * e) post-condition: -1 == actionIndex : set_next_action() reached end of actions
 *
 * Notes: 
 *    Register all skills in playback000x.scv in Execute.cpp  MAP_EXPLICIT, MAP, MAP_ISDONE, and add CALLS() here
 *    b) is a control mechanism for game tactics
 *    d) time limit might come in to early
 *    d) skill.isDone() does not work properly on all skills, eg theWalkToTargetSkill() is buggy with this respect
 * 
 * 
 * @version 2.1
 * @date 2022-01-24
 * @author Adrian Müller
 * - change semantics of trigger points: any robot close to the point of recording will trigger
 * 
 * ToDo:
 *	include generic map for head movements
 */

#include "Representations/BehaviorControl/Skills.h"
#include "Representations/BehaviorControl/TI/TIPlaybackData.h"
#include "Representations/Configuration/GlobalOptions.h" 
#include "Representations/Communication/RobotInfo.h"
#include "Representations/Modeling/RobotPose.h"
#include "Tools/BehaviorControl/Framework/Card/Card.h"
#include "Tools/BehaviorControl/Framework/Card/CabslCard.h"
#include "Tools/Math/Geometry.h"

CARD(TIPlaybackCard,
{,
  CALLS(Activity),
  CALLS(LookForward),
  CALLS(Stand),
  CALLS(TIExecute),

	REQUIRES(GlobalOptions),
	REQUIRES(RobotInfo),
	REQUIRES(RobotPose),
  REQUIRES(TIPlaybackSequences),

    DEFINES_PARAMETERS(
    {,
					 (bool)(false) onceP,
           (int)(0)startTime,              // Time when the action was started
					 (int)(-1)cardIndex,             // Index of the card inside the stack of worldmodels (and playbacks resp.)
					 (int)(-1)actionIndex,            // Index of the action inside the card, ie., which playback action is active
					 (PlaybackAction)currentAction,  // A copy of the action-data
					 (bool)(false)action_changed,    // used as flag: action is called the first time in execute(); reset after First usage, set again in setNextAction()
																					 // so we can set information like the robots starting position, only once, before action starts
					 // (Pose2f)(0) destPos,	// not yet
    }),
});


// TODO: mapping of isDone()
class TIPlaybackCard : public TIPlaybackCardBase
{
	bool preconditions() const override
	{


		// Dont execute if the card stack is empty
		return (-1 == actionIndex && !theTIPlaybackSequences.models.empty() && thisIsATriggerPoint(theRobotInfo.number));
			// return (teachInScoreReached(theRobotInfo.number));
	}

	bool postconditions() const override
	{
		// Exit the card if no more playback actions have to be done
		return !preconditions();
	}

	void execute() override
	{

		theActivitySkill(BehaviorStatus::testingBehavior);
		// OUTPUT_TEXT("ti started");

		// called only one
		if(!startTime) cardIndex = thisIsATriggerPoint(theRobotInfo.number); // selects best sequence in playback0001.csv, plaback0002.csv,...
		ASSERT(-1 != cardIndex); // at least one model must qualify, since teachInScoreReached() is called in pre-cond

		// Figure out which action to play; sets startTime 
		setNextAction();

		// Playback reached the end (OR no model found, which should not happen) -> stand still
		if(actionIndex == -1)
		{
			theLookForwardSkill();
			theStandSkill();
			return;
		}

		// TODO: Better Conditions for action Execution
		theLookForwardSkill();  // ToDo: this is just a generic action to prevent MEEKs
		theTIExecuteSkill(currentAction);
	}

	void setNextAction()
	{

		if(!startTime)
		// set for first action now
		{
			startTime      = state_time;
			action_changed = true;
			actionIndex    = 0;
		}
		// Replay is finished, nothing more to do.
		if(-1 == actionIndex) return;


		
		// Switch to next action if maxTime was exceeded OR skillIsDone
		// OUTPUT_TEXT("start" << startTime << " state " << state_time);

		if(((state_time - startTime) > currentAction.maxTime) ) // || theTIExecuteSkill.isDone())
		{
			actionIndex++;
			action_changed = true; // flag for one-time setups below
			// OUTPUT_TEXT("actionIndex" << actionIndex);
		}

		// check: this next action is out of bounds -> we reached the end
		if(static_cast<size_t>(actionIndex) >= theTIPlaybackSequences.data[cardIndex].actions.size())
		{
			OUTPUT_TEXT("Reached end of playback sequence");
			currentAction = {};
			actionIndex   = -1;  // set post condition
			return;
		}

		// ok: we are inbetween o .. #actions-1
		currentAction = theTIPlaybackSequences.data[cardIndex].actions[actionIndex];
		if(action_changed)  // do setups for the new action
		{
			action_changed = false;
			startTime      = state_time;
			// OUTPUT_TEXT("Action: " + std::to_string(actionIndex));
			// OUTPUT_TEXT("playback000" << cardIndex + 1 << " action Index and Name: " << actionIndex << " " << TypeRegistry::getEnumName(theTIPlaybackSequences.data[cardIndex].actions[actionIndex].skill));
			// OUTPUT_TEXT("remaining time" << diff);


			// obsolete code
			// prepare check for isDone(): set exit criterion
			// destPos = theRobotPose * theTIPlaybackSequences.data[cardIndex].actions[actionIndex].poseParam;
		}
	}

  bool thisIsATriggerPoint(int Number, bool findBestScore = false) const

  {
    ASSERT(!theTIPlaybackSequences.models.empty()); // has been checked in the pre-condition

    // DECLARE_DEBUG_DRAWING("representation:FieldBall:relative", "drawingOnField");
    DECLARE_DEBUG_DRAWING("representation:TeamBallModel", "drawingOnField");

    float minimal_distance = 500.0f;
    int world_model_index = -1;
    int current_bestWorldModelIndex = -1;

    for (WorldData data : theTIPlaybackSequences.models)
    {
      WorldModel& model = data.trigger;
      world_model_index++;

      /*
      if (theRobotInfo.number == 4 && !onceP) {
        // onceP = true;
        CIRCLE("representation:TeamBallModel", model.robotPose.translation.x(), model.robotPose.translation.y(), 60, 20, Drawings::solidPen, ColorRGBA::white, Drawings::noBrush, ColorRGBA(255, 0, 255));
        // OUTPUT_TEXT("Trigger Point " << world_model_index << " " << model.robotPose.translation.x() << " " << model.robotPose.translation.y());
      }
      */
      if (!findBestScore || (Geometry::distance(theRobotPose.translation, model.robotPose.translation) <= minimal_distance))
      {
        if (!findBestScore)
        {
          if (std::abs(Geometry::distance(theRobotPose.translation, model.robotPose.translation)) <= 500) {
            OUTPUT_TEXT("Trigger Point " << world_model_index << "for robot" << theRobotInfo.number);
            return true;
          }
        }
        else
        {
          current_bestWorldModelIndex = world_model_index;
          minimal_distance = Geometry::distance(theRobotPose.translation, model.robotPose.translation);
        }
      }
    }

    if (findBestScore && current_bestWorldModelIndex != -1)
    {
      ASSERT(current_bestWorldModelIndex >= 0);
      OUTPUT_TEXT("trigger became active for robot " << Number << " from file " << theTIPlaybackSequences.models[current_bestWorldModelIndex].fileName);
    }

    return findBestScore ? current_bestWorldModelIndex != -1 : false;
  }

};


MAKE_CARD(TIPlaybackCard);