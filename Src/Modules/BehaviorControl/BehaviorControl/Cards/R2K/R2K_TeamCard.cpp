/**
 * @file R2K_TeamCard.cpp
 * @author Adrian Müller
 * @version: 1.1
 * 
 * 
 * v1.0
 * functions, values set:
 * - TeamBehaviorStatus::R2K_NORMAL_GAME;  default settings
 * - TeammateRoles: just the default values, ie., for 1..5: goalie, defense, defense, offense, offense
 * - playsTheBall() (ie who is the striker)
 * - isGoalkeeper() (wether he playsTheBall() or not)
 *   - theCaptain stores the robot number, who is currently acting as goalkeeper
 * 
 * Notes:
 * - timeToReachBall is computed per bot - the resulting time frame ist NOT synced - BUT:
 * - we compute a unique answer for playsTheBall() (ie "striker") by
 *    - looping over all buddies, computing distance bot - ball
 *    - set role = PlayerRole::ballPlayer (resp. PlayerRole::goalkeeperAndBallPlayer) for bot with min distance
 *   
 * - accuracy of playsTheBall() is dependend on EBC update frequency AND correctness of self localization of buddies
 * - see ReferenceCard.cpp for usage
 * 
 * 
 * v1.1
 * - TeamBehaviorStatus::R2K_SPARSE_MODE iff opp. team has >= 3 penalties OR (opp has >= 2 penalties AND own penalties >=1)
 * - if not SPARSE: TeamBehaviorStatus::R2K_NORMAL_GAME / OFFENSIVE / DEFENSIVE is set due to score difference (tie, back, lead)
 *  - ToDo:
 *    - add time limits to switch mode strategically when game is about to end (switch to DEFENSIVE)
 *    - take into account secsTillUnpenalized for SPARSE mode (ie, adapt strategie ahead)
 * 
 *  - computes role.supportIndex(), role.numOfActiveSupporters
 *   - Note: supportIndex() == 0 means left-most player; supportIndex() == numOfActiveSupporter you are right most player
 * 
 * - if goalie is !PLAYIiNG: dynamically assigns left-most bot as substitute, sync this temp. assignment with thePlayerRole.isGoalkeeper()
 *   - NOTE: "dynamically" means: this role assiginment may change at any time, when relative positions change 
 * 
 * v1.2
 * - more sophisticated computing of TeammateRoles (take into account, which bot is penaliezed)
 *  - static assignement STATE_INITIAL, 
 *  - else: dynamic (re-)assignment for 8 TeammateRoles  
 * - provide wrapper functions (see TeammateRoles)
 *
 * 
 * 
 * ToDo
 * - EBC: broadcast, when striker changes etc.
 *  - TeaMatesRole: PLAYING enter/leave OR own,opppenalized on/off  OR Tactic change  freeze
 * - read real field dimensions from config
 *  - static assignement for state FINISHED
 * -   10sec: delay before change: 10sec
 *
*/

#include "Representations/BehaviorControl/TeamSkills.h"
#include "Tools/BehaviorControl/Framework/Card/TeamCard.h"


// #include "Tools/Debugging/Annotation.h"


// ttrb, isPlayBall
#include "Tools/Math/Geometry.h"
#include "Representations/BehaviorControl/FieldBall.h"
#include "Representations/BehaviorControl/TimeToReachBall.h"
#include "Representations/BehaviorControl/TeamBehaviorStatus.h"  
#include "Representations/Communication/TeamData.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Tools/BehaviorControl/Framework/Card/TeamCard.h"
#include "Representations/Communication/GameInfo.h"  // to sync time inbetween bots

// roles 
#include "Representations/BehaviorControl/PlayerRole.h"
#include "Representations/Communication/RobotInfo.h"

#include "Representations/Modeling/RobotPose.h" // supporterindex
#include <algorithm>  // min()
#include "Representations/BehaviorControl/TeammateRoles.h"  // GOALKEEPER, DEFENSE,...
#include "Representations/Communication/TeamInfo.h"         // access scores, OwnTeamInfo, OppTeamInfo

