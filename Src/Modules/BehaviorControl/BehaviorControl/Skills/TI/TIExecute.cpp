/**
 * @file TIExecute.cpp
 * @author Andy Hobelsberger
 * @brief This Skill executes  
 * @version 1.0
 * @date 2022-01-01
 * 
 * This Skill Executes Supported Subskills for the TI System.
 */

#include "Representations/BehaviorControl/Skills.h"
#include "Representations/BehaviorControl/TI/TIData.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Tools/Streams/TypeRegistry.h"
#include <functional>

struct SkillMapping {
  bool mapped = false;
  std::function<void(const playbackAction& action)> call;
  std::function<bool()> isDone;
  std::function<bool()> isAborted;
};

/**
 * @brief Default Mapping macro for Enum -> skill calls. This must always be called first
 * Maps a skill call with args and maps through isDone and isAborted. Customize isDone and isAborted with the MAP_DONE and MAP_ABORT macros
 */
#define MAP(_ENUM, _SKILL, _ARGS) MAP_EXPLICIT(_ENUM, _SKILL, {_SKILL _ARGS;})

#define MAP_EXPLICIT(_ENUM, _SKILL, _CALL) \
  mappings[_ENUM].call = [&](const playbackAction& action)->void _CALL; \
  mappings[_ENUM].isDone = [&]()->bool {return _SKILL.isDone();}; \
  mappings[_ENUM].isAborted = [&]()->bool {return _SKILL.isAborted();}; \
  mappings[_ENUM].mapped = true

#define MAP_DONE(_ENUM, _CALL) \
  mappings[_ENUM].isDone = [&]()->bool _CALL

#define MAP_ABORT(_ENUM, _CALL) \
  mappings[_ENUM].isAborted = [&]()->bool _CALL

SKILL_IMPLEMENTATION(TIExecuteImpl,
{,
  IMPLEMENTS(TIExecute),
  REQUIRES(FrameInfo),
  CALLS(Stand),
  CALLS(WalkAtRelativeSpeed),
});

class TIExecuteImpl : public TIExecuteImplBase
{
  public:
	std::vector<SkillMapping> mappings;

  TIExecuteImpl() {
		mappings.resize(playbackAction::numOfSkillss);
    
    // Mappings for Skills defined in TIData.h
    MAP_EXPLICIT(playbackAction::Skills::Default, theStandSkill, {theStandSkill();});

    MAP_EXPLICIT(playbackAction::Skills::Stand, theStandSkill, {theStandSkill();});

    MAP(playbackAction::Skills::WalkAtRelativeSpeed, theWalkAtRelativeSpeedSkill, (action.poseParam));
    // MAP(playbackAction::Skills::KickAtGoal, theWalkToBallAndKickAtGoalSkill, ());
    // MAP(playbackAction::Skills::WalkToBall, theWalkToBallSkill, ());

    // MAP(playbackAction::Skills::WalkToTarget, theWalkToTargetSkill, (Pose2f(180_deg, 1000.0f, 1000.0f), action.poseParam));
    // MAP_DONE(playbackAction::Skills::WalkToTarget, {return false;});

    for(size_t i = 0; i < mappings.size(); i++) {
      //ASSERT(mappings[i].mapped); // If this Trips, A function of the Enum playbackAction::Skills has no mapping. Add it above
      
      if (!mappings[i].mapped) {
        mappings[i] = mappings[playbackAction::Default];
        OUTPUT_TEXT("Warning: TI Skill `" << std::string(TypeRegistry::getEnumName(typeid(playbackAction::Skills).name(), int(i))) << "` is not mapped. It has been remapped to Default");
      }
    }  
  }

  void execute(const TIExecute& p) override{
    mappings[p.action.skill].call(p.action);
  }

  bool isDone(const TIExecute& p) const override{
    return mappings[p.action.skill].isDone();
  }

  bool isAborted(const TIExecute& p) const override{
    return mappings[p.action.skill].isAborted();
  }
};

MAKE_SKILL_IMPLEMENTATION(TIExecuteImpl);