TEAM_CARD(R2K_TeamCard,
  { ,
    // CALLS(AnnotationManager),
    CALLS(Role),
    CALLS(TeammateRoles),
    CALLS(TimeToReachBall),
    REQUIRES(FieldBall),  // ttrb
    REQUIRES(FrameInfo),  // ttrb
    REQUIRES(TeamData),   // ttrb
    REQUIRES(GameInfo),   // ttrb, check for state change
    REQUIRES(RobotInfo),  // roles
    REQUIRES(RobotPose),  // supporterindex
    // USES(TeamBehaviorStatus),   // to be tested
    CALLS(TeamActivity),
    REQUIRES(OwnTeamInfo),    // score, penalty
    REQUIRES(OpponentTeamInfo),  // score, penalty

    DEFINES_PARAMETERS(
                {
                  ,
                  // (unsigned)(std::numeric_limits<unsigned>::max())offsetToFrameTimeThisBot,
    }),

});

class R2K_TeamCard : public R2K_TeamCardBase
{
  bool preconditions() const override
  {
    return true;
  }

  bool postconditions() const override
  {
    return false;
  }

  void execute() override
  {


#define GN TeammateRoles::GOALKEEPER_NORMAL
#define GA TeammateRoles::GOALKEEPER_ACTIVE

#define DL TeammateRoles::DEFENSE_LEFT
#define DM TeammateRoles::DEFENSE_MIDDLE
#define DR TeammateRoles::DEFENSE_RIGHT

#define OL TeammateRoles::OFFENSE_LEFT
#define OM TeammateRoles::OFFENSE_MIDDLE
#define OR TeammateRoles::OFFENSE_RIGHT

#define UN TeammateRoles::UNDEFINED


    // this tactic table is used in step d4 below
    //         nr of active players x team tactic x 5 TeamMate roles[]
    int r2k_tactics[5][TeamBehaviorStatus::numOfTeamActivities][5] =
    {
      //      R2K_NORMAL_GAME, R2K_DEFENSIVE_GAME,R2K_OFFENSIVE_GAME, R2K_SPARSE_GAME
    
      // 1 player
        { {GN,UN,UN,UN,UN}, {GN,UN,UN,UN,UN}, {GN,UN,UN,UN,UN}, {OM,UN,UN,UN,UN} },
        // 2 player
            { {GN,DM,UN,UN,UN}, {GN,DM,UN,UN,UN}, {GN,DM,UN,UN,UN}, {GN,OM,UN,UN,UN} },
            // 3 player
                { {GN,DM,OM,UN,UN}, {GN,DL,DR,UN,UN}, {GA,DM,OM,UN,UN}, {GN,OL,OR,UN,UN} },
                // 4 player
                    { {GN,DL,DR,OM,UN}, {GN,DL,DM,DR,UN}, {GA,DM,OL,OR,UN}, {UN,UN,UN,UN,UN} },
                    // 5 player
                        { {GN,DL,DR,OL,OR}, {GN,DL,DM,DR,OM}, {GA,DM,OL,OM,OR}, {UN,UN,UN,UN,UN} }

    };

    // 2019 code
    // RoboCup::TeamInfo& team = gameCtrlData.teams[gameCtrlData.teams[0].teamNumber == Global::getSettings().teamNumber ? 0 : 1];
    
    // 2022 code
    int own_score = theOwnTeamInfo.score;
    int opp_score = theOpponentTeamInfo.score;
    // Note: for unknown reasons, the counter is too high by 1 when the two loops below are finished
    int own_penalties = -1;
    int opp_penalties = -1;
    // loop over players and sum up penalized states
  
    for (const auto& buddy : theOwnTeamInfo.players) {
      if (buddy.penalty != PENALTY_NONE) own_penalties++;
    }
    for (const auto& buddy : theOpponentTeamInfo.players) {
      if (buddy.penalty != PENALTY_NONE) opp_penalties++;
    }
      
    TeammateRoles teamMateRoles;



    // to do: who is active - loop supp. index, number active
    // what if substitute goalie?
    int tbs; // patch due to update errors
    if (opp_penalties > 2 || (own_penalties >= 1 && opp_penalties >= 2)) {
   
      theTeamActivitySkill(TeamBehaviorStatus::R2K_SPARSE_GAME);
      tbs = TeamBehaviorStatus::R2K_SPARSE_GAME;
    }
    else {
      if (own_score == opp_score) { //default
        theTeamActivitySkill(TeamBehaviorStatus::R2K_NORMAL_GAME); 
        tbs = TeamBehaviorStatus::R2K_NORMAL_GAME;
      }  
      if (own_score < opp_score) { 
        theTeamActivitySkill(TeamBehaviorStatus::R2K_OFFENSIVE_GAME); 
        tbs = TeamBehaviorStatus::R2K_OFFENSIVE_GAME;
      }

      // to do: add time limit, so we will not spoil our leadership in the last n minutes
      if (own_score > opp_score) { 
        theTeamActivitySkill(TeamBehaviorStatus::R2K_DEFENSIVE_GAME); 
        tbs = TeamBehaviorStatus::R2K_DEFENSIVE_GAME;
      }
    }
 
      
    /* information flow for role assignments:
    a) count #active players
    b) is our goali active? (ie not penalized)
    c) make a sorted, lean copy of relevant data (helper class BotOnField)
    d1) PlayerRole:: computing the supporterindex for each bot from left to right
    d2) TeammateRoles:: static assignment for STATE_INITIAL else 
    d3) TeammateRoles: dynamic assignment
    d4) Use tactic table for assignments of active bots
      
    e) find min distance to ball for all bots
    f) who plays the ball?
    g) bot#1 is  penalized?


    */ 
    
    // a) count #active players
    unsigned int activeBuddies = 0;
    std::vector<BotOnField> botsLineUp;
    bool goalieIsActive = false;

    for (const auto& buddy : theTeamData.teammates)
    {
      if (!buddy.isPenalized && buddy.isUpright)
      {
        activeBuddies++;
        botsLineUp.push_back(BotOnField(buddy.number, buddy.theRobotPose.translation.x()));
      }
      // b) is our goalie active ? (ie not penalized)
      if (1 == buddy.number && !buddy.isPenalized) 
        goalieIsActive = true;  // This flag will be used below
    }

    // special case: I am the active goalie
    if (theRobotInfo.number == 1 && theRobotInfo.penalty == PENALTY_NONE) goalieIsActive = true;
 
  
    // c) make a sorted, lean copy of relevant data (helper class BotOnField)
    std::sort(botsLineUp.begin(), botsLineUp.end());
  
    PlayerRole pRole;
    if (1 == theRobotInfo.number) pRole.role = PlayerRole::goalkeeper;

    // default assignments; will be overwritten
    /*
    switch (theRobotInfo.number) {
    case 1:
      pRole.role = PlayerRole::goalkeeper;
      break;
      // 2-5: unused yet
    case 2:
      pRole.role = PlayerRole::supporter1;
      break;
    case 3:
      pRole.role = PlayerRole::supporter2;
      break;
    case 4:
      pRole.role = PlayerRole::supporter3;
      break;
    case 5:
      pRole.role = PlayerRole::supporter4;
      break;
    default:
      pRole.role = PlayerRole::none;
    }
    */
    pRole.numOfActiveSupporters = activeBuddies;
  
   
    // d1) PlayerRole:: computing the supporterindex for each bot from left to right
    /// tbd pRole.supporterIndex = activeBuddies;  // initally assuming we are righmost bot
    int count = -1;             // so, we start with goali =  supporterIndex[0]

    // if (theRobotInfo.number == 5) OUTPUT_TEXT("buddies" << botsLineUp.size());

    for (auto& mate : botsLineUp)
    {
      count++;

      if (!pRole.playsTheBall() && !pRole.isGoalkeeper()) {  // do not overwrite this two roles
        // if (theRobotInfo.number == 5) OUTPUT_TEXT("count: " << count);

        if (theRobotPose.translation.x() <= mate.xPos)  // we are more left than rightmost
        {
          // pRole.role = PlayerRole::supporter4;
          // pRole.role = static_cast<PlayerRole> (static_cast<int>(PlayerRole::firstSupporterRole) + count);
          switch (count) {
          case 0: pRole.role = PlayerRole::supporter0;   break;
          case 1: pRole.role = PlayerRole::supporter1;   break;
          case 2: pRole.role = PlayerRole::supporter2;   break;
          case 3: pRole.role = PlayerRole::supporter3;   break;
          default: pRole.role = PlayerRole::none; OUTPUT_TEXT("default count: " << count);
          }
          break;
        }
      }

      // for unknown reasons, the code block above does not assign a role for the right most bot.
      // this patch is at least correct: it only runs for the right most bot 
      if (pRole.role == PlayerRole::none) {
        switch (activeBuddies) {
        case 0: pRole.role = PlayerRole::supporter0;   break;
        case 1: pRole.role = PlayerRole::supporter1;   break;
        case 2: pRole.role = PlayerRole::supporter2;   break;
        case 3: pRole.role = PlayerRole::supporter3;   break;
        case 4: pRole.role = PlayerRole::supporter4;   break;
        default: break;
        }
      }
      // ASSERT(role.supporterIndex() - firstSupporterRole <= activeBuddies);  // we are in range supporter0 
      // OUTPUT_TEXT("robot " << theRobotInfo.number << " on role " << role.supporterIndex);
    }
    
    // d2: static assignment 


    if (theGameInfo.state == STATE_INITIAL) {
      // switch (theTeamBehaviorStatus.teamActivity) {
      switch (tbs) {
      case(TeamBehaviorStatus::R2K_DEFENSIVE_GAME):
        teamMateRoles.roles = { TeammateRoles::GOALKEEPER_NORMAL,
                               TeammateRoles::DEFENSE_LEFT,
                               TeammateRoles::DEFENSE_MIDDLE,
                               TeammateRoles::DEFENSE_RIGHT,
                               TeammateRoles::OFFENSE_MIDDLE,
                               TeammateRoles::UNDEFINED };
        break;
      case(TeamBehaviorStatus::R2K_NORMAL_GAME):
        teamMateRoles.roles = { TeammateRoles::GOALKEEPER_NORMAL,
                              TeammateRoles::DEFENSE_LEFT,
                              TeammateRoles::DEFENSE_RIGHT,
                              TeammateRoles::OFFENSE_LEFT,
                              TeammateRoles::OFFENSE_RIGHT,
                              TeammateRoles::UNDEFINED };
        break;
      case(TeamBehaviorStatus::R2K_OFFENSIVE_GAME):
        teamMateRoles.roles = { TeammateRoles::GOALKEEPER_ACTIVE,
                             TeammateRoles::DEFENSE_MIDDLE,
                             TeammateRoles::OFFENSE_LEFT,
                             TeammateRoles::OFFENSE_MIDDLE,
                             TeammateRoles::OFFENSE_RIGHT,
                             TeammateRoles::UNDEFINED };

        break;
      case(TeamBehaviorStatus::R2K_SPARSE_GAME):  // never reached
        teamMateRoles.roles = { TeammateRoles::GOALKEEPER_ACTIVE,
                        TeammateRoles::DEFENSE_LEFT,
                        TeammateRoles::DEFENSE_RIGHT,
                        TeammateRoles::OFFENSE_LEFT,
                        TeammateRoles::OFFENSE_RIGHT,
                        TeammateRoles::UNDEFINED };
        break;
      default: ASSERT(false);
      }
    }
    else {
      //d3: dynamic assignment

      // botsLineUp misses this bot. So we insert this bot at correct position
      auto it = botsLineUp.begin();

      for (auto& mate : botsLineUp) {
        if (theRobotPose.translation.x() < mate.xPos) {
          botsLineUp.insert(it, BotOnField(theRobotInfo.number, theRobotPose.translation.x()));
          break;
        }
        it++;
      }


      // we use roles temporarily to store the robot numbers. 
      // In step d4, we replace these numbers by R2K_TEAM_ROLES
      // 
      // write bot numbers from left to right into roles[], according to their position on field 
      count = 0;
      for (auto& mate : botsLineUp)
      {
        teamMateRoles.roles[count++] = mate.number;
      }
      // fill up roles[] for the penalized bots
      for (int i = activeBuddies + 1; i < 5; i++) teamMateRoles.roles[i] = 0;

      // we don't do dynamic assignment for an active goalie (ie bot #1)
      bool shiftRight = false;
      if (goalieIsActive && (teamMateRoles.roles[0] != 1)) // somehow, the goalie ran into the field                      
      {
        for (int i = 4; i >= 0; i--) {  // search for goalie
          if (1 == teamMateRoles.roles[i]) shiftRight = true;  // "i" is the goalies' offset 
          if (shiftRight) teamMateRoles.roles[i] = teamMateRoles.roles[i-1]; // shift right
        }
        teamMateRoles.roles[0] = 1;
      } // nothing to do wrt. goalie

    // now we have the robots sorted left-to-right in roles[]
    // eg [1,3,2,4,5]
    // 
    // r2k_tactics[5][TeamBehaviorStatus::numOfTeamActivities][5] =
    
      for (int i = 0; i <= 4; i++) {
        // tbs-1 due to  noTeam in TeamBehaviourStatus
        // static
        teamMateRoles.roles[i] = r2k_tactics[activeBuddies][tbs - 1][i];
        
        //dynamic
        /* teamMateRoles.roles[i] = r2k_tactics[activeBuddies][tbs - 1]
          [teamMateRoles.roles[i]-1]*/
      }
  
    } // else: dynamic assignment

    int theCaptain = -1;

    // f) goalie plays the ball?
    if (goalieIsActive && 1 == theRobotInfo.number)  // the regular case
    {
      pRole.role = PlayerRole::goalkeeper;  //check wether goalkeeperAndBallPlayer comes below
      if (pRole.playsTheBall()) PlayerRole::goalkeeperAndBallPlayer;
      theCaptain = 1;
    }
    else  // g) bot#1 is penalized 
    {
      // set the goalie dynamically: choose left-most player. Note: gets dynamically re-computed
      if (!goalieIsActive && PlayerRole::supporter0 == pRole.role)
      {
        pRole.role = PlayerRole::goalkeeper;
        theCaptain = theRobotInfo.number;
        // OUTPUT_TEXT("assigning goalie to" << theRobotInfo.number);
      }
    }

    // get min distance, set playsTheBall(); updates per frame 
    auto dist = 9000;  // max - real field dimensions should be read from config
    auto minDist = 0;
    auto buddyDist = 9000;

   
    if (theFieldBall.ballWasSeen())  // to be on the safe side
      dist = Geometry::distance(theFieldBall.endPositionRelative, Vector2f(0, 0));
    
    TimeToReachBall timeToReachBall;
    // this code fragment does NOT sync teamwiese
    timeToReachBall.timeWhenReachBall = dist + theFrameInfo.time; // +offsetToFrameTimeThisBot;

    minDist = dist;
  
    // e) find min distance to ball for all bots
    for (const auto& buddy : theTeamData.teammates)
    {  // compute and compare my buddies distance with minimal distance
      minDist = std::min(minDist, buddyDist = Geometry::distance(theFieldBall.endPositionOnField, buddy.theRobotPose.translation));
    } // rof: scan team
      

    // ToDo EBC: code to check for striker change, then broadcast
    // f) who plays the ball?    
    if (minDist == dist) {
      if (pRole.isGoalkeeper()) pRole.role = PlayerRole::goalkeeperAndBallPlayer;
      else pRole.role = PlayerRole::ballPlayer;

      timeToReachBall.timeWhenReachBallStriker = timeToReachBall.timeWhenReachBall;  // to: get sync working
 
    }

    theRoleSkill(pRole);
    // if (5 == theRobotInfo.number) OUTPUT_TEXT("sI(): " << pRole.supporterIndex());
    theTimeToReachBallSkill(timeToReachBall);
    theTeammateRolesSkill(teamMateRoles);

  }  // execute

  class BotOnField
  {
  public:
    int number;
    float xPos;

    BotOnField(int n, const float x)
    {
      number = n;
      xPos = x;
    };
    bool operator<(const BotOnField& other) const
    {
      return xPos <= other.xPos;
    }
  };
};

MAKE_TEAM_CARD(R2K_TeamCard);