//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/player.h"
#include "platform/profiler.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "core/stringTable.h"
#include "core/stream/bitStream.h"
#include "core/dnet.h"
#include "core/resManager.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "collision/extrudedPolyList.h"
#include "collision/clippedPolyList.h"
#include "collision/earlyOutPolyList.h"
#include "ts/tsShapeInstance.h"
#include "sfx/sfxSystem.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "terrain/terrData.h"
#include "terrain/terrRender.h"
#include "terrain/waterBlock.h"
#include "app/game.h"
#include "T3D/gameConnection.h"
#include "T3D/trigger.h"
#include "T3D/physicalZone.h"
#include "T3D/item.h"
#include "T3D/missionArea.h"
#include "T3D/fx/particleEmitter.h"
#include "T3D/fx/cameraFXMgr.h"
#include "T3D/fx/splash.h"
#include "T3D/tsStatic.h"
#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsPlayer.h"
#include "sceneGraph/decalManager.h"


//----------------------------------------------------------------------------

// Amount we try to stay out of walls by...
static F32 sWeaponPushBack = 0.03f;

// Amount of time if takes to transition to a new action sequence.
static F32 sAnimationTransitionTime = 0.25f;
static bool sUseAnimationTransitions = true;
static F32 sLandReverseScale = 0.25f;
static F32 sStandingJumpSpeed = 2.0f;
static F32 sJumpingThreshold = 4.0f;
static F32 sSlowStandThreshSquared = 1.69f;
static S32 sRenderMyPlayer = true;
static S32 sRenderMyItems = true;

// Chooses new action animations every n ticks.
static const F32 sNewAnimationTickTime = 4.0f;
static const F32 sMountPendingTickWait = 13.0f * F32(TickMs);

// Number of ticks before we pick non-contact animations
static const S32 sContactTickTime = 10;

// Downward velocity at which we consider the player falling
static const F32 sFallingThreshold = -10.0f;

// Movement constants
static F32 sVerticalStepDot = 0.173f;   // 80
static F32 sMinFaceDistance = 0.01f;
static F32 sTractionDistance = 0.03f;
static F32 sNormalElasticity = 0.01f;
static U32 sMoveRetryCount = 5;

// Client prediction
static F32 sMinWarpTicks = 0.5f;        // Fraction of tick at which instant warp occures
static S32 sMaxWarpTicks = 3;          // Max warp duration in ticks
static S32 sMaxPredictionTicks = 30;   // Number of ticks to predict

// Anchor point compression
const F32 sAnchorMaxDistance = 32.0f;

//
static U32 sCollisionMoveMask = (TerrainObjectType    |
                                 InteriorObjectType     |
                                 WaterObjectType        | PlayerObjectType     |
                                 StaticShapeObjectType  | VehicleObjectType    |
                                 PhysicalZoneObjectType | StaticTSObjectType);

static U32 sServerCollisionContactMask = (sCollisionMoveMask |
                                          (ItemObjectType    |
                                           TriggerObjectType |
                                           CorpseObjectType));

static U32 sClientCollisionContactMask = sCollisionMoveMask | PhysicalZoneObjectType;

enum PlayerConstants {
   JumpSkipContactsMax = 8
};

//----------------------------------------------------------------------------
// Player shape animation sequences:

// look     Used to contol the upper body arm motion.  Must animate
//          vertically +-80 deg.
Player::Range Player::mArmRange(mDegToRad(-80.0f),mDegToRad(+80.0f));

// head     Used to control the direction the head is looking.  Must
//          animated vertically +-80 deg .
Player::Range Player::mHeadVRange(mDegToRad(-80.0f),mDegToRad(+80.0f));

// Action Animations:
PlayerData::ActionAnimationDef PlayerData::ActionAnimationList[NumTableActionAnims] =
{
   // *** WARNING ***
   // This array is indexed using the enum values defined in player.h

   // Root is the default animation
   { "root" },       // RootAnim,

   // These are selected in the move state based on velocity
   { "run",  { 0.0f, 1.0f, 0.0f } },       // RunForwardAnim,
   { "back", { 0.0f, -1.0f, 0.0f } },       // BackBackwardAnim
   { "side", { -1.0f, 0.0f, 0.0f } },       // SideLeftAnim,

   { "crouch_root" },
   { "crouch_forward" },
   { "prone_root" },
   { "prone_forward" },
   { "swim_root" },
   { "swim_forward" },
   { "swim_backward" },
   { "swim_left" },
   { "swim_right" },

   // These are set explicitly based on player actions
   { "climb"},  	 // Climb Anim //Climb Resource
   { "fall" },       // FallAnim
   { "jump" },       // JumpAnim
   { "standjump" },  // StandJumpAnim
   { "land" },       // LandAnim
   { "jet" },        // JetAnim
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(PlayerData);

PlayerData::PlayerData()
{
   shadowEnable = true;
   shadowCanMove = true;
   shadowCanAnimate = true;
   shadowSelfShadow = false;
   shadowSize = 256;
   shadowProjectionDistance = 14.0f;


   renderFirstPerson = true;
   pickupRadius = 0.0f;
   minLookAngle = -1.4f;
   maxLookAngle = 1.4f;
   maxFreelookAngle = 3.0f;
   maxTimeScale = 1.5f;

   mass = 9.0f;        // from shapebase
   maxEnergy = 60.0f;  // from shapebase
   drag = 0.3f;        // from shapebase
   density = 10.0f;

   runForce = 40.0f * 9.0f;
   runEnergyDrain = 0.0f;
   minRunEnergy = 0.0f;
   maxForwardSpeed = 10.0f;
   maxBackwardSpeed = 10.0f;
   maxSideSpeed = 10.0f;

   maxStepHeight = 1.0f;
   runSurfaceAngle = 80.0f;

   recoverDelay = 30;
   recoverRunForceScale = 1.0f;

   // Jumping
   jumpForce = 75.0f;
   jumpEnergyDrain = 0.0f;
   minJumpEnergy = 0.0f;
   jumpSurfaceAngle = 78.0f;
   jumpDelay = 30;
   minJumpSpeed = 500.0f;
   maxJumpSpeed = 2.0f * minJumpSpeed;

   // Swimming
   swimForce = 55.0f * 9.0f;  
   maxUnderwaterForwardSpeed = 6.0f;
   maxUnderwaterBackwardSpeed = 6.0f;
   maxUnderwaterSideSpeed = 6.0f;

   // Crouching
   crouchForce = 45.0f * 9.0f;
   maxCrouchForwardSpeed = 4.0f;
   maxCrouchBackwardSpeed = 4.0f;
   maxCrouchSideSpeed = 4.0f;    

   // Prone
   proneForce = 45.0f * 9.0f;            
   maxProneForwardSpeed = 2.0f;  
   maxProneBackwardSpeed = 2.0f; 
   maxProneSideSpeed = 0.0f;     

   // Jetting
   jetJumpForce = 0;
   jetJumpEnergyDrain = 0;
   jetMinJumpEnergy = 0;
   jetJumpSurfaceAngle = 78;
   jetMinJumpSpeed = 20;
   jetMaxJumpSpeed = 100;

   horizMaxSpeed = 80.0f;
   horizResistSpeed = 38.0f;
   horizResistFactor = 1.0f;

   upMaxSpeed = 80.0f;
   upResistSpeed = 38.0f;
   upResistFactor = 1.0f;

   minImpactSpeed = 25.0f;

   decalData      = NULL;
   decalID        = 0;
   decalOffset      = 0.0f;

   lookAction = 0;

   // size of bounding box
   boxSize.set(1.0f, 1.0f, 2.3f);
   crouchBoxSize.set(1.0f, 1.0f, 2.0f);
   proneBoxSize.set(1.0f, 2.3f, 1.0f);
   swimBoxSize.set(1.0f, 2.3f, 1.0f);

   // location of head, torso, legs
   boxHeadPercentage = 0.85f;
   boxTorsoPercentage = 0.55f;

   // damage locations
   boxHeadLeftPercentage  = 0;
   boxHeadRightPercentage = 1;
   boxHeadBackPercentage  = 0;
   boxHeadFrontPercentage = 1;

   for (S32 i = 0; i < MaxSounds; i++)
      sound[i] = NULL;

   footPuffEmitter = NULL;
   footPuffID = 0;
   footPuffNumParts = 15;
   footPuffRadius = .25f;

   dustEmitter = NULL;
   dustID = 0;

   splash = NULL;
   splashId = 0;
   splashVelocity = 1.0f;
   splashAngle = 45.0f;
   splashFreqMod = 300.0f;
   splashVelEpsilon = 0.25f;
   bubbleEmitTime = 0.4f;

   medSplashSoundVel = 2.0f;
   hardSplashSoundVel = 3.0f;
   exitSplashSoundVel = 2.0f;
   footSplashHeight = 0.1f;

   dMemset( splashEmitterList, 0, sizeof( splashEmitterList ) );
   dMemset( splashEmitterIDList, 0, sizeof( splashEmitterIDList ) );

   groundImpactMinSpeed = 10.0f;
   groundImpactShakeFreq.set( 10.0f, 10.0f, 10.0f );
   groundImpactShakeAmp.set( 20.0f, 20.0f, 20.0f );
   groundImpactShakeDuration = 1.0f;
   groundImpactShakeFalloff = 10.0f;

   // Air control
   airControl = 0.0f;

   jumpTowardsNormal = true;

   physicsPlayerType = StringTable->insert("");

   dMemset( actionList, 0, sizeof(actionList) );
}

bool PlayerData::preload(bool server, String &errorStr)
{
   if(!Parent::preload(server, errorStr))
      return false;

   // Resolve objects transmitted from server
   if (!server) {
      for (S32 i = 0; i < MaxSounds; i++)
         if (sound[i])
            Sim::findObject(SimObjectId(sound[i]),sound[i]);
   }

   //
   runSurfaceCos = mCos(mDegToRad(runSurfaceAngle));
   jumpSurfaceCos = mCos(mDegToRad(jumpSurfaceAngle));
   if (minJumpEnergy < jumpEnergyDrain)
      minJumpEnergy = jumpEnergyDrain;   

   // Jetting
   if (jetMinJumpEnergy < jetJumpEnergyDrain)
      jetMinJumpEnergy = jetJumpEnergyDrain;

   // Validate some of the data
   if (recoverDelay > (1 << RecoverDelayBits) - 1) {
      recoverDelay = (1 << RecoverDelayBits) - 1;
      Con::printf("PlayerData:: Recover delay exceeds range (0-%d)",recoverDelay);
   }
   if (jumpDelay > (1 << JumpDelayBits) - 1) {
      jumpDelay = (1 << JumpDelayBits) - 1;
      Con::printf("PlayerData:: Jump delay exceeds range (0-%d)",jumpDelay);
   }

   // If we don't have a shape don't crash out trying to
   // setup animations and sequences.
   if ( mShape )
   {
      // Go ahead a pre-load the player shape
      TSShapeInstance* si = new TSShapeInstance(mShape, false);
      TSThread* thread = si->addThread();

      // Extract ground transform velocity from animations
      // Get the named ones first so they can be indexed directly.
      ActionAnimation *dp = &actionList[0];
      for (int i = 0; i < NumTableActionAnims; i++,dp++)
      {
         ActionAnimationDef *sp = &ActionAnimationList[i];
         dp->name          = sp->name;
         dp->dir.set(sp->dir.x,sp->dir.y,sp->dir.z);
         dp->sequence      = mShape->findSequence(sp->name);
         dp->velocityScale = true;
         dp->death         = false;
         if (dp->sequence != -1)
            getGroundInfo(si,thread,dp);

         // No real reason to spam the console about a missing jet animation
         if (dStricmp(sp->name, "jet") != 0)
            AssertWarn(dp->sequence != -1, avar("PlayerData::preload - Unable to find named animation sequence '%s'!", sp->name));
      }
      for (int b = 0; b < mShape->sequences.size(); b++)
      {
         if (!isTableSequence(b))
         {
            dp->sequence      = b;
            dp->name          = mShape->getName(mShape->sequences[b].nameIndex);
            dp->velocityScale = false;
            getGroundInfo(si,thread,dp++);
         }
      }
      actionCount = dp - actionList;
      AssertFatal(actionCount <= NumActionAnims, "Too many action animations!");
      delete si;

      // Resolve lookAction index
      dp = &actionList[0];
      String lookName("look");
      for (int c = 0; c < actionCount; c++,dp++)
         if (dp->name == lookName)
            lookAction = c;

      // Resolve spine
      spineNode[0] = mShape->findNode("Bip01 Pelvis");
      spineNode[1] = mShape->findNode("Bip01 Spine");
      spineNode[2] = mShape->findNode("Bip01 Spine1");
      spineNode[3] = mShape->findNode("Bip01 Spine2");
      spineNode[4] = mShape->findNode("Bip01 Neck");
      spineNode[5] = mShape->findNode("Bip01 Head");

      // Recoil animations
      recoilSequence[0] = mShape->findSequence("light_recoil");
      recoilSequence[1] = mShape->findSequence("medium_recoil");
      recoilSequence[2] = mShape->findSequence("heavy_recoil");
   }

   // Convert pickupRadius to a delta of boundingBox
   //
   // NOTE: it isn't technically correct to precalculate this based off
   // the boxSize since the player's boxSize can now change depending on
   // his PlayerPose, but since the pickupRadius actually used (unless the
   // boxSize is too small), it shouldn't matter.
   //
   F32 dr = (boxSize.x > boxSize.y)? boxSize.x: boxSize.y;
   if (pickupRadius < dr)
      pickupRadius = dr;
   else
      if (pickupRadius > 2.0f * dr)
         pickupRadius = 2.0f * dr;
   pickupDelta = (S32)(pickupRadius - dr);

   // Validate jump speed
   if (maxJumpSpeed <= minJumpSpeed)
      maxJumpSpeed = minJumpSpeed + 0.1f;

   // Load up all the emitters
   if (!footPuffEmitter && footPuffID != 0)
      if (!Sim::findObject(footPuffID, footPuffEmitter))
         Con::errorf(ConsoleLogEntry::General, "PlayerData::preload - Invalid packet, bad datablockId(footPuffEmitter): 0x%x", footPuffID);

   if (!decalData && decalID != 0 )
      if (!Sim::findObject(decalID, decalData))
         Con::errorf(ConsoleLogEntry::General, "PlayerData::preload Invalid packet, bad datablockId(decalData): 0x%x", decalID);

   if (!dustEmitter && dustID != 0 )
      if (!Sim::findObject(dustID, dustEmitter))
         Con::errorf(ConsoleLogEntry::General, "PlayerData::preload - Invalid packet, bad datablockId(dustEmitter): 0x%x", dustID);

   for (int i=0; i<NUM_SPLASH_EMITTERS; i++)
      if( !splashEmitterList[i] && splashEmitterIDList[i] != 0 )
         if( Sim::findObject( splashEmitterIDList[i], splashEmitterList[i] ) == false)
            Con::errorf(ConsoleLogEntry::General, "PlayerData::onAdd - Invalid packet, bad datablockId(particle emitter): 0x%x", splashEmitterIDList[i]);

   return true;
}

void PlayerData::getGroundInfo(TSShapeInstance* si, TSThread* thread,ActionAnimation *dp)
{
   dp->death = !dStrnicmp(dp->name, "death", 5);
   if (dp->death)
   {
      // Death animations use roll frame-to-frame changes in ground transform into position
      dp->speed = 0.0f;
      dp->dir.set(0.0f, 0.0f, 0.0f);
   }
   else
   {
      VectorF save = dp->dir;
      si->setSequence(thread,dp->sequence,0);
      si->animate();
      si->advanceTime(1);
      si->animateGround();
      si->getGroundTransform().getColumn(3,&dp->dir);
      if ((dp->speed = dp->dir.len()) < 0.01f)
      {
         // No ground displacement... In this case we'll use the
         // default table entry, if there is one.
         if (save.len() > 0.01f)
         {
            dp->dir = save;
            dp->speed = 1.0f;
            dp->velocityScale = false;
         }
         else
            dp->speed = 0.0f;
      }
      else
         dp->dir *= 1.0f / dp->speed;
   }
}

bool PlayerData::isTableSequence(S32 seq)
{
   // The sequences from the table must already have
   // been loaded for this to work.
   for (int i = 0; i < NumTableActionAnims; i++)
      if (actionList[i].sequence == seq)
         return true;
   return false;
}

bool PlayerData::isJumpAction(U32 action)
{
   return (action == JumpAnim || action == StandJumpAnim);
}

void PlayerData::initPersistFields()
{
   Parent::initPersistFields();

   addField("renderFirstPerson", TypeBool, Offset(renderFirstPerson, PlayerData));
   addField("pickupRadius", TypeF32, Offset(pickupRadius, PlayerData));

   addField("minLookAngle", TypeF32, Offset(minLookAngle, PlayerData));
   addField("maxLookAngle", TypeF32, Offset(maxLookAngle, PlayerData));
   addField("maxFreelookAngle", TypeF32, Offset(maxFreelookAngle, PlayerData));

   addField("maxTimeScale", TypeF32, Offset(maxTimeScale, PlayerData));

   addField("maxStepHeight", TypeF32, Offset(maxStepHeight, PlayerData));
   addField("runForce", TypeF32, Offset(runForce, PlayerData));
   addField("runEnergyDrain", TypeF32, Offset(runEnergyDrain, PlayerData));
   addField("minRunEnergy", TypeF32, Offset(minRunEnergy, PlayerData));
   addField("maxForwardSpeed", TypeF32, Offset(maxForwardSpeed, PlayerData));
   addField("maxBackwardSpeed", TypeF32, Offset(maxBackwardSpeed, PlayerData));
   addField("maxSideSpeed", TypeF32, Offset(maxSideSpeed, PlayerData));
   addField("runSurfaceAngle", TypeF32, Offset(runSurfaceAngle, PlayerData));
   addField("minImpactSpeed", TypeF32, Offset(minImpactSpeed, PlayerData));

   addField("recoverDelay", TypeS32, Offset(recoverDelay, PlayerData));
   addField("recoverRunForceScale", TypeF32, Offset(recoverRunForceScale, PlayerData));

   addField("jumpForce", TypeF32, Offset(jumpForce, PlayerData));
   addField("jumpEnergyDrain", TypeF32, Offset(jumpEnergyDrain, PlayerData));
   addField("minJumpEnergy", TypeF32, Offset(minJumpEnergy, PlayerData));
   addField("minJumpSpeed", TypeF32, Offset(minJumpSpeed, PlayerData));
   addField("maxJumpSpeed", TypeF32, Offset(maxJumpSpeed, PlayerData));
   addField("jumpSurfaceAngle", TypeF32, Offset(jumpSurfaceAngle, PlayerData));
   addField("jumpDelay", TypeS32, Offset(jumpDelay, PlayerData));

   // Swimming
   addField("swimForce", TypeF32, Offset(swimForce, PlayerData));
   addField("maxUnderwaterForwardSpeed", TypeF32, Offset(maxUnderwaterForwardSpeed, PlayerData));
   addField("maxUnderwaterBackwardSpeed", TypeF32, Offset(maxUnderwaterBackwardSpeed, PlayerData));
   addField("maxUnderwaterSideSpeed", TypeF32, Offset(maxUnderwaterSideSpeed, PlayerData));

   // Crouching
   addField("crouchForce", TypeF32, Offset(crouchForce, PlayerData));
   addField("maxCrouchForwardSpeed", TypeF32, Offset(maxCrouchForwardSpeed, PlayerData));
   addField("maxCrouchBackwardSpeed", TypeF32, Offset(maxCrouchBackwardSpeed, PlayerData));
   addField("maxCrouchSideSpeed", TypeF32, Offset(maxCrouchSideSpeed, PlayerData));

   // Prone
   addField("proneForce", TypeF32, Offset(proneForce, PlayerData));
   addField("maxProneForwardSpeed", TypeF32, Offset(maxProneForwardSpeed, PlayerData));
   addField("maxProneBackwardSpeed", TypeF32, Offset(maxProneBackwardSpeed, PlayerData));
   addField("maxProneSideSpeed", TypeF32, Offset(maxProneSideSpeed, PlayerData));

   // Jetting
   addField("jetJumpForce", TypeF32, Offset(jetJumpForce, PlayerData));
   addField("jetJumpEnergyDrain", TypeF32, Offset(jetJumpEnergyDrain, PlayerData));
   addField("jetMinJumpEnergy", TypeF32, Offset(jetMinJumpEnergy, PlayerData));
   addField("jetMinJumpSpeed", TypeF32, Offset(jetMinJumpSpeed, PlayerData));
   addField("jetMaxJumpSpeed", TypeF32, Offset(jetMaxJumpSpeed, PlayerData));
   addField("jetJumpSurfaceAngle", TypeF32, Offset(jetJumpSurfaceAngle, PlayerData));

   addField("boundingBox", TypePoint3F, Offset(boxSize, PlayerData));
   addField("crouchBoundingBox", TypePoint3F, Offset(crouchBoxSize, PlayerData));
   addField("proneBoundingBox", TypePoint3F, Offset(proneBoxSize, PlayerData));
   addField("swimBoundingBox", TypePoint3F, Offset(swimBoxSize, PlayerData));
   
   addField("boxHeadPercentage", TypeF32, Offset(boxHeadPercentage, PlayerData));
   addField("boxTorsoPercentage", TypeF32, Offset(boxTorsoPercentage, PlayerData));
   addField("boxHeadLeftPercentage", TypeS32, Offset(boxHeadLeftPercentage, PlayerData));
   addField("boxHeadRightPercentage", TypeS32, Offset(boxHeadRightPercentage, PlayerData));
   addField("boxHeadBackPercentage", TypeS32, Offset(boxHeadBackPercentage, PlayerData));
   addField("boxHeadFrontPercentage", TypeS32, Offset(boxHeadFrontPercentage, PlayerData));

   addField("horizMaxSpeed", TypeF32, Offset(horizMaxSpeed, PlayerData));
   addField("horizResistSpeed", TypeF32, Offset(horizResistSpeed, PlayerData));
   addField("horizResistFactor", TypeF32, Offset(horizResistFactor, PlayerData));

   addField("upMaxSpeed", TypeF32, Offset(upMaxSpeed, PlayerData));
   addField("upResistSpeed", TypeF32, Offset(upResistSpeed, PlayerData));
   addField("upResistFactor", TypeF32, Offset(upResistFactor, PlayerData));

   addField("decalData",         TypeDecalDataPtr, Offset(decalData, PlayerData));
   addField("decalOffset",TypeF32, Offset(decalOffset, PlayerData));

   addField("footPuffEmitter",   TypeParticleEmitterDataPtr,   Offset(footPuffEmitter,    PlayerData));
   addField("footPuffNumParts",  TypeS32,                      Offset(footPuffNumParts,   PlayerData));
   addField("footPuffRadius",    TypeF32,                      Offset(footPuffRadius,     PlayerData));
   addField("dustEmitter",       TypeParticleEmitterDataPtr,   Offset(dustEmitter,        PlayerData));

   addField("FootSoftSound",       TypeSFXProfilePtr, Offset(sound[FootSoft],          PlayerData));
   addField("FootHardSound",       TypeSFXProfilePtr, Offset(sound[FootHard],          PlayerData));
   addField("FootMetalSound",      TypeSFXProfilePtr, Offset(sound[FootMetal],         PlayerData));
   addField("FootSnowSound",       TypeSFXProfilePtr, Offset(sound[FootSnow],          PlayerData));
   addField("FootShallowSound",    TypeSFXProfilePtr, Offset(sound[FootShallowSplash], PlayerData));
   addField("FootWadingSound",     TypeSFXProfilePtr, Offset(sound[FootWading],        PlayerData));
   addField("FootUnderwaterSound", TypeSFXProfilePtr, Offset(sound[FootUnderWater],    PlayerData));
   addField("FootBubblesSound",    TypeSFXProfilePtr, Offset(sound[FootBubbles],       PlayerData));
   addField("movingBubblesSound",   TypeSFXProfilePtr, Offset(sound[MoveBubbles],        PlayerData));
   addField("waterBreathSound",     TypeSFXProfilePtr, Offset(sound[WaterBreath],        PlayerData));

   addField("impactSoftSound",   TypeSFXProfilePtr, Offset(sound[ImpactSoft],  PlayerData));
   addField("impactHardSound",   TypeSFXProfilePtr, Offset(sound[ImpactHard],  PlayerData));
   addField("impactMetalSound",  TypeSFXProfilePtr, Offset(sound[ImpactMetal], PlayerData));
   addField("impactSnowSound",   TypeSFXProfilePtr, Offset(sound[ImpactSnow],  PlayerData));

   addField("mediumSplashSoundVelocity", TypeF32,     Offset(medSplashSoundVel,  PlayerData));
   addField("hardSplashSoundVelocity",   TypeF32,     Offset(hardSplashSoundVel,  PlayerData));
   addField("exitSplashSoundVelocity",   TypeF32,     Offset(exitSplashSoundVel,  PlayerData));

   addField("impactWaterEasy",   TypeSFXProfilePtr, Offset(sound[ImpactWaterEasy],   PlayerData));
   addField("impactWaterMedium", TypeSFXProfilePtr, Offset(sound[ImpactWaterMedium], PlayerData));
   addField("impactWaterHard",   TypeSFXProfilePtr, Offset(sound[ImpactWaterHard],   PlayerData));
   addField("exitingWater",      TypeSFXProfilePtr, Offset(sound[ExitWater],         PlayerData));

   addField("splash",         TypeSplashDataPtr,      Offset(splash,          PlayerData));
   addField("splashVelocity", TypeF32,                Offset(splashVelocity,  PlayerData));
   addField("splashAngle",    TypeF32,                Offset(splashAngle,     PlayerData));
   addField("splashFreqMod",  TypeF32,                Offset(splashFreqMod,   PlayerData));
   addField("splashVelEpsilon", TypeF32,              Offset(splashVelEpsilon, PlayerData));
   addField("bubbleEmitTime", TypeF32,                Offset(bubbleEmitTime,  PlayerData));
   addField("splashEmitter",  TypeParticleEmitterDataPtr,   Offset(splashEmitterList,   PlayerData), NUM_SPLASH_EMITTERS);
   addField("footstepSplashHeight",      TypeF32,     Offset(footSplashHeight,  PlayerData));

   addField("groundImpactMinSpeed",       TypeF32,       Offset(groundImpactMinSpeed,        PlayerData));
   addField("groundImpactShakeFreq",      TypePoint3F,   Offset(groundImpactShakeFreq,       PlayerData));
   addField("groundImpactShakeAmp",       TypePoint3F,   Offset(groundImpactShakeAmp,        PlayerData));
   addField("groundImpactShakeDuration",  TypeF32,       Offset(groundImpactShakeDuration,   PlayerData));
   addField("groundImpactShakeFalloff",   TypeF32,       Offset(groundImpactShakeFalloff,    PlayerData));

   // Air control
   addField("airControl",                 TypeF32,       Offset(airControl,PlayerData));
   addField("jumpTowardsNormal",          TypeBool,      Offset(jumpTowardsNormal,PlayerData));

   // PhysicsPlayer
   addField("physicsPlayerType",          TypeString,    Offset(physicsPlayerType,PlayerData));
}

void PlayerData::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->writeFlag(renderFirstPerson);

   stream->write(minLookAngle);
   stream->write(maxLookAngle);
   stream->write(maxFreelookAngle);
   stream->write(maxTimeScale);

   stream->write(mass);
   stream->write(maxEnergy);
   stream->write(drag);
   stream->write(density);

   stream->write(maxStepHeight);

   stream->write(runForce);
   stream->write(runEnergyDrain);
   stream->write(minRunEnergy);
   stream->write(maxForwardSpeed);
   stream->write(maxBackwardSpeed);
   stream->write(maxSideSpeed);
   stream->write(runSurfaceAngle);

   stream->write(recoverDelay);
   stream->write(recoverRunForceScale);

   // Jumping
   stream->write(jumpForce);
   stream->write(jumpEnergyDrain);
   stream->write(minJumpEnergy);
   stream->write(minJumpSpeed);
   stream->write(maxJumpSpeed);
   stream->write(jumpSurfaceAngle);
   stream->writeInt(jumpDelay,JumpDelayBits);

   // Swimming
   stream->write(swimForce);   
   stream->write(maxUnderwaterForwardSpeed);
   stream->write(maxUnderwaterBackwardSpeed);
   stream->write(maxUnderwaterSideSpeed);

   // Crouching
   stream->write(crouchForce);   
   stream->write(maxCrouchForwardSpeed);
   stream->write(maxCrouchBackwardSpeed);
   stream->write(maxCrouchSideSpeed);

   // Prone
   stream->write(proneForce);   
   stream->write(maxProneForwardSpeed);
   stream->write(maxProneBackwardSpeed);
   stream->write(maxProneSideSpeed);

   // Jetting
   stream->write(jetJumpForce);
   stream->write(jetJumpEnergyDrain);
   stream->write(jetMinJumpEnergy);
   stream->write(jetMinJumpSpeed);
   stream->write(jetMaxJumpSpeed);
   stream->write(jetJumpSurfaceAngle);

   stream->write(horizMaxSpeed);
   stream->write(horizResistSpeed);
   stream->write(horizResistFactor);

   stream->write(upMaxSpeed);
   stream->write(upResistSpeed);
   stream->write(upResistFactor);

   stream->write(splashVelocity);
   stream->write(splashAngle);
   stream->write(splashFreqMod);
   stream->write(splashVelEpsilon);
   stream->write(bubbleEmitTime);

   stream->write(medSplashSoundVel);
   stream->write(hardSplashSoundVel);
   stream->write(exitSplashSoundVel);
   stream->write(footSplashHeight);
   // Don't need damage scale on the client
   stream->write(minImpactSpeed);

   S32 i;
   for ( i = 0; i < MaxSounds; i++)
      if (stream->writeFlag(sound[i]))
         stream->writeRangedU32(packed? SimObjectId(sound[i]):
                                sound[i]->getId(),DataBlockObjectIdFirst,DataBlockObjectIdLast);

   mathWrite(*stream, boxSize);
   mathWrite(*stream, crouchBoxSize);
   mathWrite(*stream, proneBoxSize);
   mathWrite(*stream, swimBoxSize);

   if( stream->writeFlag( footPuffEmitter ) )
   {
      stream->writeRangedU32( footPuffEmitter->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
   }

   stream->write( footPuffNumParts );
   stream->write( footPuffRadius );

   if( stream->writeFlag( decalData ) )
   {
      stream->writeRangedU32( decalData->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
   }
   stream->write(decalOffset);

   if( stream->writeFlag( dustEmitter ) )
   {
      stream->writeRangedU32( dustEmitter->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
   }


   if (stream->writeFlag( splash ))
   {
      stream->writeRangedU32(splash->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
   }

   for( i=0; i<NUM_SPLASH_EMITTERS; i++ )
   {
      if( stream->writeFlag( splashEmitterList[i] != NULL ) )
      {
         stream->writeRangedU32( splashEmitterList[i]->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
      }
   }


   stream->write(groundImpactMinSpeed);
   stream->write(groundImpactShakeFreq.x);
   stream->write(groundImpactShakeFreq.y);
   stream->write(groundImpactShakeFreq.z);
   stream->write(groundImpactShakeAmp.x);
   stream->write(groundImpactShakeAmp.y);
   stream->write(groundImpactShakeAmp.z);
   stream->write(groundImpactShakeDuration);
   stream->write(groundImpactShakeFalloff);

   // Air control
   stream->write(airControl);

   // Jump off at normal
   stream->writeFlag(jumpTowardsNormal);

   stream->writeString(physicsPlayerType);
}

void PlayerData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   renderFirstPerson = stream->readFlag();

   stream->read(&minLookAngle);
   stream->read(&maxLookAngle);
   stream->read(&maxFreelookAngle);
   stream->read(&maxTimeScale);

   stream->read(&mass);
   stream->read(&maxEnergy);
   stream->read(&drag);
   stream->read(&density);

   stream->read(&maxStepHeight);

   stream->read(&runForce);
   stream->read(&runEnergyDrain);
   stream->read(&minRunEnergy);
   stream->read(&maxForwardSpeed);
   stream->read(&maxBackwardSpeed);
   stream->read(&maxSideSpeed);
   stream->read(&runSurfaceAngle);

   stream->read(&recoverDelay);
   stream->read(&recoverRunForceScale);

   // Jumping
   stream->read(&jumpForce);
   stream->read(&jumpEnergyDrain);
   stream->read(&minJumpEnergy);
   stream->read(&minJumpSpeed);
   stream->read(&maxJumpSpeed);
   stream->read(&jumpSurfaceAngle);
   jumpDelay = stream->readInt(JumpDelayBits);

   // Swimming
   stream->read(&swimForce);
   stream->read(&maxUnderwaterForwardSpeed);
   stream->read(&maxUnderwaterBackwardSpeed);
   stream->read(&maxUnderwaterSideSpeed);   

   // Crouching
   stream->read(&crouchForce);
   stream->read(&maxCrouchForwardSpeed);
   stream->read(&maxCrouchBackwardSpeed);
   stream->read(&maxCrouchSideSpeed);

   // Prone
   stream->read(&proneForce);
   stream->read(&maxProneForwardSpeed);
   stream->read(&maxProneBackwardSpeed);
   stream->read(&maxProneSideSpeed);

   // Jetting
   stream->read(&jetJumpForce);
   stream->read(&jetJumpEnergyDrain);
   stream->read(&jetMinJumpEnergy);
   stream->read(&jetMinJumpSpeed);
   stream->read(&jetMaxJumpSpeed);
   stream->read(&jetJumpSurfaceAngle);

   stream->read(&horizMaxSpeed);
   stream->read(&horizResistSpeed);
   stream->read(&horizResistFactor);

   stream->read(&upMaxSpeed);
   stream->read(&upResistSpeed);
   stream->read(&upResistFactor);

   stream->read(&splashVelocity);
   stream->read(&splashAngle);
   stream->read(&splashFreqMod);
   stream->read(&splashVelEpsilon);
   stream->read(&bubbleEmitTime);

   stream->read(&medSplashSoundVel);
   stream->read(&hardSplashSoundVel);
   stream->read(&exitSplashSoundVel);
   stream->read(&footSplashHeight);

   stream->read(&minImpactSpeed);

   S32 i;
   for (i = 0; i < MaxSounds; i++) {
      sound[i] = NULL;
      if (stream->readFlag())
         sound[i] = (SFXProfile*)stream->readRangedU32(DataBlockObjectIdFirst,
                                                         DataBlockObjectIdLast);
   }

   mathRead(*stream, &boxSize);
   mathRead(*stream, &crouchBoxSize);
   mathRead(*stream, &proneBoxSize);
   mathRead(*stream, &swimBoxSize);

   if( stream->readFlag() )
   {
      footPuffID = (S32) stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   }

   stream->read(&footPuffNumParts);
   stream->read(&footPuffRadius);

   if( stream->readFlag() )
   {
      decalID = (S32) stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   }
   stream->read(&decalOffset);

   if( stream->readFlag() )
   {
      dustID = (S32) stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   }


   if (stream->readFlag())
   {
      splashId = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
   }

   for( i=0; i<NUM_SPLASH_EMITTERS; i++ )
   {
      if( stream->readFlag() )
      {
         splashEmitterIDList[i] = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
      }
   }

   stream->read(&groundImpactMinSpeed);
   stream->read(&groundImpactShakeFreq.x);
   stream->read(&groundImpactShakeFreq.y);
   stream->read(&groundImpactShakeFreq.z);
   stream->read(&groundImpactShakeAmp.x);
   stream->read(&groundImpactShakeAmp.y);
   stream->read(&groundImpactShakeAmp.z);
   stream->read(&groundImpactShakeDuration);
   stream->read(&groundImpactShakeFalloff);

   // Air control
   stream->read(&airControl);

   // Jump off at normal
   jumpTowardsNormal = stream->readFlag();

   physicsPlayerType = stream->readSTString();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(Player);
F32 Player::mGravity = -20;


//----------------------------------------------------------------------------

Player::Player()
{
   mTypeMask |= PlayerObjectType;

   delta.pos = mAnchorPoint = Point3F(0,0,100);
   delta.rot = delta.head = Point3F(0,0,0);
   delta.rotOffset.set(0.0f,0.0f,0.0f);
   delta.warpOffset.set(0.0f,0.0f,0.0f);
   delta.posVec.set(0.0f,0.0f,0.0f);
   delta.rotVec.set(0.0f,0.0f,0.0f);
   delta.headVec.set(0.0f,0.0f,0.0f);
   delta.warpTicks = 0;
   delta.dt = 1.0f;
   delta.move = NullMove;
   mPredictionCount = sMaxPredictionTicks;
   mObjToWorld.setColumn(3,delta.pos);
   mRot = delta.rot;
   mHead = delta.head;
   mVelocity.set(0.0f, 0.0f, 0.0f);
   mDataBlock = 0;
   mHeadHThread = mHeadVThread = mRecoilThread = 0;
   mArmAnimation.action = PlayerData::NullAnimation;
   mArmAnimation.thread = 0;
   mActionAnimation.action = PlayerData::NullAnimation;
   mActionAnimation.thread = 0;
   mActionAnimation.delayTicks = 0;
   mActionAnimation.forward = true;
   mActionAnimation.firstPerson = false;
   //mActionAnimation.time = 1.0f; //ActionAnimation::Scale;
   mActionAnimation.waitForEnd = false;
   mActionAnimation.holdAtEnd = false;
   mActionAnimation.animateOnServer = false;
   mActionAnimation.atEnd = false;
   mState = MoveState;
   mFalling = false;
   mClimbing = false;  //Climb Resource
   mSwimming = false;
   mInWater = false;
   mPose = StandPose;
   mContactTimer = 0;
   mJumpDelay = 0;
   mJumpSurfaceLastContact = 0;
   mJumpSurfaceNormal.set(0.0f, 0.0f, 1.0f);
   mControlObject = 0;
   dMemset( mSplashEmitter, 0, sizeof( mSplashEmitter ) );

   mImpactSound = 0;
   mRecoverTicks = 0;
   mReversePending = 0;

   mLastPos.set( 0.0f, 0.0f, 0.0f );

   mMoveBubbleSound = 0;
   mWaterBreathSound = 0;   

   mConvex.init(this);
   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   mWeaponBackFraction = 0.0f;

   mInMissionArea = true;

   mBubbleEmitterTime = 10.0;
   mLastWaterPos.set( 0.0, 0.0, 0.0 );

   mMountPending = 0;

   mNSLinkMask = LinkSuperClassName | LinkClassName;

   mPhysicsPlayer = NULL;
}

Player::~Player()
{
}


//----------------------------------------------------------------------------

bool Player::onAdd()
{
   ActionAnimation serverAnim = mActionAnimation;
   if(!Parent::onAdd() || !mDataBlock)
      return false;

   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   addToScene();

   // Make sure any state and animation passed from the server
   // in the initial update is set correctly.
   ActionState state = mState;
   mState = NullState;
   setState(state);
   setPose(StandPose);

   if (serverAnim.action != PlayerData::NullAnimation)
   {
      setActionThread(serverAnim.action, true, serverAnim.holdAtEnd, true, false, true);
      if (serverAnim.atEnd)
      {
         mShapeInstance->clearTransition(mActionAnimation.thread);
         mShapeInstance->setPos(mActionAnimation.thread,
                                mActionAnimation.forward ? 1.0f : 0.0f);
         if (inDeathAnim())
            mDeath.lastPos = 1.0f;
      }

      // We have to leave them sitting for a while since mounts don't come through right
      // away (and sometimes not for a while).  Still going to let this time out because
      // I'm not sure if we're guaranteed another anim will come through and cancel.
      if (!isServerObject() && inSittingAnim())
         mMountPending = (S32) sMountPendingTickWait;
      else
         mMountPending = 0;
   }
   if (mArmAnimation.action != PlayerData::NullAnimation)
      setArmThread(mArmAnimation.action);

   //
   if (isServerObject())
   {
      scriptOnAdd();
   }
   else
   {
      U32 i;
      for( i=0; i<PlayerData::NUM_SPLASH_EMITTERS; i++ )
      {
         if ( mDataBlock->splashEmitterList[i] ) 
         {
            mSplashEmitter[i] = new ParticleEmitter;
            mSplashEmitter[i]->onNewDataBlock( mDataBlock->splashEmitterList[i] );
            if( !mSplashEmitter[i]->registerObject() )
            {
               Con::warnf( ConsoleLogEntry::General, "Could not register splash emitter for class: %s", mDataBlock->getName() );
               delete mSplashEmitter[i];
               mSplashEmitter[i] = NULL;
            }
         }
      }
      mLastWaterPos = getPosition();

      // clear out all camera effects
      gCamFXMgr.clear();
   }

   if ( gPhysicsPlugin )
      mPhysicsPlayer = gPhysicsPlugin->createPlayer( this );

   return true;
}

void Player::onRemove()
{
   setControlObject(0);
   scriptOnRemove();
   removeFromScene();

   U32 i;
   for( i=0; i<PlayerData::NUM_SPLASH_EMITTERS; i++ )
   {
      if( mSplashEmitter[i] )
      {
         mSplashEmitter[i]->deleteWhenEmpty();
         mSplashEmitter[i] = NULL;
      }
   }

   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   SAFE_DELETE( mPhysicsPlayer );		

   Parent::onRemove();
}

void Player::onScaleChanged()
{
   const Point3F& scale = getScale();
   mScaledBox = mObjBox;
   mScaledBox.minExtents.convolve( scale );
   mScaledBox.maxExtents.convolve( scale );
}


//----------------------------------------------------------------------------

bool Player::onNewDataBlock(GameBaseData* dptr)
{
   PlayerData* prevData = mDataBlock;
   mDataBlock = dynamic_cast<PlayerData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   // Initialize arm thread, preserve arm sequence from last datablock.
   // Arm animation can be from last datablock, or sent from the server.
   U32 prevAction = mArmAnimation.action;
   mArmAnimation.action = PlayerData::NullAnimation;
   if (mDataBlock->lookAction) {
      mArmAnimation.thread = mShapeInstance->addThread();
      mShapeInstance->setTimeScale(mArmAnimation.thread,0);
      if (prevData) {
         if (prevAction != prevData->lookAction && prevAction != PlayerData::NullAnimation)
            setArmThread(prevData->actionList[prevAction].name);
         prevAction = PlayerData::NullAnimation;
      }
      if (mArmAnimation.action == PlayerData::NullAnimation) {
         mArmAnimation.action = (prevAction != PlayerData::NullAnimation)?
            prevAction: mDataBlock->lookAction;
         mShapeInstance->setSequence(mArmAnimation.thread,
           mDataBlock->actionList[mArmAnimation.action].sequence,0);
      }
   }
   else
      mArmAnimation.thread = 0;

   // Initialize head look thread
   TSShape const* shape = mShapeInstance->getShape();
   S32 headSeq = shape->findSequence("head");
   if (headSeq != -1) {
      mHeadVThread = mShapeInstance->addThread();
      mShapeInstance->setSequence(mHeadVThread,headSeq,0);
      mShapeInstance->setTimeScale(mHeadVThread,0);
   }
   else
      mHeadVThread = 0;

   headSeq = shape->findSequence("headside");
   if (headSeq != -1) {
      mHeadHThread = mShapeInstance->addThread();
      mShapeInstance->setSequence(mHeadHThread,headSeq,0);
      mShapeInstance->setTimeScale(mHeadHThread,0);
   }
   else
      mHeadHThread = 0;

   // Recoil thread. The server player does not play this animation.
   mRecoilThread = 0;
   if (isGhost())
      for (U32 s = 0; s < PlayerData::NumRecoilSequences; s++)
         if (mDataBlock->recoilSequence[s] != -1) {
            mRecoilThread = mShapeInstance->addThread();
            mShapeInstance->setSequence(mRecoilThread,mDataBlock->recoilSequence[s],0);
            mShapeInstance->setTimeScale(mRecoilThread,0);
         }

   // Initialize the primary thread, the actual sequence is
   // set later depending on player actions.
   mActionAnimation.action = PlayerData::NullAnimation;
   mActionAnimation.thread = mShapeInstance->addThread();
   updateAnimationTree(!isGhost());

   if ( isGhost() )
   {
      // Create the sounds ahead of time.  This reduces runtime
      // costs and makes the system easier to understand.

      SFX_DELETE( mMoveBubbleSound );
      SFX_DELETE( mWaterBreathSound );

      if ( mDataBlock->sound[PlayerData::MoveBubbles] )
         mMoveBubbleSound = SFX->createSource( mDataBlock->sound[PlayerData::MoveBubbles] );

      if ( mDataBlock->sound[PlayerData::WaterBreath] )
         mWaterBreathSound = SFX->createSource( mDataBlock->sound[PlayerData::WaterBreath] );
   }

   mObjBox.maxExtents.x = mDataBlock->boxSize.x * 0.5f;
   mObjBox.maxExtents.y = mDataBlock->boxSize.y * 0.5f;
   mObjBox.maxExtents.z = mDataBlock->boxSize.z;
   mObjBox.minExtents.x = -mObjBox.maxExtents.x;
   mObjBox.minExtents.y = -mObjBox.maxExtents.y;
   mObjBox.minExtents.z = 0.0f;

   // Setup the box for our convex object...
   mObjBox.getCenter(&mConvex.mCenter);
   mConvex.mSize.x = mObjBox.len_x() / 2.0f;
   mConvex.mSize.y = mObjBox.len_y() / 2.0f;
   mConvex.mSize.z = mObjBox.len_z() / 2.0f;

   // Initialize our scaled attributes as well
   onScaleChanged();
   resetWorldBox();

   scriptOnNewDataBlock();
   return true;
}


//----------------------------------------------------------------------------

void Player::setControllingClient(GameConnection* client)
{
   Parent::setControllingClient(client);
   if (mControlObject)
      mControlObject->setControllingClient(client);
}

void Player::setControlObject(ShapeBase* obj)
{
   if (mControlObject) {
      mControlObject->setControllingObject(0);
      mControlObject->setControllingClient(0);
   }
   if (obj == this || obj == 0)
      mControlObject = 0;
   else {
      if (ShapeBase* coo = obj->getControllingObject())
         coo->setControlObject(0);
      if (GameConnection* con = obj->getControllingClient())
         con->setControlObject(0);

      mControlObject = obj;
      mControlObject->setControllingObject(this);
      mControlObject->setControllingClient(getControllingClient());
   }
}

void Player::onCameraScopeQuery(NetConnection *connection, CameraScopeQuery *query)
{
   // First, we are certainly in scope, and whatever we're riding is too...
   if(mControlObject.isNull() || mControlObject == mMount.object)
      Parent::onCameraScopeQuery(connection, query);
   else
   {
      connection->objectInScope(this);
      if (isMounted())
         connection->objectInScope(mMount.object);
      mControlObject->onCameraScopeQuery(connection, query);
   }
}

ShapeBase* Player::getControlObject()
{
   return mControlObject;
}

void Player::disableCollision()
{
   Parent::disableCollision();

   if ( mPhysicsPlayer )
      mPhysicsPlayer->disableCollision();
}

void Player::enableCollision()
{
   Parent::enableCollision();

   if ( mPhysicsPlayer )
      mPhysicsPlayer->enableCollision();
}

void Player::processTick(const Move* move)
{
   PROFILE_SCOPE(Player_ProcessTick);

   // If we're not being controlled by a client, let the
   // AI sub-module get a chance at producing a move.
   Move aiMove;
   if (!move && isServerObject() && getAIMove(&aiMove))
      move = &aiMove;

   // Manage the control object and filter moves for the player
   Move pMove,cMove;
   if (mControlObject) {
      if (!move)
         mControlObject->processTick(0);
      else {
         pMove = NullMove;
         cMove = *move;
         if (isMounted()) {
            // Filter Jump trigger if mounted
            pMove.trigger[2] = move->trigger[2];
            cMove.trigger[2] = false;
         }
         if (move->freeLook) {
            // Filter yaw/picth/roll when freelooking.
            pMove.yaw = move->yaw;
            pMove.pitch = move->pitch;
            pMove.roll = move->roll;
            pMove.freeLook = true;
            cMove.freeLook = false;
            cMove.yaw = cMove.pitch = cMove.roll = 0.0f;
         }
         mControlObject->processTick((mDamageState == Enabled)? &cMove: &NullMove);
         move = &pMove;
      }
   }

   Parent::processTick(move);
   // Warp to catch up to server
   if (delta.warpTicks > 0) {
      delta.warpTicks--;

      // Set new pos.
      getTransform().getColumn(3,&delta.pos);
      delta.pos += delta.warpOffset;
      delta.rot += delta.rotOffset;
      setPosition(delta.pos,delta.rot);
      setRenderPosition(delta.pos,delta.rot);
      updateDeathOffsets();
      updateLookAnimation();

      // Backstepping
      delta.posVec.x = -delta.warpOffset.x;
      delta.posVec.y = -delta.warpOffset.y;
      delta.posVec.z = -delta.warpOffset.z;
      delta.rotVec.x = -delta.rotOffset.x;
      delta.rotVec.y = -delta.rotOffset.y;
      delta.rotVec.z = -delta.rotOffset.z;
   }
   else {
      // If there is no move, the player is either an
      // unattached player on the server, or a player's
      // client ghost.
      if (!move) {
         if (isGhost()) {
            // If we haven't run out of prediction time,
            // predict using the last known move.
            if (mPredictionCount-- <= 0)
               return;

            move = &delta.move;
         }
         else
            move = &NullMove;
      }
      if (!isGhost())
         updateAnimation(TickSec);

      PROFILE_START(Player_PhysicsSection);
      if(isServerObject() || (didRenderLastRender() || getControllingClient()))
      {
         if ( !mPhysicsPlayer )
            updateWorkingCollisionSet();

         updateState();
         updateMove(move);
         updateLookAnimation();
         updateDeathOffsets();
         updatePos();
      }
      PROFILE_END();

      if (!isGhost())
      {
         // Animations are advanced based on frame rate on the
         // client and must be ticked on the server.
         updateActionThread();
         updateAnimationTree(true);
      }
   }
}

void Player::interpolateTick(F32 dt)
{
   if (mControlObject)
      mControlObject->interpolateTick(dt);

   // Client side interpolation
   Parent::interpolateTick(dt);

   Point3F pos = delta.pos + delta.posVec * dt;
   Point3F rot = delta.rot + delta.rotVec * dt;

   setRenderPosition(pos,rot,dt);

   // apply camera effects - is this the best place? - bramage
   GameConnection* connection = GameConnection::getConnectionToServer();
   if( connection->isFirstPerson() )
   {
      ShapeBase *obj = dynamic_cast<ShapeBase*>(connection->getControlObject());
      if( obj == this )
      {
         MatrixF curTrans = getRenderTransform();
         curTrans.mul( gCamFXMgr.getTrans() );
         Parent::setRenderTransform( curTrans );
      }
   }

   updateLookAnimation(dt);
   delta.dt = dt;
}

void Player::advanceTime(F32 dt)
{
   // Client side animations
   Parent::advanceTime(dt);
   updateActionThread();
   updateAnimation(dt);
   updateSplash();
   updateFroth(dt);
   updateWaterSounds(dt);

   mLastPos = getPosition();

   if (mImpactSound)
      playImpactSound();

   // update camera effects.  Definitely need to find better place for this - bramage
   if( isControlObject() )
   {
      if( mDamageState == Disabled || mDamageState == Destroyed )
      {
         // clear out all camera effects being applied to player if dead
         gCamFXMgr.clear();
      }
      gCamFXMgr.update( dt );
   }
}

bool Player::getAIMove(Move* move)
{
   return false;
}

void Player::setState(ActionState state, U32 recoverTicks)
{
   if (state != mState) {
      // Skip initialization if there is no manager, the state
      // will get reset when the object is added to a manager.
      if (isProperlyAdded()) {
         switch (state) {
            case RecoverState: {
               mRecoverTicks = recoverTicks;
               mReversePending = U32(F32(mRecoverTicks) / sLandReverseScale);
               setActionThread(PlayerData::LandAnim, true, false, true, true);
               break;
            }
         }
      }

      mState = state;
   }
}

void Player::updateState()
{
   switch (mState)
   {
      case RecoverState:
         if (mRecoverTicks-- <= 0)
         {
            if (mReversePending && mActionAnimation.action != PlayerData::NullAnimation)
            {
               // this serves and counter, and direction state
               mRecoverTicks = mReversePending;
               mActionAnimation.forward = false;

               S32 seq = mDataBlock->actionList[mActionAnimation.action].sequence;
               F32 pos = mShapeInstance->getPos(mActionAnimation.thread);

               mShapeInstance->setTimeScale(mActionAnimation.thread, -sLandReverseScale);
               mShapeInstance->transitionToSequence(mActionAnimation.thread,
                                                    seq, pos, sAnimationTransitionTime, true);
               mReversePending = 0;
            }
            else
            {
               setState(MoveState);
            }
         }        // Stand back up slowly only if not moving much-
         else if (!mReversePending && mVelocity.lenSquared() > sSlowStandThreshSquared)
         {
            mActionAnimation.waitForEnd = false;
            setState(MoveState);
         }
         break;
   }
}

const char* Player::getStateName()
{
   if (mDamageState != Enabled)
      return "Dead";
   if (isMounted())
      return "Mounted";
   if (mState == RecoverState)
      return "Recover";
   return "Move";
}

void Player::getDamageLocation(const Point3F& in_rPos, const char *&out_rpVert, const char *&out_rpQuad)
{
   // TODO: This will be WRONG when player is prone!

   Point3F newPoint;
   mWorldToObj.mulP(in_rPos, &newPoint);

   Point3F boxSize = mObjBox.getExtents();
   F32 zHeight = boxSize.z;
   F32 zTorso  = mDataBlock->boxTorsoPercentage;
   F32 zHead   = mDataBlock->boxHeadPercentage;

   zTorso *= zHeight;
   zHead  *= zHeight;

   if (newPoint.z <= zTorso)
      out_rpVert = "legs";
   else if (newPoint.z <= zHead)
      out_rpVert = "torso";
   else
      out_rpVert = "head";

   if(dStrcmp(out_rpVert, "head") != 0)
   {
      if (newPoint.y >= 0.0f)
      {
         if (newPoint.x <= 0.0f)
            out_rpQuad = "front_left";
         else
            out_rpQuad = "front_right";
      }
      else
      {
         if (newPoint.x <= 0.0f)
            out_rpQuad = "back_left";
         else
            out_rpQuad = "back_right";
      }
   }
   else
   {
      F32 backToFront = boxSize.x;
      F32 leftToRight = boxSize.y;

      F32 backPoint  = backToFront * (mDataBlock->boxHeadBackPercentage  - 0.5f);
      F32 frontPoint = backToFront * (mDataBlock->boxHeadFrontPercentage - 0.5f);
      F32 leftPoint  = leftToRight * (mDataBlock->boxHeadLeftPercentage  - 0.5f);
      F32 rightPoint = leftToRight * (mDataBlock->boxHeadRightPercentage - 0.5f);

      S32 index = 0;
      if (newPoint.y < backPoint)
         index += 0;
      else if (newPoint.y <= frontPoint)
         index += 3;
      else
         index += 6;

      if (newPoint.x < leftPoint)
         index += 0;
      else if (newPoint.x <= rightPoint)
         index += 1;
      else
         index += 2;

      switch (index)
      {
         case 0:
         out_rpQuad = "left_back";
         break;

         case 1: out_rpQuad = "middle_back"; break;
         case 2: out_rpQuad = "right_back"; break;
         case 3: out_rpQuad = "left_middle";   break;
         case 4: out_rpQuad = "middle_middle"; break;
         case 5: out_rpQuad = "right_middle"; break;
         case 6: out_rpQuad = "left_front";   break;
         case 7: out_rpQuad = "middle_front"; break;
         case 8: out_rpQuad = "right_front"; break;

         default:
            AssertFatal(0, "Bad non-tant index");
      };
   }
}

void Player::setPose( Pose pose )
{
   // Already the set pose, return.
   if ( pose == mPose ) 
      return;

   mPose = pose;

   // Not added yet, just assign the pose and return.
   if ( !isProperlyAdded() )   
      return;
        
   Point3F boxSize(1,1,1);

   // Resize the player boxes
   switch (pose) 
   {
      case StandPose:
         boxSize = mDataBlock->boxSize;
         break;
      case CrouchPose:
         boxSize = mDataBlock->crouchBoxSize;         
         break;
      case PronePose:
         boxSize = mDataBlock->proneBoxSize;         
         break;
      case SwimPose:
         boxSize = mDataBlock->swimBoxSize;
         break;
   }

   // Object and World Boxes...
   mObjBox.maxExtents.x = boxSize.x * 0.5f;
   mObjBox.maxExtents.y = boxSize.y * 0.5f;
   mObjBox.maxExtents.z = boxSize.z;
   mObjBox.minExtents.x = -mObjBox.maxExtents.x;
   mObjBox.minExtents.y = -mObjBox.maxExtents.y;
   mObjBox.minExtents.z = 0.0f;

   resetWorldBox();

   // Setup the box for our convex object...
   mObjBox.getCenter(&mConvex.mCenter);
   mConvex.mSize.x = mObjBox.len_x() / 2.0f;
   mConvex.mSize.y = mObjBox.len_y() / 2.0f;
   mConvex.mSize.z = mObjBox.len_z() / 2.0f;

   // Initialize our scaled attributes as well...
   onScaleChanged();

   // Resize the PhysicsPlayer rep. should we have one
   if ( mPhysicsPlayer )
      mPhysicsPlayer->setSpacials( getPosition(), boxSize );

}

void Player::updateMove(const Move* move)
{
   delta.move = *move;

   // Is waterCoverage high enough to be 'swimming'?
   {
      //bool swimming = mWaterCoverage > 0.65f;
      bool swimming = false; //disabled

      if ( swimming != mSwimming )
      {
         if ( !isGhost() )
         {
            String buf = swimming ? "onStartSwim" : "onStopSwim";
            Con::executef( mDataBlock, buf, scriptThis() );
         }

         mSwimming = swimming;
      }
   }

   // Trigger images
   if (mDamageState == Enabled) {
      setImageTriggerState(0,move->trigger[0]);
      setImageTriggerState(1,move->trigger[1]);
   }

   // Update current orientation
   if (mDamageState == Enabled) {
      F32 prevZRot = mRot.z;
      delta.headVec = mHead;

      F32 p = move->pitch;
      if (p > M_PI_F) 
         p -= M_2PI_F;
      mHead.x = mClampF(mHead.x + p,mDataBlock->minLookAngle,
                        mDataBlock->maxLookAngle);

      F32 y = move->yaw;
      if (y > M_PI_F)
         y -= M_2PI_F;

      GameConnection* con = getControllingClient();
      if (move->freeLook && ((isMounted() && getMountNode() == 0) || (con && !con->isFirstPerson())))
      {
         mHead.z = mClampF(mHead.z + y,
                           -mDataBlock->maxFreelookAngle,
                           mDataBlock->maxFreelookAngle);
      }
      else
      {
         mRot.z += y;
         // Rotate the head back to the front, center horizontal
         // as well if we're controlling another object.
         mHead.z *= 0.5f;
         if (mControlObject)
            mHead.x *= 0.5f;
      }

      // constrain the range of mRot.z
      while (mRot.z < 0.0f)
         mRot.z += M_2PI_F;
      while (mRot.z > M_2PI_F)
         mRot.z -= M_2PI_F;

      delta.rot = mRot;
      delta.rotVec.x = delta.rotVec.y = 0.0f;
      delta.rotVec.z = prevZRot - mRot.z;
      if (delta.rotVec.z > M_PI_F)
         delta.rotVec.z -= M_2PI_F;
      else if (delta.rotVec.z < -M_PI_F)
         delta.rotVec.z += M_2PI_F;

      delta.head = mHead;
      delta.headVec -= mHead;
   }
   MatrixF zRot;
   zRot.set(EulerF(0.0f, 0.0f, mRot.z));

   // Desired move direction & speed
   VectorF moveVec;
   F32 moveSpeed;
   if (mState == MoveState && mDamageState == Enabled)
   {
      zRot.getColumn(0,&moveVec);
      moveVec *= move->x;
      VectorF tv;
      zRot.getColumn(1,&tv);
      moveVec += tv * move->y;

      // Clamp water movement
      if (move->y > 0.0f)
      {
         if ( mSwimming )
            moveSpeed = getMax(mDataBlock->maxUnderwaterForwardSpeed * move->y,
                               mDataBlock->maxUnderwaterSideSpeed * mFabs(move->x));
         else
            moveSpeed = getMax(mDataBlock->maxForwardSpeed * move->y,
                               mDataBlock->maxSideSpeed * mFabs(move->x));
      }
      else
      {
         if ( mSwimming )
            moveSpeed = getMax(mDataBlock->maxUnderwaterBackwardSpeed * mFabs(move->y),
                               mDataBlock->maxUnderwaterSideSpeed * mFabs(move->x));
         else
            moveSpeed = getMax(mDataBlock->maxBackwardSpeed * mFabs(move->y),
                               mDataBlock->maxSideSpeed * mFabs(move->x));
      }

      // Cancel any script driven animations if we are going to move.
      if (moveVec.x + moveVec.y + moveVec.z != 0.0f &&
          (mActionAnimation.action >= PlayerData::NumTableActionAnims
               || mActionAnimation.action == PlayerData::LandAnim))
         mActionAnimation.action = PlayerData::NullAnimation;
   }
   else
   {
      moveVec.set(0.0f, 0.0f, 0.0f);
      moveSpeed = 0.0f;
   }

   // Acceleration due to gravity
   VectorF acc(0.0f, 0.0f, mGravity * mGravityMod * TickSec);

   // Determine ground contact normal. Only look for contacts if
   // we can move - and we aren't swimming.
   VectorF contactNormal(0,0,0);
   bool jumpSurface = false, runSurface = false;
   if ( !isMounted() && !mSwimming )
      findContact( &runSurface, &jumpSurface, &contactNormal );
   if ( jumpSurface )
      mJumpSurfaceNormal = contactNormal;
   
   // If we don't have a runSurface but we do have a contactNormal,
   // then we are standing on something that is too steep.
   // Deflect the force of gravity by the normal so we slide.
   // We could also try aligning it to the runSurface instead,
   // but this seems to work well.
   if ( !runSurface && !contactNormal.isZero() )   
      acc = ( acc - 2 * contactNormal * mDot( acc, contactNormal ) );   

   // Acceleration on run surface
   if (runSurface) {
      mContactTimer = 0;

      // Remove acc into contact surface (should only be gravity)
      // Clear out floating point acc errors, this will allow
      // the player to "rest" on the ground.
      F32 vd = -mDot(acc,contactNormal);
      if (vd > 0.0f) {
         VectorF dv = contactNormal * (vd + 0.002f);
         acc += dv;
         if (acc.len() < 0.0001f)
            acc.set(0.0f, 0.0f, 0.0f);
      }

      // Force a 0 move if there is no energy, and only drain
      // move energy if we're moving.
      VectorF pv;
      if (mEnergy >= mDataBlock->minRunEnergy) {
         if (moveSpeed)
            mEnergy -= mDataBlock->runEnergyDrain;
         pv = moveVec;
      }
      else
         pv.set(0.0f, 0.0f, 0.0f);

      // Adjust the player's requested dir. to be parallel
      // to the contact surface.
      F32 pvl = pv.len();
      if(mJetting)
      {
         pvl = moveVec.len();
         if (pvl)
         {
            VectorF nn;
            mCross(pv,VectorF(0.0f, 0.0f, 0.0f),&nn);
            nn *= 1 / pvl;
            VectorF cv(0.0f, 0.0f, 0.0f);
            cv -= nn * mDot(nn,cv);
            pv -= cv * mDot(pv,cv);
            pvl = pv.len();
         }
      }
      else
      {
         if (pvl)
         {
            VectorF nn;
            mCross(pv,VectorF(0.0f, 0.0f, 1.0f),&nn);
            nn *= 1.0f / pvl;
            VectorF cv = contactNormal;
            cv -= nn * mDot(nn,cv);
            pv -= cv * mDot(pv,cv);
            pvl = pv.len();
         }
      }

      // Convert to acceleration
      if ( pvl )
         pv *= moveSpeed / pvl;
      VectorF runAcc = pv - (mVelocity + acc);
      F32 runSpeed = runAcc.len();

      // Clamp acceleration, player also accelerates faster when
      // in his hard landing recover state.
      F32 maxAcc = (mDataBlock->runForce / getMass()) * TickSec;
      if (mState == RecoverState)
         maxAcc *= mDataBlock->recoverRunForceScale;
      if (runSpeed > maxAcc)
         runAcc *= maxAcc / runSpeed;
      acc += runAcc;

      // If we are running on the ground, then we're not jumping
      if (mDataBlock->isJumpAction(mActionAnimation.action))
         mActionAnimation.action = PlayerData::NullAnimation;
   }
   else if (!mSwimming && mDataBlock->airControl > 0.0f)
   {
      VectorF pv;
      pv = moveVec;
      F32 pvl = pv.len();

      if (pvl)
         pv *= moveSpeed / pvl;

      VectorF runAcc = pv - acc;
      runAcc.z = 0;
      runAcc.x = runAcc.x * mDataBlock->airControl;
      runAcc.y = runAcc.y * mDataBlock->airControl;
      F32 runSpeed = runAcc.len();
      F32 maxAcc = (mDataBlock->runForce / getMass()) * TickSec * 0.3f;

      if (runSpeed > maxAcc)
         runAcc *= maxAcc / runSpeed;

      acc += runAcc;

      // There are no special air control animations 
      // so... increment this unless you really want to 
      // play the run anims in the air.
      mContactTimer++;
   }
   else if (mSwimming)
   {
      // get the head pitch and add it to the moveVec
      // This more accurate swim vector calc comes from Matt Fairfax
      MatrixF xRot, zRot;
      xRot.set(EulerF(mHead.x, 0, 0));
      zRot.set(EulerF(0, 0, mRot.z));
      MatrixF rot;
      rot.mul(zRot, xRot);
      rot.getColumn(0,&moveVec);

      moveVec *= move->x;
      VectorF tv;
      rot.getColumn(1,&tv);
      moveVec += tv * move->y;
      rot.getColumn(2,&tv);
      moveVec += tv * move->z;

      // Force a 0 move if there is no energy, and only drain
      // move energy if we're moving.
      VectorF swimVec;
      if (mEnergy >= mDataBlock->minRunEnergy) {
         if (moveSpeed)
            mEnergy -= mDataBlock->runEnergyDrain;
         swimVec = moveVec;
      }
      else
         swimVec.set(0,0,0);

      F32 swimVecLen = swimVec.len();

      // Convert to acceleration
      if ( swimVecLen )
         swimVec *= moveSpeed / swimVecLen;

      VectorF swimAcc = swimVec - mVelocity;

      F32 swimSpeed = swimAcc.len();

      // Clamp acceleration, player also accelerates faster when
      // in his hard landing recover state.
      F32 maxAcc = (mDataBlock->swimForce / getMass()) * TickSec;
      if ( swimSpeed > maxAcc )
         swimAcc *= maxAcc / swimSpeed;

      acc += swimAcc;

      mContactTimer++;
   }
   else
      mContactTimer++;   

   // Acceleration from Jumping
   if (move->trigger[2] && !isMounted() && canJump())
   {
      // Scale the jump impulse base on maxJumpSpeed
      F32 zSpeedScale = mVelocity.z;
      if (zSpeedScale <= mDataBlock->maxJumpSpeed)
      {
         zSpeedScale = (zSpeedScale <= mDataBlock->minJumpSpeed)? 1:
            1 - (zSpeedScale - mDataBlock->minJumpSpeed) /
            (mDataBlock->maxJumpSpeed - mDataBlock->minJumpSpeed);

         // Desired jump direction
         VectorF pv = moveVec;
         F32 len = pv.len();
         if (len > 0)
            pv *= 1 / len;

         // We want to scale the jump size by the player size, somewhat
         // in reduced ratio so a smaller player can jump higher in
         // proportion to his size, than a larger player.
         F32 scaleZ = (getScale().z * 0.25) + 0.75;

         // Calculate our jump impulse
         F32 impulse = mDataBlock->jumpForce / getMass();

         if (mDataBlock->jumpTowardsNormal)
         {
            // If we are facing into the surface jump up, otherwise
            // jump away from surface.
            F32 dot = mDot(pv,mJumpSurfaceNormal);
            if (dot <= 0)
               acc.z += mJumpSurfaceNormal.z * scaleZ * impulse * zSpeedScale;
            else
            {
               acc.x += pv.x * impulse * dot;
               acc.y += pv.y * impulse * dot;
               acc.z += mJumpSurfaceNormal.z * scaleZ * impulse * zSpeedScale;
            }
         }
         else
            acc.z += scaleZ * impulse * zSpeedScale;

         mJumpDelay = mDataBlock->jumpDelay;
         mEnergy -= mDataBlock->jumpEnergyDrain;

         // If we don't have a StandJumpAnim, just play the JumpAnim...
         S32 seq = (mVelocity.len() < 0.5) ? PlayerData::StandJumpAnim: PlayerData::JumpAnim; 
         if ( mDataBlock->actionList[seq].sequence == -1 )
            seq = PlayerData::JumpAnim;         
         setActionThread( seq, true, false, true );

         mJumpSurfaceLastContact = JumpSkipContactsMax;
      }
   }
   else
   {
      if (jumpSurface) 
      {
         if (mJumpDelay > 0)
            mJumpDelay--;
         mJumpSurfaceLastContact = 0;
      }
      else
         mJumpSurfaceLastContact++;
   }

   if (move->trigger[1] && !isMounted() && canJetJump())
   {
      mJetting = true;

      // Scale the jump impulse base on maxJumpSpeed
      F32 zSpeedScale = mVelocity.z;

      if (zSpeedScale <= mDataBlock->jetMaxJumpSpeed)
      {
         zSpeedScale = (zSpeedScale <= mDataBlock->jetMinJumpSpeed)? 1:
         1 - (zSpeedScale - mDataBlock->jetMinJumpSpeed) / (mDataBlock->jetMaxJumpSpeed - mDataBlock->jetMinJumpSpeed);

         // Desired jump direction
         VectorF pv = moveVec;
         F32 len = pv.len();

         if (len > 0.0f)
            pv *= 1 / len;

         // If we are facing into the surface jump up, otherwise
         // jump away from surface.
         F32 dot = mDot(pv,mJumpSurfaceNormal);
         F32 impulse = mDataBlock->jetJumpForce / getMass();

         if (dot <= 0)
            acc.z += mJumpSurfaceNormal.z * impulse * zSpeedScale;
         else
         {
            acc.x += pv.x * impulse * dot;
            acc.y += pv.y * impulse * dot;
            acc.z += mJumpSurfaceNormal.z * impulse * zSpeedScale;
         }

         mEnergy -= mDataBlock->jetJumpEnergyDrain;
      }
   }
   else
   {
      mJetting = false;
   }

   if (mJetting)
   {
      F32 newEnergy = mEnergy - mDataBlock->minJumpEnergy;

      if (newEnergy < 0)
      {
         newEnergy = 0;
         mJetting = false;
      }

      mEnergy = newEnergy;
   }

   // Add in force from physical zones...
   acc += (mAppliedForce / getMass()) * TickSec;

   // Adjust velocity with all the move & gravity acceleration
   // TG: I forgot why doesn't the TickSec multiply happen here...
   mVelocity += acc;

   // apply horizontal air resistance

   F32 hvel = mSqrt(mVelocity.x * mVelocity.x + mVelocity.y * mVelocity.y);

   if(hvel > mDataBlock->horizResistSpeed)
   {
      F32 speedCap = hvel;
      if(speedCap > mDataBlock->horizMaxSpeed)
         speedCap = mDataBlock->horizMaxSpeed;
      speedCap -= mDataBlock->horizResistFactor * TickSec * (speedCap - mDataBlock->horizResistSpeed);
      F32 scale = speedCap / hvel;
      mVelocity.x *= scale;
      mVelocity.y *= scale;
   }
   if(mVelocity.z > mDataBlock->upResistSpeed)
   {
      if(mVelocity.z > mDataBlock->upMaxSpeed)
         mVelocity.z = mDataBlock->upMaxSpeed;
      mVelocity.z -= mDataBlock->upResistFactor * TickSec * (mVelocity.z - mDataBlock->upResistSpeed);
   }

   // Container buoyancy & drag
   if (mBuoyancy != 0)
   {     
      // Applying buoyancy when standing still causing some jitters-
      if (mBuoyancy > 1.0 || !mVelocity.isZero() || !runSurface)
      {
         // A little hackery to prevent oscillation
         // based on http://reinot.blogspot.com/2005/11/oh-yes-they-float-georgie-they-all.html

         F32 buoyancyForce = mBuoyancy * mGravity * mGravityMod * TickSec;
         F32 currHeight = getPosition().z;
         const F32 C = 2.0f;
         const F32 M = 0.1f;
         
         if ( currHeight + mVelocity.z * TickSec * C > mLiquidHeight )
            buoyancyForce *= M;
                  
         mVelocity.z -= buoyancyForce;
      }
   }
   mVelocity -= mVelocity * mDrag * TickSec;

   // If we are not touching anything and have sufficient -z vel,
   // we are falling.
   if (runSurface)
      mFalling = false;
   else
   {
      VectorF vel;
      mWorldToObj.mulV(mVelocity,&vel);
      mFalling = vel.z < sFallingThreshold;
   }
   
   // Vehicle Dismount   
   if ( !isGhost() && move->trigger[2] && isMounted())
      Con::executef(mDataBlock, "doDismount", scriptThis());

   // Enter/Leave Liquid
   if ( !mInWater && mWaterCoverage > 0.0f ) 
   {      
      mInWater = true;

      if ( !isGhost() )
         Con::executef( mDataBlock, "onEnterLiquid", scriptThis(), Con::getFloatArg(mWaterCoverage), mLiquidType.c_str() );               
   }
   else if ( mInWater && mWaterCoverage <= 0.0f ) 
   {      
      mInWater = false;

      if ( !isGhost() )
         Con::executef( mDataBlock, "onLeaveLiquid", scriptThis(), mLiquidType.c_str() );      
      else
      {
         // exit-water splash sound happens for client only
         if ( getSpeed() >= mDataBlock->exitSplashSoundVel && !isMounted() )         
            SFX->playOnce( mDataBlock->sound[PlayerData::ExitWater], &getTransform() );                     
      }
   }

   // Update the PlayerPose
   Pose desiredPose = mPose;

   if ( mSwimming )
      desiredPose = SwimPose; 
   else if ( runSurface && move->trigger[3] && canCrouch() )     
      desiredPose = CrouchPose;
   else if ( runSurface && move->trigger[4] && canProne() )
      desiredPose = PronePose;
   else if ( canStand() )
      desiredPose = StandPose;

   setPose( desiredPose );
}


//----------------------------------------------------------------------------

bool Player::checkDismountPosition(const MatrixF& oldMat, const MatrixF& mat)
{
   AssertFatal(getContainer() != NULL, "Error, must have a container!");
   AssertFatal(getObjectMount() != NULL, "Error, must be mounted!");

   Point3F pos;
   Point3F oldPos;
   mat.getColumn(3, &pos);
   oldMat.getColumn(3, &oldPos);
   RayInfo info;
   disableCollision();
   getObjectMount()->disableCollision();
   if (getContainer()->castRay(oldPos, pos, sCollisionMoveMask, &info))
   {
      enableCollision();
      getObjectMount()->enableCollision();
      return false;
   }

   Box3F wBox = mObjBox;
   wBox.minExtents += pos;
   wBox.maxExtents += pos;

   EarlyOutPolyList polyList;
   polyList.mNormal.set(0.0f, 0.0f, 0.0f);
   polyList.mPlaneList.clear();
   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].set(wBox.minExtents,VectorF(-1.0f, 0.0f, 0.0f));
   polyList.mPlaneList[1].set(wBox.maxExtents,VectorF(0.0f, 1.0f, 0.0f));
   polyList.mPlaneList[2].set(wBox.maxExtents,VectorF(1.0f, 0.0f, 0.0f));
   polyList.mPlaneList[3].set(wBox.minExtents,VectorF(0.0f, -1.0f, 0.0f));
   polyList.mPlaneList[4].set(wBox.minExtents,VectorF(0.0f, 0.0f, -1.0f));
   polyList.mPlaneList[5].set(wBox.maxExtents,VectorF(0.0f, 0.0f, 1.0f));

   if (getContainer()->buildPolyList(wBox, sCollisionMoveMask, &polyList))
   {
      enableCollision();
      getObjectMount()->enableCollision();
      return false;
   }

   enableCollision();
   getObjectMount()->enableCollision();
   return true;
}


//----------------------------------------------------------------------------

bool Player::canJump()
{
   return mState == MoveState && mDamageState == Enabled && !isMounted() && !mJumpDelay && mEnergy >= mDataBlock->minJumpEnergy && mJumpSurfaceLastContact < JumpSkipContactsMax;
}

bool Player::canJetJump()
{
   return mState == MoveState && mDamageState == Enabled && !isMounted() && mEnergy >= mDataBlock->jetMinJumpEnergy && mDataBlock->jetJumpForce != 0.0f;
}

bool Player::canSwim()
{  
   // JCF: not used!
   //return mState == MoveState && mDamageState == Enabled && !isMounted() && mEnergy >= mDataBlock->minSwimEnergy && mWaterCoverage >= 0.8f;
   return true;
}

bool Player::canCrouch()
{  
   if ( mState != MoveState || 
        mDamageState != Enabled || 
        isMounted() || 
        mSwimming ||
        mFalling )
      return false;

   // Can't crouch if no crouch animation!
   if ( mDataBlock->actionList[PlayerData::CrouchRootAnim].sequence == -1 )
      return false;   
 
   // JCF: not testing for overlaps if there's no physics player!!
   // Do standard Torque physics test here!
   if ( !mPhysicsPlayer )
      return true;

	// We are already in this pose, so don't test it again...
	if ( mPose == CrouchPose )
		return true;

   return mPhysicsPlayer->testSpacials( getPosition(), mDataBlock->crouchBoxSize );
}

bool Player::canStand()
{   
   if ( mState != MoveState || 
        mDamageState != Enabled || 
        isMounted() || 
        mSwimming )
      return false;

   // Do standard Torque physics test here!
   if ( !mPhysicsPlayer )
      return true;

	// We are already in this pose, so don't test it again...
	if ( mPose == StandPose )
		return true;

   return mPhysicsPlayer->testSpacials( getPosition(), mDataBlock->boxSize );
}

bool Player::canProne()
{   
   if ( mState != MoveState || 
        mDamageState != Enabled || 
        isMounted() || 
        mSwimming ||
        mFalling )
      return false;

   // Can't go prone if no prone animation!
   if ( mDataBlock->actionList[PlayerData::ProneRootAnim].sequence == -1 )
      return false;

   // Do standard Torque physics test here!
   if ( !mPhysicsPlayer )
      return true;

	// We are already in this pose, so don't test it again...
	if ( mPose == PronePose )
		return true;

   return mPhysicsPlayer->testSpacials( getPosition(), mDataBlock->proneBoxSize );
}

//----------------------------------------------------------------------------

void Player::updateDamageLevel()
{
   if (!isGhost())
      setDamageState((mDamage >= mDataBlock->maxDamage)? Disabled: Enabled);
   if (mDamageThread)
      mShapeInstance->setPos(mDamageThread, mDamage / mDataBlock->destroyedLevel);
}

void Player::updateDamageState()
{
   // Become a corpse when we're disabled (dead).
   if (mDamageState == Enabled) {
      mTypeMask &= ~CorpseObjectType;
      mTypeMask |= PlayerObjectType;
   }
   else {
      mTypeMask &= ~PlayerObjectType;
      mTypeMask |= CorpseObjectType;
   }

   Parent::updateDamageState();
}


//----------------------------------------------------------------------------

void Player::updateLookAnimation(F32 dT)
{
   // Calculate our interpolated head position.
   Point3F renderHead = delta.head + delta.headVec * dT;

   // Adjust look pos.  This assumes that the animations match
   // the min and max look angles provided in the datablock.
   if (mArmAnimation.thread) 
   {
      // TG: Adjust arm position to avoid collision.
      F32 tp = mControlObject? 0.5:
         (renderHead.x - mArmRange.min) / mArmRange.delta;
      mShapeInstance->setPos(mArmAnimation.thread,mClampF(tp,0,1));
   }
   
   if (mHeadVThread) 
   {
      F32 tp = (renderHead.x - mHeadVRange.min) / mHeadVRange.delta;
      mShapeInstance->setPos(mHeadVThread,mClampF(tp,0,1));
   }
   
   if (mHeadHThread) 
   {
      F32 dt = 2 * mDataBlock->maxLookAngle;
      F32 tp = (renderHead.z + mDataBlock->maxLookAngle) / dt;
      mShapeInstance->setPos(mHeadHThread,mClampF(tp,0,1));
   }
}


//----------------------------------------------------------------------------
// Methods to get delta (as amount to affect velocity by)

bool Player::inDeathAnim()
{
   if (mActionAnimation.thread && mActionAnimation.action >= 0)
      if (mActionAnimation.action < mDataBlock->actionCount)
         return mDataBlock->actionList[mActionAnimation.action].death;

   return false;
}

// Get change from mLastDeathPos - return current pos.  Assumes we're in death anim.
F32 Player::deathDelta(Point3F & delta)
{
   // Get ground delta from the last time we offset this.
   MatrixF  mat;
   F32 pos = mShapeInstance->getPos(mActionAnimation.thread);
   mShapeInstance->deltaGround1(mActionAnimation.thread, mDeath.lastPos, pos, mat);
   mat.getColumn(3, & delta);
   return pos;
}

// Called before updatePos() to prepare it's needed change to velocity, which
// must roll over.  Should be updated on tick, this is where we remember last
// position of animation that was used to roll into velocity.
void Player::updateDeathOffsets()
{
   if (inDeathAnim())
      // Get ground delta from the last time we offset this.
      mDeath.lastPos = deathDelta(mDeath.posAdd);
   else
      mDeath.clear();
}


//----------------------------------------------------------------------------

static const U32 sPlayerConformMask =  InteriorObjectType|StaticShapeObjectType|
                                       StaticObjectType|TerrainObjectType;

static void accel(F32& from, F32 to, F32 rate)
{
   if (from < to)
      from = getMin(from += rate, to);
   else
      from = getMax(from -= rate, to);
}

// if (dt == -1)
//    normal tick, so we advance.
// else
//    interpolate with dt as % of tick, don't advance
//
MatrixF * Player::Death::fallToGround(F32 dt, const Point3F& loc, F32 curZ, F32 boxRad)
{
   static const F32 sConformCheckDown = 4.0f;
   RayInfo     coll;
   bool        conformToStairs = false;
   Point3F     pos(loc.x, loc.y, loc.z + 0.1f);
   Point3F     below(pos.x, pos.y, loc.z - sConformCheckDown);
   MatrixF  *  retVal = NULL;

   PROFILE_SCOPE(ConformToGround);

   if (gClientContainer.castRay(pos, below, sPlayerConformMask, &coll))
   {
      F32      adjust, height = (loc.z - coll.point.z), sink = curSink;
      VectorF  desNormal = coll.normal;
      VectorF  normal = curNormal;

      // dt >= 0 means we're interpolating and don't accel the numbers
      if (dt >= 0.0f)
         adjust = dt * TickSec;
      else
         adjust = TickSec;

      // Try to get them to conform to stairs by doing several LOS calls.  We do this if
      // normal is within about 5 deg. of vertical.
      if (desNormal.z > 0.995f)
      {
         Point3F  corners[3], downpts[3];
         S32      c;

         for (c = 0; c < 3; c++) {    // Build 3 corners to cast down from-
            corners[c].set(loc.x - boxRad, loc.y - boxRad, loc.z + 1.0f);
            if (c)      // add (0,boxWidth) and (boxWidth,0)
               corners[c][c - 1] += (boxRad * 2.0f);
            downpts[c].set(corners[c].x, corners[c].y, loc.z - sConformCheckDown);
         }

         // Do the three casts-
         for (c = 0; c < 3; c++)
            if (gClientContainer.castRay(corners[c], downpts[c], sPlayerConformMask, &coll))
               downpts[c] = coll.point;
            else
               break;

         // Do the math if everything hit below-
         if (c == 3) {
            mCross(downpts[1] - downpts[0], downpts[2] - downpts[1], &desNormal);
            AssertFatal(desNormal.z > 0, "Abnormality in Player::Death::fallToGround()");
            downpts[2] = downpts[2] - downpts[1];
            downpts[1] = downpts[1] - downpts[0];
            desNormal.normalize();
            conformToStairs = true;
         }
      }

      // Move normal in direction we want-
      F32   * cur = normal, * des = desNormal;
      for (S32 i = 0; i < 3; i++)
         accel(*cur++, *des++, adjust * 0.25f);

      if (mFabs(height) < 2.2f && !normal.isZero() && desNormal.z > 0.01f)
      {
         VectorF  upY(0.0f, 1.0f, 0.0f), ahead;
         VectorF  sideVec;
         MatrixF  mat(true);

         normal.normalize();
         mat.set(EulerF (0.0f, 0.0f, curZ));
         mat.mulV(upY, & ahead);
	      mCross(ahead, normal, &sideVec);
         sideVec.normalize();
         mCross(normal, sideVec, &ahead);

         static MatrixF resMat(true);
         resMat.setColumn(0, sideVec);
         resMat.setColumn(1, ahead);
         resMat.setColumn(2, normal);

         // Adjust Z down to account for box offset on slope.  Figure out how
         // much we want to sink, and gradually accel to this amount.  Don't do if
         // we're conforming to stairs though
         F32   xy = mSqrt(desNormal.x * desNormal.x + desNormal.y * desNormal.y);
         F32   desiredSink = (boxRad * xy / desNormal.z);
         if (conformToStairs)
            desiredSink *= 0.5f;

         accel(sink, desiredSink, adjust * 0.15f);

         Point3F  position(pos);
         position.z -= sink;
         resMat.setColumn(3, position);

         if (dt < 0.0f)
         {  // we're moving, so update normal and sink amount
            curNormal = normal;
            curSink = sink;
         }

         retVal = &resMat;
      }
   }
   return retVal;
}


//-------------------------------------------------------------------------------------

// This is called ::onAdd() to see if we're in a sitting animation.  These then
// can use a longer tick delay for the mount to get across.
bool Player::inSittingAnim()
{
   U32   action = mActionAnimation.action;
   if (mActionAnimation.thread && action < mDataBlock->actionCount) {
      const char * name = mDataBlock->actionList[action].name;
      if (!dStricmp(name, "Sitting") || !dStricmp(name, "Scoutroot"))
         return true;
   }
   return false;
}


//----------------------------------------------------------------------------

bool Player::setArmThread(const char* sequence)
{
   // The arm sequence must be in the action list.
   for (U32 i = 1; i < mDataBlock->actionCount; i++)
      if (!dStricmp(mDataBlock->actionList[i].name,sequence))
         return setArmThread(i);
   return false;
}

bool Player::setArmThread(U32 action)
{
   PlayerData::ActionAnimation &anim = mDataBlock->actionList[action];
   if (anim.sequence != -1 &&
      anim.sequence != mShapeInstance->getSequence(mArmAnimation.thread))
   {
      mShapeInstance->setSequence(mArmAnimation.thread,anim.sequence,0);
      mArmAnimation.action = action;
      setMaskBits(ActionMask);
      return true;
   }
   return false;
}


//----------------------------------------------------------------------------

bool Player::setActionThread(const char* sequence,bool hold,bool wait,bool fsp)
{
   for (U32 i = 1; i < mDataBlock->actionCount; i++)
   {
      PlayerData::ActionAnimation &anim = mDataBlock->actionList[i];
      if (!dStricmp(anim.name,sequence))
      {
         setActionThread(i,true,hold,wait,fsp);
         setMaskBits(ActionMask);
         return true;
      }
   }
   return false;
}

void Player::setActionThread(U32 action,bool forward,bool hold,bool wait,bool fsp, bool forceSet)
{
   if (mActionAnimation.action == action && !forceSet)
      return;

   if (action >= PlayerData::NumActionAnims)
   {
      Con::errorf("Player::setActionThread(%d): Player action out of range", action);
      return;
   }

   PlayerData::ActionAnimation &anim = mDataBlock->actionList[action];
   if (anim.sequence != -1)
   {
      mActionAnimation.action          = action;
      mActionAnimation.forward         = forward;
      mActionAnimation.firstPerson     = fsp;
      mActionAnimation.holdAtEnd       = hold;
      mActionAnimation.waitForEnd      = hold? true: wait;
      mActionAnimation.animateOnServer = fsp;
      mActionAnimation.atEnd           = false;
      mActionAnimation.delayTicks      = (S32)sNewAnimationTickTime;
      mActionAnimation.atEnd           = false;

      if (sUseAnimationTransitions && (isGhost()/* || mActionAnimation.animateOnServer*/))
      {
         // The transition code needs the timeScale to be set in the
         // right direction to know which way to go.
         F32   transTime = sAnimationTransitionTime;
         if (mDataBlock && mDataBlock->isJumpAction(action))
            transTime = 0.15f;

         F32 timeScale = mActionAnimation.forward ? 1.0f : -1.0f;
         if (mDataBlock && mDataBlock->isJumpAction(action))
            timeScale *= 1.5f;

         mShapeInstance->setTimeScale(mActionAnimation.thread,timeScale);
         mShapeInstance->transitionToSequence(mActionAnimation.thread,anim.sequence,
            mActionAnimation.forward ? 0.0f : 1.0f, transTime, true);
      }
      else
         mShapeInstance->setSequence(mActionAnimation.thread,anim.sequence,
            mActionAnimation.forward ? 0.0f : 1.0f);
   }
}

void Player::updateActionThread()
{
   PROFILE_START(UpdateActionThread);

   // Select an action animation sequence, this assumes that
   // this function is called once per tick.
   if(mActionAnimation.action != PlayerData::NullAnimation)
      if (mActionAnimation.forward)
         mActionAnimation.atEnd = mShapeInstance->getPos(mActionAnimation.thread) == 1;
      else
         mActionAnimation.atEnd = mShapeInstance->getPos(mActionAnimation.thread) == 0;

   // Only need to deal with triggers on the client
   if (isGhost())  {
      bool triggeredLeft = false;
      bool triggeredRight = false;
      F32 offset = 0.0f;
      if(mShapeInstance->getTriggerState(1)) {
         triggeredLeft = true;
         offset = -mDataBlock->decalOffset;
      }
      else if(mShapeInstance->getTriggerState(2)) {
         triggeredRight = true;
         offset = mDataBlock->decalOffset;
      }


      if (triggeredLeft || triggeredRight)
      {
         Point3F rot, pos;
         RayInfo rInfo;
         MatrixF mat = getRenderTransform();
         mat.getColumn(1, &rot);
         mat.mulP(Point3F(offset,0.0f,0.0f), &pos);
         if (gClientContainer.castRay(Point3F(pos.x, pos.y, pos.z + 0.01f),
            Point3F(pos.x, pos.y, pos.z - 2.0f ),
            STATIC_COLLISION_MASK | VehicleObjectType, &rInfo))
         {
            Material* material = ( rInfo.material ? dynamic_cast< Material* >( rInfo.material->getMaterial() ) : 0 );

            // Put footprints on surface, if appropriate for material.

            if( material && material->mShowFootprints
                && mDataBlock->decalData != 0 )
            {
                  mSceneManager->getCurrentDecalManager()->addDecal( rInfo.point, rot,
                     Point3F( rInfo.normal ), getScale(), mDataBlock->decalData );
            }
            
            // Emit footpuffs.

            if( rInfo.t <= 0.5 && mWaterCoverage == 0.0
                && material && material->mShowDust )
            {
               // New emitter every time for visibility reasons
               ParticleEmitter * emitter = new ParticleEmitter;
               emitter->onNewDataBlock( mDataBlock->footPuffEmitter );

               ColorF colorList[ ParticleData::PDC_NUM_KEYS];

               for( U32 x = 0; x < getMin( Material::NUM_EFFECT_COLOR_STAGES, ParticleData::PDC_NUM_KEYS ); ++ x )
                  colorList[ x ].set( material->mEffectColor[ x ].red,
                                      material->mEffectColor[ x ].green,
                                      material->mEffectColor[ x ].blue,
                                      material->mEffectColor[ x ].alpha );
               for( U32 x = Material::NUM_EFFECT_COLOR_STAGES; x < ParticleData::PDC_NUM_KEYS; ++ x )
                  colorList[ x ].set( 1.0, 1.0, 1.0, 0.0 );

               emitter->setColors( colorList );
               if( !emitter->registerObject() )
               {
                  Con::warnf( ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName() );
                  delete emitter;
                  emitter = NULL;
               }
               else
               {
                  emitter->emitParticles( pos, Point3F( 0.0, 0.0, 1.0 ), mDataBlock->footPuffRadius,
                     Point3F( 0, 0, 0 ), mDataBlock->footPuffNumParts );
                  emitter->deleteWhenEmpty();
               }
            }

            // Play footstep sound.

            MatrixF footMat = getTransform();
            if( mWaterCoverage > 0.0 )
            {
               // Treading water.

               if ( mWaterCoverage < mDataBlock->footSplashHeight )
                  SFX->playOnce( mDataBlock->sound[ PlayerData::FootShallowSplash ], &footMat );
               else
               {
                  if ( mWaterCoverage < 1.0 )
                     SFX->playOnce( mDataBlock->sound[ PlayerData::FootWading ], &footMat );
                  else
                  {
                     if ( triggeredLeft )
                     {
                        SFX->playOnce( mDataBlock->sound[ PlayerData::FootUnderWater ], &footMat );
                        SFX->playOnce( mDataBlock->sound[ PlayerData::FootBubbles ], &footMat );
                     }
                  }
               }
            }
            else if( material && material->mFootstepSoundCustom )
            {
               // Footstep sound defined on material.  This is the way it should be
               // done now.

               SFX->playOnce( material->mFootstepSoundCustom, &footMat );
            }
            else
            {
               // Play default sound.
   
               S32 sound = -1;
               if( material && material->mFootstepSoundId != -1 )
                  sound = material->mFootstepSoundId;
               else if( rInfo.object->getTypeMask() & VehicleObjectType )
                  sound = 2;

               switch ( sound )
               {
               case 0: // Soft
                  SFX->playOnce( mDataBlock->sound[PlayerData::FootSoft], &footMat );
                  break;
               case 1: // Hard
                  SFX->playOnce( mDataBlock->sound[PlayerData::FootHard], &footMat );
                  break;
               case 2: // Metal
                  SFX->playOnce( mDataBlock->sound[PlayerData::FootMetal], &footMat );
                  break;
               case 3: // Snow
                  SFX->playOnce( mDataBlock->sound[PlayerData::FootSnow], &footMat );
                  break;
                  /*
               default: //Hard
                  SFX->playOnce( mDataBlock->sound[PlayerData::FootHard], &footMat );
                  break;
                  */
               }
            }
         }
      }
   }

   // Mount pending variable puts a hold on the delayTicks below so players don't
   // inadvertently stand up because their mount has not come over yet.
   if (mMountPending)
      mMountPending = (isMounted() ? 0 : (mMountPending - 1));

   if (mActionAnimation.action == PlayerData::NullAnimation ||
       ((!mActionAnimation.waitForEnd || mActionAnimation.atEnd)) &&
       !mActionAnimation.holdAtEnd && (mActionAnimation.delayTicks -= !mMountPending) <= 0)
   {
      //The scripting language will get a call back when a script animation has finished...
      //  example: When the chat menu animations are done playing...
      if ( isServerObject() && mActionAnimation.action >= PlayerData::NumTableActionAnims )
         Con::executef(mDataBlock, "animationDone",scriptThis());
      pickActionAnimation();
   }

   if ( (mActionAnimation.action != PlayerData::LandAnim) &&
        (mActionAnimation.action != PlayerData::NullAnimation) )
   {
      // Update action animation time scale to match ground velocity
      PlayerData::ActionAnimation &anim =
         mDataBlock->actionList[mActionAnimation.action];
      F32 scale = 1;
      if (anim.velocityScale && anim.speed) {
         VectorF vel;
         mWorldToObj.mulV(mVelocity,&vel);
         scale = mFabs(mDot(vel, anim.dir) / anim.speed);

         if (scale > mDataBlock->maxTimeScale)
            scale = mDataBlock->maxTimeScale;
      }

      mShapeInstance->setTimeScale(mActionAnimation.thread,
                                   mActionAnimation.forward? scale: -scale);
   }
   PROFILE_END();
}

void Player::pickActionAnimation()
{
   // Only select animations in our normal move state.
   if (mState != MoveState || mDamageState != Enabled)
      return;

   if (isMounted())
   {
      // Go into root position unless something was set explicitly
      // from a script.
      if (mActionAnimation.action != PlayerData::RootAnim &&
          mActionAnimation.action < PlayerData::NumTableActionAnims)
         setActionThread(PlayerData::RootAnim,true,false,false);
      return;
   }

   bool forward = true;
   U32 action = PlayerData::RootAnim;
   bool fsp = false;
   
   // Jetting overrides the fall animation condition
   if (mJetting)
   {
      // Play the jetting animation
      action = PlayerData::JetAnim;
   }
   else if (mFalling)
   {
      // Not in contact with any surface and falling
      action = PlayerData::FallAnim;
   }
   
   //Climb Resource 
   else if (mClimbing)
   {
	   	 action = PlayerData::ClimbAnim;
	     if (mVelocity.z > 0)
			 forward = true;
		 else
			 forward = false;

   }
   //Climb Resource
   
   else if ( mSwimming )
   {
      action = PlayerData::SwimRootAnim;

      F32 curMax = 0.1f;
      VectorF vel;
      mWorldToObj.mulV(mVelocity,&vel);
      for (U32 i = PlayerData::SwimForwardAnim; i <= PlayerData::SwimRightAnim; i++)
      {
         PlayerData::ActionAnimation &anim = mDataBlock->actionList[i];
         if (anim.sequence != -1 && anim.speed) 
         {
            // JCF: bias towards picking the forward/backward anims over
            // the side to prevent oscillation between anims.  This seems to work
            // ok but the real fix is to have 8 way directional animations.
            VectorF biasedDir = anim.dir * VectorF(0.5f,1.0f,0.5f);
            F32 d = mDot(vel, biasedDir);
            if (d > curMax)
            {
               curMax = d;
               action = i;
               forward = true;
            }
         }
      }
   }
   else if ( mPose == StandPose )
   {
      if (mContactTimer >= sContactTickTime) {
         // Nothing under our feet
         action = PlayerData::RootAnim;
      }
      else
      {
         // Our feet are on something
         // Pick animation that is the best fit for our current velocity.
         // Assumes that root is the first animation in the list.
         F32 curMax = 0.1f;
         VectorF vel;
         mWorldToObj.mulV(mVelocity,&vel);
         for (U32 i = 1; i < PlayerData::NumMoveActionAnims; i++)
         {
            PlayerData::ActionAnimation &anim = mDataBlock->actionList[i];
            if (anim.sequence != -1 && anim.speed) 
            {
               // We bias towards picking the forward/backward anims over
               // the side to prevent oscillation between anims.  This seems 
               // to work ok but the real fix is to have 8 way directional 
               // animations.
               VectorF biasedDir = anim.dir * VectorF(0.5f,1.0f,0.5f);
               F32 d = mDot(vel, biasedDir);
               if (d > curMax)
               {
                  curMax = d;
                  action = i;
                  forward = true;
               }
               else
               {
                  // Special case, re-use slide left animation to slide right
                  if (i == PlayerData::SideLeftAnim && -d > curMax)
                  {
                     curMax = -d;
                     action = i;
                     forward = false;
                  }
               }
            }
         }
      }
   }
   else if ( mPose == CrouchPose )
   {
      VectorF vel;
      mWorldToObj.mulV(mVelocity,&vel);
      
      action = PlayerData::CrouchRootAnim;
      fsp = true;

      if ( vel.y > 0.1f )
      {
         action = PlayerData::CrouchForwardAnim;
         forward = true;
      }
      else if ( vel.y < -0.1f )
      {
         action = PlayerData::CrouchForwardAnim;
         forward = false;
      }            
   }
   else if ( mPose == PronePose )
   {
      VectorF vel;
      mWorldToObj.mulV(mVelocity,&vel);

      action = PlayerData::ProneRootAnim;
      fsp = true;

      if ( vel.y > 0.1f )
      {
         action = PlayerData::ProneForwardAnim;
         forward = true;
      }
      else if ( vel.y < -0.1f )
      {
         action = PlayerData::ProneForwardAnim;
         forward = false;
      } 
   }
   setActionThread(action,forward,false,false,fsp);
}

void Player::onImageRecoil(U32,ShapeBaseImageData::StateData::RecoilState)
{
   if (mRecoilThread)
   {
      mShapeInstance->setPos(mRecoilThread,0);
      mShapeInstance->setTimeScale(mRecoilThread,1);
   }
}

void Player::onUnmount(ShapeBase* obj,S32 node)
{
   // Reset back to root position during dismount.
   setActionThread(PlayerData::RootAnim,true,false,false);

   // Re-orient the player straight up
   Point3F pos,vec;
   getTransform().getColumn(1,&vec);
   getTransform().getColumn(3,&pos);
   Point3F rot(0.0f,0.0f,-mAtan(-vec.x,vec.y));
   setPosition(pos,rot);

   // Parent function will call script
   Parent::onUnmount(obj,node);
}


//----------------------------------------------------------------------------

void Player::updateAnimation(F32 dt)
{
   if ((isGhost() || mActionAnimation.animateOnServer) && mActionAnimation.thread)
      mShapeInstance->advanceTime(dt,mActionAnimation.thread);
   if (mRecoilThread)
      mShapeInstance->advanceTime(dt,mRecoilThread);

   // If we are the client's player on this machine, then we need
   // to make sure the transforms are up to date as they are used
   // to setup the camera.
   if (isGhost())
   {
      if (getControllingClient())
      {
         updateAnimationTree(isFirstPerson());
         mShapeInstance->animate();
      }
      else
      {
         updateAnimationTree(false);
      }
   }
}

void Player::updateAnimationTree(bool firstPerson)
{
   S32 mode = 0;
   if (firstPerson)
      if (mActionAnimation.firstPerson)
         mode = 0;
//            TSShapeInstance::MaskNodeRotation;
//            TSShapeInstance::MaskNodePosX |
//            TSShapeInstance::MaskNodePosY;
      else
         mode = TSShapeInstance::MaskNodeAllButBlend;

   for (U32 i = 0; i < PlayerData::NumSpineNodes; i++)
      if (mDataBlock->spineNode[i] != -1)
         mShapeInstance->setNodeAnimationState(mDataBlock->spineNode[i],mode);
}


//----------------------------------------------------------------------------

bool Player::step(Point3F *pos,F32 *maxStep,F32 time)
{
   const Point3F& scale = getScale();
   Box3F box;
   VectorF offset = mVelocity * time;
   box.minExtents = mObjBox.minExtents + offset + *pos;
   box.maxExtents = mObjBox.maxExtents + offset + *pos;
   box.maxExtents.z += mDataBlock->maxStepHeight * scale.z + sMinFaceDistance;

   SphereF sphere;
   sphere.center = (box.minExtents + box.maxExtents) * 0.5f;
   VectorF bv = box.maxExtents - sphere.center;
   sphere.radius = bv.len();

   ClippedPolyList polyList;
   polyList.mPlaneList.clear();
   polyList.mNormal.set(0.0f, 0.0f, 0.0f);
   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].set(box.minExtents,VectorF(-1.0f, 0.0f, 0.0f));
   polyList.mPlaneList[1].set(box.maxExtents,VectorF(0.0f, 1.0f, 0.0f));
   polyList.mPlaneList[2].set(box.maxExtents,VectorF(1.0f, 0.0f, 0.0f));
   polyList.mPlaneList[3].set(box.minExtents,VectorF(0.0f, -1.0f, 0.0f));
   polyList.mPlaneList[4].set(box.minExtents,VectorF(0.0f, 0.0f, -1.0f));
   polyList.mPlaneList[5].set(box.maxExtents,VectorF(0.0f, 0.0f, 1.0f));

   CollisionWorkingList& rList = mConvex.getWorkingList();
   CollisionWorkingList* pList = rList.wLink.mNext;
   while (pList != &rList) {
      Convex* pConvex = pList->mConvex;
      
      // Alright, here's the deal... a polysoup mesh really needs to be 
      // designed with stepping in mind.  If there are too many smallish polygons
      // the stepping system here gets confused and allows you to run up walls 
      // or on the edges/seams of meshes.

      TSStatic *st = dynamic_cast<TSStatic *> (pConvex->getObject());
      bool skip = false;
      if (st && !st->allowPlayerStep())
         skip = true;

      if ((pConvex->getObject()->getType() & StaticObjectType) != 0 && !skip)
      {
         Box3F convexBox = pConvex->getBoundingBox();
         if (box.isOverlapped(convexBox))
            pConvex->getPolyList(&polyList);
      }
      pList = pList->wLink.mNext;
   }

   // Find max step height
   F32 stepHeight = pos->z - sMinFaceDistance;
   U32* vp = polyList.mIndexList.begin();
   U32* ep = polyList.mIndexList.end();
   for (; vp != ep; vp++) {
      F32 h = polyList.mVertexList[*vp].point.z + sMinFaceDistance;
      if (h > stepHeight)
         stepHeight = h;
   }

   F32 step = stepHeight - pos->z;
   if (stepHeight > pos->z && step < *maxStep) {
      // Go ahead and step
      pos->z = stepHeight;
      *maxStep -= step;
      return true;
   }

   return false;
}


//----------------------------------------------------------------------------
inline Point3F createInterpPos(const Point3F& s, const Point3F& e, const F32 t, const F32 d)
{
   Point3F ret;
   ret.interpolate(s, e, t/d);
   return ret;
}

Point3F Player::_move( const F32 travelTime, Collision *outCol )
{
   // Try and move to new pos
   F32 totalMotion  = 0.0f;
   
   // TODO: not used?
   //F32 initialSpeed = mVelocity.len();

   Point3F start;
   Point3F initialPosition;
   getTransform().getColumn(3,&start);
   initialPosition = start;

   static CollisionList collisionList;
   static CollisionList physZoneCollisionList;

   collisionList.clear();
   physZoneCollisionList.clear();

   MatrixF collisionMatrix(true);
   collisionMatrix.setColumn(3, start);

   VectorF firstNormal;
   F32 maxStep = mDataBlock->maxStepHeight;
   F32 time = travelTime;
   U32 count = 0;

   const Point3F& scale = getScale();

   static Polyhedron sBoxPolyhedron;
   static ExtrudedPolyList sExtrudedPolyList;
   static ExtrudedPolyList sPhysZonePolyList;

   for (; count < sMoveRetryCount; count++) {
      F32 speed = mVelocity.len();
      if (!speed && !mDeath.haveVelocity())
         break;

      Point3F end = start + mVelocity * time;
      if (mDeath.haveVelocity()) {
         // Add in death movement-
         VectorF  deathVel = mDeath.getPosAdd();
         VectorF  resVel;
         getTransform().mulV(deathVel, & resVel);
         end += resVel;
      }
      Point3F distance = end - start;

      if (mFabs(distance.x) < mObjBox.len_x() &&
          mFabs(distance.y) < mObjBox.len_y() &&
          mFabs(distance.z) < mObjBox.len_z())
      {
         // We can potentially early out of this.  If there are no polys in the clipped polylist at our
         //  end position, then we can bail, and just set start = end;
         Box3F wBox = mScaledBox;
         wBox.minExtents += end;
         wBox.maxExtents += end;

         static EarlyOutPolyList eaPolyList;
         eaPolyList.clear();
         eaPolyList.mNormal.set(0.0f, 0.0f, 0.0f);
         eaPolyList.mPlaneList.clear();
         eaPolyList.mPlaneList.setSize(6);
         eaPolyList.mPlaneList[0].set(wBox.minExtents,VectorF(-1.0f, 0.0f, 0.0f));
         eaPolyList.mPlaneList[1].set(wBox.maxExtents,VectorF(0.0f, 1.0f, 0.0f));
         eaPolyList.mPlaneList[2].set(wBox.maxExtents,VectorF(1.0f, 0.0f, 0.0f));
         eaPolyList.mPlaneList[3].set(wBox.minExtents,VectorF(0.0f, -1.0f, 0.0f));
         eaPolyList.mPlaneList[4].set(wBox.minExtents,VectorF(0.0f, 0.0f, -1.0f));
         eaPolyList.mPlaneList[5].set(wBox.maxExtents,VectorF(0.0f, 0.0f, 1.0f));

         // Build list from convex states here...
         CollisionWorkingList& rList = mConvex.getWorkingList();
         CollisionWorkingList* pList = rList.wLink.mNext;
         while (pList != &rList) {
            Convex* pConvex = pList->mConvex;
            if (pConvex->getObject()->getTypeMask() & sCollisionMoveMask) {
               Box3F convexBox = pConvex->getBoundingBox();
               if (wBox.isOverlapped(convexBox))
               {
                  // No need to separate out the physical zones here, we want those
                  //  to cause a fallthrough as well...
                  pConvex->getPolyList(&eaPolyList);
               }
            }
            pList = pList->wLink.mNext;
         }

         if (eaPolyList.isEmpty())
         {
            totalMotion += (end - start).len();
            start = end;
            break;
         }
      }

      collisionMatrix.setColumn(3, start);
      sBoxPolyhedron.buildBox(collisionMatrix, mScaledBox);

      // Setup the bounding box for the extrudedPolyList
      Box3F plistBox = mScaledBox;
      collisionMatrix.mul(plistBox);
      Point3F oldMin = plistBox.minExtents;
      Point3F oldMax = plistBox.maxExtents;
      plistBox.minExtents.setMin(oldMin + (mVelocity * time) - Point3F(0.1f, 0.1f, 0.1f));
      plistBox.maxExtents.setMax(oldMax + (mVelocity * time) + Point3F(0.1f, 0.1f, 0.1f));

      // Build extruded polyList...
      VectorF vector = end - start;
      sExtrudedPolyList.extrude(sBoxPolyhedron,vector);
      sExtrudedPolyList.setVelocity(mVelocity);
      sExtrudedPolyList.setCollisionList(&collisionList);

      sPhysZonePolyList.extrude(sBoxPolyhedron,vector);
      sPhysZonePolyList.setVelocity(mVelocity);
      sPhysZonePolyList.setCollisionList(&physZoneCollisionList);

      // Build list from convex states here...
      CollisionWorkingList& rList = mConvex.getWorkingList();
      CollisionWorkingList* pList = rList.wLink.mNext;
      while (pList != &rList) {
         Convex* pConvex = pList->mConvex;
         if (pConvex->getObject()->getTypeMask() & sCollisionMoveMask) {
            Box3F convexBox = pConvex->getBoundingBox();
            if (plistBox.isOverlapped(convexBox))
            {
               if (pConvex->getObject()->getTypeMask() & PhysicalZoneObjectType)
                  pConvex->getPolyList(&sPhysZonePolyList);
               else
                  pConvex->getPolyList(&sExtrudedPolyList);
            }
         }
         pList = pList->wLink.mNext;
      }

      // Take into account any physical zones...
      for (U32 j = 0; j < physZoneCollisionList.getCount(); j++) 
      {
         AssertFatal(dynamic_cast<PhysicalZone*>(physZoneCollisionList[j].object), "Bad phys zone!");
         const PhysicalZone* pZone = (PhysicalZone*)physZoneCollisionList[j].object;
         if (pZone->isActive())
            mVelocity *= pZone->getVelocityMod();
      }

      if (collisionList.getCount() != 0 && collisionList.getTime() < 1.0f) 
      {
         // Set to collision point
         F32 velLen = mVelocity.len();

         F32 dt = time * getMin(collisionList.getTime(), 1.0f);
         start += mVelocity * dt;
         time -= dt;

         totalMotion += velLen * dt;

         mFalling = false;

         // Back off...
         if ( velLen > 0.f ) {
            F32 newT = getMin(0.01f / velLen, dt);
            start -= mVelocity * newT;
            totalMotion -= velLen * newT;
         }

         // Try stepping if there is a vertical surface
         if (collisionList.getMaxHeight() < start.z + mDataBlock->maxStepHeight * scale.z) 
         {
            bool stepped = false;
            for (U32 c = 0; c < collisionList.getCount(); c++) 
            {
               const Collision& cp = collisionList[c];
               // if (mFabs(mDot(cp.normal,VectorF(0,0,1))) < sVerticalStepDot)
               //    Dot with (0,0,1) just extracts Z component [lh]-
               if (mFabs(cp.normal.z) < sVerticalStepDot)
               {
                  stepped = step(&start,&maxStep,time);
                  break;
               }
            }
            if (stepped)
            {
               continue;
            }
         }

         // Pick the surface most parallel to the face that was hit.
         const Collision *collision = &collisionList[0];
         const Collision *cp = collision + 1;
         const Collision *ep = collision + collisionList.getCount();
         for (; cp != ep; cp++)
         {
            if (cp->faceDot > collision->faceDot)
               collision = cp;
         }

         // Copy this collision out so
         // we can use it to do impacts
         // and query collision.
         *outCol = *collision;

         F32 bd = -mDot( mVelocity, collision->normal );

         // Subtract out velocity
         VectorF dv = collision->normal * (bd + sNormalElasticity);
         mVelocity += dv;
         if (count == 0)
         {
            firstNormal = collision->normal;
         }
         else
         {
            if (count == 1)
            {
               // Re-orient velocity along the crease.
               if (mDot(dv,firstNormal) < 0.0f &&
                   mDot(collision->normal,firstNormal) < 0.0f)
               {
                  VectorF nv;
                  mCross(collision->normal,firstNormal,&nv);
                  F32 nvl = nv.len();
                  if (nvl)
                  {
                     if (mDot(nv,mVelocity) < 0.0f)
                        nvl = -nvl;
                     nv *= mVelocity.len() / nvl;
                     mVelocity = nv;
                  }
               }
            }
         }
      }
      else
      {
         totalMotion += (end - start).len();
         start = end;
         break;
      }
   }

   if (count == sMoveRetryCount)
   {
      // Failed to move
      start = initialPosition;
      mVelocity.set(0.0f, 0.0f, 0.0f);
   }

   return start;
}

void Player::_handleCollision( const Collision &collision )
{
   F32 bd = -mDot( mVelocity, collision.normal );

   // shake camera on ground impact
   if( bd > mDataBlock->groundImpactMinSpeed && isControlObject() )
   {
      F32 ampScale = (bd - mDataBlock->groundImpactMinSpeed) / mDataBlock->minImpactSpeed;

      CameraShake *groundImpactShake = new CameraShake;
      groundImpactShake->setDuration( mDataBlock->groundImpactShakeDuration );
      groundImpactShake->setFrequency( mDataBlock->groundImpactShakeFreq );

      VectorF shakeAmp = mDataBlock->groundImpactShakeAmp * ampScale;
      groundImpactShake->setAmplitude( shakeAmp );
      groundImpactShake->setFalloff( mDataBlock->groundImpactShakeFalloff );
      groundImpactShake->init();
      gCamFXMgr.addFX( groundImpactShake );
   }

   if ( bd > mDataBlock->minImpactSpeed && !mMountPending ) 
   {
      if ( !isGhost() )
         onImpact( collision.object, collision.normal * bd );

      if (mDamageState == Enabled && mState != RecoverState) 
      {
         // Scale how long we're down for
         F32   value = (bd - mDataBlock->minImpactSpeed);
         F32   range = (mDataBlock->minImpactSpeed * 0.9f);
         U32   recover = mDataBlock->recoverDelay;
         if (value < range)
            recover = 1 + S32(mFloor( F32(recover) * value / range) );
         //Con::printf("Used %d recover ticks", recover);
         //Con::printf("  minImpact = %g, this one = %g", mDataBlock->minImpactSpeed, bd);
         setState(RecoverState, recover);
      }
   }

   if ( isServerObject() && bd > (mDataBlock->minImpactSpeed / 3.0f) ) 
   {
      mImpactSound = PlayerData::ImpactNormal;
      setMaskBits(ImpactMask);
   }

   // Track collisions
   if (  !isGhost() && 
         collision.object && 
         collision.object != mContactInfo.contactObject )
      queueCollision( collision.object, mVelocity - collision.object->getVelocity() );
}

bool Player::updatePos(const F32 travelTime)
{
   PROFILE_SCOPE(Player_UpdatePos);
   getTransform().getColumn(3,&delta.posVec);

   // When mounted to another object, only Z rotation used.
   if (isMounted()) {
      mVelocity = mMount.object->getVelocity();
      setPosition(Point3F(0.0f, 0.0f, 0.0f), mRot);
      setMaskBits(MoveMask);
      return true;
   }

   Point3F newPos;

   Collision col;
   dMemset( &col, 0, sizeof( col ) );

   // JCFDEBUG:
   //Point3F savedVelocity = mVelocity;

   // JCF: this avoids unnecessary work, but also avoids problems with
   // the physicsPlayer jitters while stationary.
   if ( mVelocity.isZero() )
      newPos = delta.posVec;
   else
   {   
      if ( mPhysicsPlayer )
      {
         newPos = mPhysicsPlayer->move( mVelocity * travelTime, &col );
         mVelocity = ( newPos - delta.posVec ) / travelTime;
      }
      else
         newPos = _move( travelTime, &col );

      // JCFDEBUG:
      //if ( isClientObject() )
      //   Con::printf( "(client) vel: %g %g %g", mVelocity.x, mVelocity.y, mVelocity.z );
      //else
      //   Con::printf( "(server) vel: %g %g %g", mVelocity.x, mVelocity.y, mVelocity.z );
   }
   
   _handleCollision( col );   

   // Set new position
   // If on the client, calc delta for backstepping
   if (isClientObject())
   {
      delta.pos = newPos;
      delta.posVec = delta.posVec - delta.pos;
      delta.dt = 1.0f;
   }

   setPosition( newPos, mRot );
   setMaskBits( MoveMask );
   updateContainer();

   if (!isGhost())  
   {
      // Collisions are only queued on the server and can be
      // generated by either updateMove or updatePos
      notifyCollision();

      // Do mission area callbacks on the server as well
      checkMissionArea();
   }

   // Check the total distance moved.  If it is more than 1000th of the velocity, then
   //  we moved a fair amount...
   //if (totalMotion >= (0.001f * initialSpeed))
      return true;
   //else
      //return false;
}


//----------------------------------------------------------------------------
 
void Player::_findContact( SceneObject **contactObject, VectorF *contactNormal )
{
   Point3F pos;
   getTransform().getColumn(3,&pos);

   Box3F wBox;
   Point3F exp(0,0,sTractionDistance);
   wBox.minExtents = pos + mScaledBox.minExtents - exp;
   wBox.maxExtents.x = pos.x + mScaledBox.maxExtents.x;
   wBox.maxExtents.y = pos.y + mScaledBox.maxExtents.y;
   wBox.maxExtents.z = pos.z + mScaledBox.minExtents.z + sTractionDistance;

   static ClippedPolyList polyList;
   polyList.clear();
   polyList.doConstruct();
   polyList.mNormal.set(0.0f, 0.0f, 0.0f);
   polyList.setInterestNormal(Point3F(0.0f, 0.0f, -1.0f));

   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].setYZ(wBox.minExtents, -1.0f);
   polyList.mPlaneList[1].setXZ(wBox.maxExtents, 1.0f);
   polyList.mPlaneList[2].setYZ(wBox.maxExtents, 1.0f);
   polyList.mPlaneList[3].setXZ(wBox.minExtents, -1.0f);
   polyList.mPlaneList[4].setXY(wBox.minExtents, -1.0f);
   polyList.mPlaneList[5].setXY(wBox.maxExtents, 1.0f);
   Box3F plistBox = wBox;

   // Expand build box as it will be used to collide with items.
   // PickupRadius will be at least the size of the box.
   F32 pd = (F32)mDataBlock->pickupDelta;
   wBox.minExtents.x -= pd; wBox.minExtents.y -= pd;
   wBox.maxExtents.x += pd; wBox.maxExtents.y += pd;
   wBox.maxExtents.z = pos.z + mScaledBox.maxExtents.z;

   // Build list from convex states here...
   CollisionWorkingList& rList = mConvex.getWorkingList();
   CollisionWorkingList* pList = rList.wLink.mNext;
   U32 mask = isGhost() ? sClientCollisionContactMask : sServerCollisionContactMask;
   while (pList != &rList)
   {
      Convex* pConvex = pList->mConvex;

      U32 objectMask = pConvex->getObject()->getTypeMask();

      // Check: triggers, corpses and items...
      //
 
      //Climb Resource - Added this to check for ladders.
	  if(objectMask & ClimableItemObjectType)
	  {  
		  // Con::printf("Checking if we are facing item to climb");
		    
			MatrixF playerTransform = getRenderTransform();

			Point3F start, end, vec;
			
			playerTransform.getColumn(1,&vec); //get the forward vector
			playerTransform.getColumn(3,&start);  //get the position

			end = start + vec;  //set the end of the ray

		  RayInfo rInfo;
		  //see if we are facing the climable item that we just collided with
		  if(gClientContainer.castRay(start, end, objectMask & (ClimableItemObjectType | StaticShapeObjectType), &rInfo))
			{
				if(!mClimbing)
					Con::executef(mDataBlock,"onStartClimb",scriptThis(), Con::getIntArg(rInfo.object->getId()));
			mClimbing = true;						
		  }
		  else
		  {
			  if(mClimbing)
				  Con::executef(mDataBlock,"onEndClimb",scriptThis());
			  mClimbing = false;
		  }
	  }
	   //Climb Resource
	  
      if (objectMask & TriggerObjectType)
      {
         Trigger* pTrigger = static_cast<Trigger*>(pConvex->getObject());
         pTrigger->potentialEnterObject(this);
      }
      else if (objectMask & CorpseObjectType)
      {
         // If we've overlapped the worldbounding boxes, then that's it...
         if (getWorldBox().isOverlapped(pConvex->getObject()->getWorldBox()))
         {
            ShapeBase* col = static_cast<ShapeBase*>(pConvex->getObject());
            queueCollision(col,getVelocity() - col->getVelocity());
         }
      }
      else if (objectMask & ItemObjectType)
      {
         // If we've overlapped the worldbounding boxes, then that's it...
         Item* item = static_cast<Item*>(pConvex->getObject());
         if (getWorldBox().isOverlapped(item->getWorldBox()))
            if (this != item->getCollisionObject())
               queueCollision(item,getVelocity() - item->getVelocity());
      }
      else if ((objectMask & mask) && !(objectMask & PhysicalZoneObjectType))
      {
         Box3F convexBox = pConvex->getBoundingBox();
         if (plistBox.isOverlapped(convexBox))
            pConvex->getPolyList(&polyList);
      }

      pList = pList->wLink.mNext;
   }

   if (!polyList.isEmpty())
   {
      // Pick flattest surface
      F32 bestVd = -1.0f;
      ClippedPolyList::Poly* poly = polyList.mPolyList.begin();
      ClippedPolyList::Poly* end = polyList.mPolyList.end();
      for (; poly != end; poly++)
      {
         F32 vd = poly->plane.z;       // i.e.  mDot(Point3F(0,0,1), poly->plane);
         if (vd > bestVd)
         {
            bestVd = vd;
            *contactObject = poly->object;
            *contactNormal = poly->plane;
         }
      }
   }
}

void Player::findContact( bool *run, bool *jump, VectorF *contactNormal )
{
   SceneObject *contactObject = NULL;

   if ( mPhysicsPlayer )
      mPhysicsPlayer->findContact( &contactObject, contactNormal );
   else
      _findContact( &contactObject, contactNormal );
   
   F32 vd = (*contactNormal).z;
   *run = vd > mDataBlock->runSurfaceCos || mClimbing;  //Climb Resource -  make sure we can climb climable objects;
   *jump = vd > mDataBlock->jumpSurfaceCos;

   mContactInfo.clear();
  
   mContactInfo.contacted = contactObject != NULL;
   mContactInfo.contactObject = contactObject;

   if ( mContactInfo.contacted )
      mContactInfo.contactNormal = *contactNormal;

   mContactInfo.run = *run;
   mContactInfo.jump = *jump;
}

//----------------------------------------------------------------------------

void Player::checkMissionArea()
{
   // Checks to see if the player is in the Mission Area...
   Point3F pos;
   MissionArea * obj = dynamic_cast<MissionArea*>(Sim::findObject("GlobalMissionArea"));

   if(!obj)
      return;

   const RectI &area = obj->getArea();
   getTransform().getColumn(3, &pos);

   if ((pos.x < area.point.x || pos.x > area.point.x + area.extent.x ||
       pos.y < area.point.y || pos.y > area.point.y + area.extent.y)) {
      if(mInMissionArea) {
         mInMissionArea = false;
         Con::executef(mDataBlock, "onLeaveMissionArea",scriptThis());
      }
   }
   else if(!mInMissionArea)
   {
      mInMissionArea = true;
      Con::executef(mDataBlock, "onEnterMissionArea",scriptThis());
   }
}


//----------------------------------------------------------------------------

bool Player::isDisplacable() const
{
   return true;
}

Point3F Player::getMomentum() const
{
   return mVelocity * getMass();
}

void Player::setMomentum(const Point3F& newMomentum)
{
   Point3F newVelocity = newMomentum / getMass();
   mVelocity = newVelocity;
}

F32 Player::getMass() const
{
   if( mDataBlock )
      return mDataBlock->mass;

   // AFAICT the value of mMass never gets assigned from the player datablock. [5/24/2007 Pat]
   return mMass;
}

#define  LH_HACK   1
// Hack for short-term soln to Training crash -
#if   LH_HACK
static U32  sBalance;

bool Player::displaceObject(const Point3F& displacement)
{
   F32 vellen = mVelocity.len();
   if (vellen < 0.001f || sBalance > 16) {
      mVelocity.set(0.0f, 0.0f, 0.0f);
      return false;
   }

   F32 dt = displacement.len() / vellen;

   sBalance++;

   bool result = updatePos(dt);

   sBalance--;

   getTransform().getColumn(3, &delta.pos);
   delta.posVec.set(0.0f, 0.0f, 0.0f);

   return result;
}

#else

bool Player::displaceObject(const Point3F& displacement)
{
   F32 vellen = mVelocity.len();
   if (vellen < 0.001f) {
      mVelocity.set(0.0f, 0.0f, 0.0f);
      return false;
   }

   F32 dt = displacement.len() / vellen;

   bool result = updatePos(dt);

   mObjToWorld.getColumn(3, &delta.pos);
   delta.posVec.set(0.0f, 0.0f, 0.0f);

   return result;
}

#endif

//----------------------------------------------------------------------------

void Player::setPosition(const Point3F& pos,const Point3F& rot)
{
   MatrixF mat;
   if (isMounted()) {
      // Use transform from mounted object
      MatrixF nmat,zrot;
      mMount.object->getMountTransform(mMount.node,&nmat);
      zrot.set(EulerF(0.0f, 0.0f, rot.z));
      mat.mul(nmat,zrot);
   }
   else {
      mat.set(EulerF(0.0f, 0.0f, rot.z));
      mat.setColumn(3,pos);
   }
   Parent::setTransform(mat);
   mRot = rot;

   if ( mPhysicsPlayer )
      mPhysicsPlayer->setPosition( mat );
}


void Player::setRenderPosition(const Point3F& pos, const Point3F& rot, F32 dt)
{
   MatrixF mat;
   if (isMounted()) {
      // Use transform from mounted object
      MatrixF nmat,zrot;
      mMount.object->getRenderMountTransform(mMount.node,&nmat);
      zrot.set(EulerF(0.0f, 0.0f, rot.z));
      mat.mul(nmat,zrot);
   }
   else {
      EulerF   orient(0.0f, 0.0f, rot.z);

      mat.set(orient);
      mat.setColumn(3, pos);

      if (inDeathAnim()) {
         F32   boxRad = (mDataBlock->boxSize.x * 0.5f);
         if (MatrixF * fallMat = mDeath.fallToGround(dt, pos, rot.z, boxRad))
            mat = * fallMat;
      }
      else
         mDeath.initFall();
   }
   Parent::setRenderTransform(mat);
}

//----------------------------------------------------------------------------

void Player::setTransform(const MatrixF& mat)
{
   // This method should never be called on the client.

   // This currently converts all rotation in the mat into
   // rotations around the z axis.
   Point3F pos,vec;
   mat.getColumn(1,&vec);
   mat.getColumn(3,&pos);
   Point3F rot(0.0f, 0.0f, -mAtan(-vec.x,vec.y));
   setPosition(pos,rot);
   setMaskBits(MoveMask | NoWarpMask);
}

void Player::getEyeTransform(MatrixF* mat)
{
   // Eye transform in world space.  We only use the eye position
   // from the animation and supply our own rotation.
   MatrixF pmat,xmat,zmat;
   xmat.set(EulerF(mHead.x, 0.0f, 0.0f));
   zmat.set(EulerF(0.0f, 0.0f, mHead.z));
   pmat.mul(zmat,xmat);

   F32 *dp = pmat;

   F32* sp;
   MatrixF eyeMat(true);
   if (mDataBlock->eyeNode != -1)
   {
      sp = mShapeInstance->mNodeTransforms[mDataBlock->eyeNode];
   }
   else
   {
      Point3F center;
      mObjBox.getCenter(&center);
      eyeMat.setPosition(center);
      sp = eyeMat;
   }

   const Point3F& scale = getScale();
   dp[3] = sp[3] * scale.x;
   dp[7] = sp[7] * scale.y;
   dp[11] = sp[11] * scale.z;
   mat->mul(getTransform(),pmat);
}

void Player::getRenderEyeTransform(MatrixF* mat)
{
   // Eye transform in world space.  We only use the eye position
   // from the animation and supply our own rotation.
   MatrixF pmat,xmat,zmat;
   xmat.set(EulerF(delta.head.x + delta.headVec.x * delta.dt, 0.0f, 0.0f));
   zmat.set(EulerF(0.0f, 0.0f, delta.head.z + delta.headVec.z * delta.dt));
   pmat.mul(zmat,xmat);

   F32 *dp = pmat;

   F32* sp;
   MatrixF eyeMat(true);
   if (mDataBlock->eyeNode != -1)
   {
      sp = mShapeInstance->mNodeTransforms[mDataBlock->eyeNode];
   }
   else
   {
      Point3F center;
      mObjBox.getCenter(&center);
      eyeMat.setPosition(center);
      sp = eyeMat;
   }

   const Point3F& scale = getScale();
   dp[3] = sp[3] * scale.x;
   dp[7] = sp[7] * scale.y;
   dp[11] = sp[11] * scale.z;
   mat->mul(getRenderTransform(), pmat);
}

void Player::getMuzzleTransform(U32 imageSlot,MatrixF* mat)
{
   MatrixF nmat;
   Parent::getRetractionTransform(imageSlot,&nmat);
   MatrixF smat;
   Parent::getImageTransform(imageSlot,&smat);

   disableCollision();

   // See if we are pushed into a wall...
   if (getDamageState() == Enabled) {
      Point3F start, end;
      smat.getColumn(3, &start);
      nmat.getColumn(3, &end);

      RayInfo rinfo;
      if (getContainer()->castRay(start, end, sCollisionMoveMask, &rinfo)) {
         Point3F finalPoint;
         finalPoint.interpolate(start, end, rinfo.t);
         nmat.setColumn(3, finalPoint);
      }
      else
         Parent::getMuzzleTransform(imageSlot,&nmat);
   }
   else
      Parent::getMuzzleTransform(imageSlot,&nmat);

   enableCollision();

   // If we are in one of the standard player animations, adjust the
   // muzzle to point in the direction we are looking.
   if (mActionAnimation.action < PlayerData::NumTableActionAnims)
   {
      MatrixF xmat;
      xmat.set(EulerF(mHead.x, 0.0f, 0.0f));
      mat->mul(getTransform(),xmat);
      F32 *sp = nmat;
      F32 *dp = *mat;
      dp[3] = sp[3];
      dp[7] = sp[7];
      dp[11] = sp[11];
   }
   else
   {
      *mat = nmat;
   }
}


void Player::getRenderMuzzleTransform(U32 imageSlot,MatrixF* mat)
{
   MatrixF nmat;
   Parent::getRenderRetractionTransform(imageSlot,&nmat);
   MatrixF smat;
   Parent::getRenderImageTransform(imageSlot,&smat);

   disableCollision();

   // See if we are pushed into a wall...
   if (getDamageState() == Enabled)
   {
      Point3F start, end;
      smat.getColumn(3, &start);
      nmat.getColumn(3, &end);

      RayInfo rinfo;
      if (getContainer()->castRay(start, end, sCollisionMoveMask, &rinfo)) {
         Point3F finalPoint;
         finalPoint.interpolate(start, end, rinfo.t);
         nmat.setColumn(3, finalPoint);
      }
      else
      {
         Parent::getRenderMuzzleTransform(imageSlot,&nmat);
      }
   }
   else
   {
      Parent::getRenderMuzzleTransform(imageSlot,&nmat);
   }

   enableCollision();

   // If we are in one of the standard player animations, adjust the
   // muzzle to point in the direction we are looking.
   if (mActionAnimation.action < PlayerData::NumTableActionAnims) {
      MatrixF xmat;
      xmat.set(EulerF(mHead.x, 0.0f, 0.0f));
      mat->mul(getRenderTransform(),xmat);
      F32 *sp = nmat;
      F32 *dp = *mat;
      dp[3] = sp[3];
      dp[7] = sp[7];
      dp[11] = sp[11];
   }
   else
   {
      *mat = nmat;
   }
}


// Bot aiming code calls this frequently and will work fine without the check
// for being pushed into a wall, which shows up on profile at ~ 3% (eight bots)
void Player::getMuzzlePointAI(U32 imageSlot, Point3F* point)
{
   MatrixF nmat;
   Parent::getMuzzleTransform(imageSlot, &nmat);

   // If we are in one of the standard player animations, adjust the
   // muzzle to point in the direction we are looking.
   if (mActionAnimation.action < PlayerData::NumTableActionAnims)
   {
      MatrixF xmat;
      xmat.set(EulerF(mHead.x, 0, 0));
      MatrixF result;
      result.mul(getTransform(), xmat);
      F32 *sp = nmat, *dp = result;
      dp[3] = sp[3]; dp[7] = sp[7]; dp[11] = sp[11];
      result.getColumn(3, point);
   }
   else
      nmat.getColumn(3, point);
}

void Player::getCameraParameters(F32 *min,F32* max,Point3F* off,MatrixF* rot)
{
   if (!mControlObject.isNull() && mControlObject == getObjectMount()) {
      mControlObject->getCameraParameters(min,max,off,rot);
      return;
   }
   const Point3F& scale = getScale();
   *min = mDataBlock->cameraMinDist * scale.y;
   *max = mDataBlock->cameraMaxDist * scale.y;
   off->set(0.0f, 0.0f, 0.0f);
   rot->identity();
}


//----------------------------------------------------------------------------

Point3F Player::getVelocity() const
{
   return mVelocity;
}

F32 Player::getSpeed() const
{
   return mVelocity.len();
}

void Player::setVelocity(const VectorF& vel)
{
	AssertFatal( !mIsNaN( vel ), "Player::setVelocity() - The velocity is NaN!" );

   mVelocity = vel;
   setMaskBits(MoveMask);
}

void Player::applyImpulse(const Point3F&,const VectorF& vec)
{
	AssertFatal( !mIsNaN( vec ), "Player::applyImpulse() - The vector is NaN!" );

   // Players ignore angular velocity
   VectorF vel;
   vel.x = vec.x / getMass();
   vel.y = vec.y / getMass();
   vel.z = vec.z / getMass();
   setVelocity(mVelocity + vel);
}


//----------------------------------------------------------------------------

bool Player::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (getDamageState() != Enabled)
      return false;

   // Collide against bounding box. Need at least this for the editor.
   F32 st,et,fst = 0.0f,fet = 1.0f;
   F32 *bmin = &mObjBox.minExtents.x;
   F32 *bmax = &mObjBox.maxExtents.x;
   F32 const *si = &start.x;
   F32 const *ei = &end.x;

   for (int i = 0; i < 3; i++) {
      if (*si < *ei) {
         if (*si > *bmax || *ei < *bmin)
            return false;
         F32 di = *ei - *si;
         st = (*si < *bmin)? (*bmin - *si) / di: 0.0f;
         et = (*ei > *bmax)? (*bmax - *si) / di: 1.0f;
      }
      else {
         if (*ei > *bmax || *si < *bmin)
            return false;
         F32 di = *ei - *si;
         st = (*si > *bmax)? (*bmax - *si) / di: 0.0f;
         et = (*ei < *bmin)? (*bmin - *si) / di: 1.0f;
      }
      if (st > fst) fst = st;
      if (et < fet) fet = et;
      if (fet < fst)
         return false;
      bmin++; bmax++;
      si++; ei++;
   }

   info->normal = start - end;
   info->normal.normalizeSafe();
   getTransform().mulV( info->normal );

   info->t = fst;
   info->object = this;
   info->point.interpolate(start,end,fst);
   info->material = 0;
   return true;
}


//----------------------------------------------------------------------------

static MatrixF IMat(1);

bool Player::buildPolyList(AbstractPolyList* polyList, const Box3F&, const SphereF&)
{
   // Collision with the player is always against the player's object
   // space bounding box axis aligned in world space.
   Point3F pos;
   getTransform().getColumn(3,&pos);
   IMat.setColumn(3,pos);
   polyList->setTransform(&IMat, Point3F(1.0f,1.0f,1.0f));
   polyList->setObject(this);
   polyList->addBox(mObjBox);
   return true;
}


void Player::buildConvex(const Box3F& box, Convex* convex)
{
   if (mShapeInstance == NULL)
      return;

   // These should really come out of a pool
   mConvexList->collectGarbage();

   Box3F realBox = box;
   mWorldToObj.mul(realBox);
   realBox.minExtents.convolveInverse(mObjScale);
   realBox.maxExtents.convolveInverse(mObjScale);

   if (realBox.isOverlapped(getObjBox()) == false)
      return;

   Convex* cc = 0;
   CollisionWorkingList& wl = convex->getWorkingList();
   for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
      if (itr->mConvex->getType() == BoxConvexType &&
          itr->mConvex->getObject() == this) {
         cc = itr->mConvex;
         break;
      }
   }
   if (cc)
      return;

   // Create a new convex.
   BoxConvex* cp = new OrthoBoxConvex;
   mConvexList->registerObject(cp);
   convex->addToWorkingList(cp);
   cp->init(this);

   mObjBox.getCenter(&cp->mCenter);
   cp->mSize.x = mObjBox.len_x() / 2.0f;
   cp->mSize.y = mObjBox.len_y() / 2.0f;
   cp->mSize.z = mObjBox.len_z() / 2.0f;
}


//----------------------------------------------------------------------------

void Player::updateWorkingCollisionSet()
{
   // First, we need to adjust our velocity for possible acceleration.  It is assumed
   // that we will never accelerate more than 20 m/s for gravity, plus 10 m/s for
   // jetting, and an equivalent 10 m/s for jumping.  We also assume that the
   // working list is updated on a Tick basis, which means we only expand our
   // box by the possible movement in that tick.
   Point3F scaledVelocity = mVelocity * TickSec;
   F32 len    = scaledVelocity.len();
   F32 newLen = len + (10.0f * TickSec);

   // Check to see if it is actually necessary to construct the new working list,
   // or if we can use the cached version from the last query.  We use the x
   // component of the min member of the mWorkingQueryBox, which is lame, but
   // it works ok.
   bool updateSet = false;

   Box3F convexBox = mConvex.getBoundingBox(getTransform(), getScale());
   F32 l = (newLen * 1.1f) + 0.1f;  // from Convex::updateWorkingList
   const Point3F  lPoint( l, l, l );
   convexBox.minExtents -= lPoint;
   convexBox.maxExtents += lPoint;

   // Check containment
   if (mWorkingQueryBox.minExtents.x != -1e9f)
   {
      if (mWorkingQueryBox.isContained(convexBox) == false)
         // Needed region is outside the cached region.  Update it.
         updateSet = true;
   }
   else
   {
      // Must update
      updateSet = true;
   }
   // Actually perform the query, if necessary
   if (updateSet == true) {
      const Point3F  twolPoint( 2.0f * l, 2.0f * l, 2.0f * l );
      mWorkingQueryBox = convexBox;
      mWorkingQueryBox.minExtents -= twolPoint;
      mWorkingQueryBox.maxExtents += twolPoint;

      disableCollision();
      mConvex.updateWorkingList(mWorkingQueryBox,
         isGhost() ? sClientCollisionContactMask : sServerCollisionContactMask);
      enableCollision();
   }
}


//----------------------------------------------------------------------------

void Player::writePacketData(GameConnection *connection, BitStream *stream)
{
   Parent::writePacketData(connection, stream);

   stream->writeInt(mState,NumStateBits);
   if (stream->writeFlag(mState == RecoverState))
      stream->writeInt(mRecoverTicks,PlayerData::RecoverDelayBits);
   if (stream->writeFlag(mJumpDelay > 0))
      stream->writeInt(mJumpDelay,PlayerData::JumpDelayBits);

   Point3F pos;
   getTransform().getColumn(3,&pos);
   if (stream->writeFlag(!isMounted())) {
      // Will get position from mount
      stream->setCompressionPoint(pos);
      stream->write(pos.x);
      stream->write(pos.y);
      stream->write(pos.z);
      stream->write(mVelocity.x);
      stream->write(mVelocity.y);
      stream->write(mVelocity.z);
      stream->writeInt(mJumpSurfaceLastContact > 15 ? 15 : mJumpSurfaceLastContact, 4);
   }
   stream->write(mHead.x);
   stream->write(mHead.z);
   stream->write(mRot.z);

   if (mControlObject) {
      S32 gIndex = connection->getGhostIndex(mControlObject);
      if (stream->writeFlag(gIndex != -1)) {
         stream->writeInt(gIndex,NetConnection::GhostIdBitSize);
         mControlObject->writePacketData(connection, stream);
      }
   }
   else
      stream->writeFlag(false);
}


void Player::readPacketData(GameConnection *connection, BitStream *stream)
{
   Parent::readPacketData(connection, stream);

   mState = (ActionState)stream->readInt(NumStateBits);
   if (stream->readFlag())
      mRecoverTicks = stream->readInt(PlayerData::RecoverDelayBits);
   if (stream->readFlag())
      mJumpDelay = stream->readInt(PlayerData::JumpDelayBits);
   else
      mJumpDelay = 0;

   Point3F pos,rot;
   if (stream->readFlag()) {
      // Only written if we are not mounted
      stream->read(&pos.x);
      stream->read(&pos.y);
      stream->read(&pos.z);
      stream->read(&mVelocity.x);
      stream->read(&mVelocity.y);
      stream->read(&mVelocity.z);
      stream->setCompressionPoint(pos);
      delta.pos = pos;
      mJumpSurfaceLastContact = stream->readInt(4);
   }
   else
      pos = delta.pos;
   stream->read(&mHead.x);
   stream->read(&mHead.z);
   stream->read(&rot.z);
   rot.x = rot.y = 0;
   setPosition(pos,rot);
   delta.head = mHead;
   delta.rot = rot;

   if (stream->readFlag()) {
      S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);
      ShapeBase* obj = static_cast<ShapeBase*>(connection->resolveGhost(gIndex));
      setControlObject(obj);
      obj->readPacketData(connection, stream);
   }
   else
      setControlObject(0);
}

U32 Player::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag((mask & ImpactMask) && !(mask & InitialUpdateMask)))
      stream->writeInt(mImpactSound, PlayerData::ImpactBits);

   if (stream->writeFlag(mask & ActionMask &&
         mActionAnimation.action != PlayerData::NullAnimation &&
         mActionAnimation.action >= PlayerData::NumTableActionAnims)) {
      stream->writeInt(mActionAnimation.action,PlayerData::ActionAnimBits);
      stream->writeFlag(mActionAnimation.holdAtEnd);
      stream->writeFlag(mActionAnimation.atEnd);
      stream->writeFlag(mActionAnimation.firstPerson);
      if (!mActionAnimation.atEnd) {
         // If somewhere in middle on initial update, must send position-
         F32   where = mShapeInstance->getPos(mActionAnimation.thread);
         if (stream->writeFlag((mask & InitialUpdateMask) != 0 && where > 0))
            stream->writeSignedFloat(where, 6);
      }
   }

   if (stream->writeFlag(mask & ActionMask &&
         mArmAnimation.action != PlayerData::NullAnimation &&
         (!(mask & InitialUpdateMask) ||
         mArmAnimation.action != mDataBlock->lookAction))) {
      stream->writeInt(mArmAnimation.action,PlayerData::ActionAnimBits);
   }

   // The rest of the data is part of the control object packet update.
   // If we're controlled by this client, we don't need to send it.
   // we only need to send it if this is the initial update - in that case,
   // the client won't know this is the control object yet.
   if(stream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
      return(retMask);

   if (stream->writeFlag(mask & MoveMask))
   {
      stream->writeFlag(mFalling);
      
	  stream->writeFlag(mClimbing); //Climb Resource - update the clients

      stream->writeInt(mState,NumStateBits);
      if (stream->writeFlag(mState == RecoverState))
         stream->writeInt(mRecoverTicks,PlayerData::RecoverDelayBits);

      Point3F pos;
      getTransform().getColumn(3,&pos);
      stream->writeCompressedPoint(pos);
      F32 len = mVelocity.len();
      if(stream->writeFlag(len > 0.02f))
      {
         Point3F outVel = mVelocity;
         outVel *= 1.0f/len;
         stream->writeNormalVector(outVel, 10);
         len *= 32.0f;  // 5 bits of fraction
         if(len > 8191)
            len = 8191;
         stream->writeInt((S32)len, 13);
      }
      stream->writeFloat(mRot.z / M_2PI_F, 7);
      stream->writeSignedFloat(mHead.x / mDataBlock->maxLookAngle, 6);
      stream->writeSignedFloat(mHead.z / mDataBlock->maxLookAngle, 6);
      delta.move.pack(stream);
      stream->writeFlag(!(mask & NoWarpMask));
   }
   // Ghost need energy to predict reliably
   stream->writeFloat(getEnergyLevel() / mDataBlock->maxEnergy,EnergyLevelBits);
   return retMask;
}

void Player::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con,stream);

   if (stream->readFlag())
      mImpactSound = stream->readInt(PlayerData::ImpactBits);

   // Server specified action animation
   if (stream->readFlag()) {
      U32 action = stream->readInt(PlayerData::ActionAnimBits);
      bool hold = stream->readFlag();
      bool atEnd = stream->readFlag();
      bool fsp = stream->readFlag();

      F32   animPos = -1.0f;
      if (!atEnd && stream->readFlag())
         animPos = stream->readSignedFloat(6);

      if (isProperlyAdded()) {
         setActionThread(action,true,hold,true,fsp);
         bool  inDeath = inDeathAnim();
         if (atEnd)
         {
            mShapeInstance->clearTransition(mActionAnimation.thread);
            mShapeInstance->setPos(mActionAnimation.thread,
                                   mActionAnimation.forward? 1: 0);
            if (inDeath)
               mDeath.lastPos = 1.0f;
         }
         else if (animPos > 0) {
            mShapeInstance->setPos(mActionAnimation.thread, animPos);
            if (inDeath)
               mDeath.lastPos = animPos;
         }

         // mMountPending suppresses tickDelay countdown so players will sit until
         // their mount, or another animation, comes through (or 13 seconds elapses).
         mMountPending = (S32) (inSittingAnim() ? sMountPendingTickWait : 0);
      }
      else {
         mActionAnimation.action = action;
         mActionAnimation.holdAtEnd = hold;
         mActionAnimation.atEnd = atEnd;
         mActionAnimation.firstPerson = fsp;
      }
   }

   // Server specified arm animation
   if (stream->readFlag()) {
      U32 action = stream->readInt(PlayerData::ActionAnimBits);
      if (isProperlyAdded())
         setArmThread(action);
      else
         mArmAnimation.action = action;
   }

   // controlled by the client?
   if(stream->readFlag())
      return;

   if (stream->readFlag()) {
      mPredictionCount = sMaxPredictionTicks;
      mFalling = stream->readFlag();
	  mClimbing = stream->readFlag(); //Climb Resource

      ActionState actionState = (ActionState)stream->readInt(NumStateBits);
      if (stream->readFlag()) {
         mRecoverTicks = stream->readInt(PlayerData::RecoverDelayBits);
         setState(actionState, mRecoverTicks);
      }
      else
         setState(actionState);

      Point3F pos,rot;
      stream->readCompressedPoint(&pos);
      F32 speed = mVelocity.len();
      if(stream->readFlag())
      {
         stream->readNormalVector(&mVelocity, 10);
         mVelocity *= stream->readInt(13) / 32.0f;
      }
      else
      {
         mVelocity.set(0.0f, 0.0f, 0.0f);
      }
      
      rot.y = rot.x = 0.0f;
      rot.z = stream->readFloat(7) * M_2PI_F;
      mHead.x = stream->readSignedFloat(6) * mDataBlock->maxLookAngle;
      mHead.z = stream->readSignedFloat(6) * mDataBlock->maxLookAngle;
      delta.move.unpack(stream);

      delta.head = mHead;
      delta.headVec.set(0.0f, 0.0f, 0.0f);

      if (stream->readFlag() && isProperlyAdded())
      {
         // Determine number of ticks to warp based on the average
         // of the client and server velocities.
         delta.warpOffset = pos - delta.pos;
         F32 as = (speed + mVelocity.len()) * 0.5f * TickSec;
         F32 dt = (as > 0.00001f) ? delta.warpOffset.len() / as: sMaxWarpTicks;
         delta.warpTicks = (S32)((dt > sMinWarpTicks) ? getMax(mFloor(dt + 0.5f), 1.0f) : 0.0f);

         if (delta.warpTicks)
         {
            // Setup the warp to start on the next tick.
            if (delta.warpTicks > sMaxWarpTicks)
               delta.warpTicks = sMaxWarpTicks;
            delta.warpOffset /= (F32)delta.warpTicks;

            delta.rotOffset = rot - delta.rot;
            if(delta.rotOffset.z < - M_PI_F)
               delta.rotOffset.z += M_2PI_F;
            else if(delta.rotOffset.z > M_PI_F)
               delta.rotOffset.z -= M_2PI_F;
            delta.rotOffset /= (F32)delta.warpTicks;
         }
         else
         {
            // Going to skip the warp, server and client are real close.
            // Adjust the frame interpolation to move smoothly to the
            // new position within the current tick.
            Point3F cp = delta.pos + delta.posVec * delta.dt;
            if (delta.dt == 0) 
            {
               delta.posVec.set(0.0f, 0.0f, 0.0f);
               delta.rotVec.set(0.0f, 0.0f, 0.0f);
            }
            else
            {
               F32 dti = 1.0f / delta.dt;
               delta.posVec = (cp - pos) * dti;
               delta.rotVec.z = mRot.z - rot.z;

               if(delta.rotVec.z > M_PI_F)
                  delta.rotVec.z -= M_2PI_F;
               else if(delta.rotVec.z < -M_PI_F)
                  delta.rotVec.z += M_2PI_F;

               delta.rotVec.z *= dti;
            }
            delta.pos = pos;
            delta.rot = rot;
            setPosition(pos,rot);
         }
      }
      else 
      {
         // Set the player to the server position
         delta.pos = pos;
         delta.rot = rot;
         delta.posVec.set(0.0f, 0.0f, 0.0f);
         delta.rotVec.set(0.0f, 0.0f, 0.0f);
         delta.warpTicks = 0;
         delta.dt = 0.0f;
         setPosition(pos,rot);
      }
   }
   F32 energy = stream->readFloat(EnergyLevelBits) * mDataBlock->maxEnergy;
   setEnergyLevel(energy);
}


//----------------------------------------------------------------------------
ConsoleMethod( Player, getState, const char*, 2, 2, "Return the current state name.")
{
   return object->getStateName();
}

ConsoleMethod( Player, getDamageLocation, const char*, 3, 3, "(Point3F pos)")
{
   const char *buffer1;
   const char *buffer2;
   char *buff = Con::getReturnBuffer(128);

   Point3F pos(0.0f, 0.0f, 0.0f);
   dSscanf(argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z);
   object->getDamageLocation(pos, buffer1, buffer2);

   dSprintf(buff, 128, "%s %s", buffer1, buffer2);
   return buff;
}

ConsoleMethod( Player, setArmThread, bool, 3, 3, "(string sequenceName)")
{
   return object->setArmThread(argv[2]);
}

ConsoleMethod( Player, setActionThread, bool, 3, 5, "(string sequenceName, bool hold, bool fsp)")
{
   bool hold = (argc > 3)? dAtob(argv[3]): false;
   bool fsp  = (argc > 4)? dAtob(argv[4]): true;
   return object->setActionThread(argv[2],hold,true,fsp);
}

ConsoleMethod( Player, setControlObject, bool, 3, 3, "(ShapeBase obj)")
{
   ShapeBase* controlObject;
   if (Sim::findObject(argv[2],controlObject)) {
      object->setControlObject(controlObject);
      return true;
   }
   else
      object->setControlObject(0);
   return false;
}

ConsoleMethod( Player, getControlObject, S32, 2, 2, "Get the current control object.")
{
   ShapeBase* controlObject = object->getControlObject();
   return controlObject ? controlObject->getId(): 0;
}

ConsoleMethod( Player, clearControlObject, void, 2, 2, "")
{
   object->setControlObject(0);
}

ConsoleMethod( Player, checkDismountPoint, bool, 4, 4, "(Point3F oldPos, Point3F pos)")
{
   Point3F oldPos(0.0f, 0.0f, 0.0f);
   Point3F pos(0.0f, 0.0f, 0.0f);
   dSscanf(argv[2], "%g %g %g",
           &oldPos.x,
           &oldPos.y,
           &oldPos.z);
   dSscanf(argv[3], "%g %g %g",
           &pos.x,
           &pos.y,
           &pos.z);
   MatrixF oldPosMat(true);
   oldPosMat.setColumn(3, oldPos);
   MatrixF posMat(true);
   posMat.setColumn(3, pos);
   return object->checkDismountPosition(oldPosMat, posMat);
}

//----------------------------------------------------------------------------
void Player::consoleInit()
{
   Con::addVariable("pref::Player::renderMyPlayer",TypeBool, &sRenderMyPlayer);
   Con::addVariable("pref::Player::renderMyItems",TypeBool, &sRenderMyItems);

   Con::addVariable("Player::minWarpTicks",TypeF32,&sMinWarpTicks);
   Con::addVariable("Player::maxWarpTicks",TypeS32,&sMaxWarpTicks);
   Con::addVariable("Player::maxPredictionTicks",TypeS32,&sMaxPredictionTicks);
}

//--------------------------------------------------------------------------
void Player::calcClassRenderData()
{
   Parent::calcClassRenderData();

   disableCollision();
   MatrixF nmat;
   MatrixF smat;
   Parent::getRetractionTransform(0,&nmat);
   Parent::getImageTransform(0, &smat);

   // See if we are pushed into a wall...
   Point3F start, end;
   smat.getColumn(3, &start);
   nmat.getColumn(3, &end);

   RayInfo rinfo;
   if (getContainer()->castRay(start, end, 0xFFFFFFFF & ~(WaterObjectType|DefaultObjectType), &rinfo)) {
      if (rinfo.t < 1.0f)
         mWeaponBackFraction = 1.0f - rinfo.t;
      else
         mWeaponBackFraction = 0.0f;
   } else {
      mWeaponBackFraction = 0.0f;
   }
   enableCollision();
}


void Player::playFootstepSound(bool triggeredLeft, S32 sound)
{
   // [rene, 03/11/08 - more or less obsolete; footstep sounds should be
   //    defined as material property now]

   MatrixF footMat = getTransform();

   if ( mWaterCoverage == 0.0f )
   {
      switch ( sound )
      {
         case 0: // Soft
            SFX->playOnce( mDataBlock->sound[PlayerData::FootSoft], &footMat );
            break;
         case 1: // Hard
            SFX->playOnce( mDataBlock->sound[PlayerData::FootHard], &footMat );
            break;
         case 2: // Metal
            SFX->playOnce( mDataBlock->sound[PlayerData::FootMetal], &footMat );
         break;
         case 3: // Snow
            SFX->playOnce( mDataBlock->sound[PlayerData::FootSnow], &footMat );
            break;
         default: //Hard
            SFX->playOnce( mDataBlock->sound[PlayerData::FootHard], &footMat );
         break;
      }
   }
   else
   {
      if ( mWaterCoverage < mDataBlock->footSplashHeight )
         SFX->playOnce( mDataBlock->sound[PlayerData::FootShallowSplash], &footMat );
      else
      {
         if ( mWaterCoverage < 1.0 )
            SFX->playOnce( mDataBlock->sound[PlayerData::FootWading], &footMat );
         else
         {
            if ( triggeredLeft )
            {
               SFX->playOnce( mDataBlock->sound[PlayerData::FootUnderWater], &footMat );
               SFX->playOnce( mDataBlock->sound[PlayerData::FootBubbles], &footMat );
            }
         }
      }
   }
}

void Player:: playImpactSound()
{
   if( mWaterCoverage == 0.0f )
   {
      Point3F pos;
      RayInfo rInfo;
      MatrixF mat = getTransform();
      mat.mulP(Point3F(mDataBlock->decalOffset,0.0f,0.0f), &pos);

      if( gClientContainer.castRay( Point3F( pos.x, pos.y, pos.z + 0.01f ),
                                    Point3F( pos.x, pos.y, pos.z - 2.0f ),
                                    STATIC_COLLISION_MASK | VehicleObjectType,
                                    &rInfo ) )
      {
         Material* material = ( rInfo.material ? dynamic_cast< Material* >( rInfo.material->getMaterial() ) : 0 );

         if( material && material->mImpactSoundCustom )
            SFX->playOnce( material->mImpactSoundCustom, &getTransform() );
         else
         {
            S32 sound = -1;
            if( material && material->mImpactSoundId )
               sound = material->mImpactSoundId;
            else if( rInfo.object->getTypeMask() & VehicleObjectType )
               sound = 2; // Play metal;

            switch( sound )
            {
            case 0:
               //Soft
               SFX->playOnce( mDataBlock->sound[ PlayerData::ImpactSoft ], &getTransform() );
               break;
            case 1:
               //Hard
               SFX->playOnce( mDataBlock->sound[ PlayerData::ImpactHard ], &getTransform() );
               break;
            case 2:
               //Metal
               SFX->playOnce( mDataBlock->sound[ PlayerData::ImpactMetal ], &getTransform() );
               break;
            case 3:
               //Snow
               SFX->playOnce( mDataBlock->sound[ PlayerData::ImpactSnow ], &getTransform() );
               break;
               /*
            default:
               //Hard
               alxPlay(mDataBlock->sound[PlayerData::ImpactHard], &getTransform());
               break;
               */
            }
         }
      }
   }

   mImpactSound = 0;
}

//--------------------------------------------------------------------------
// Update splash
//--------------------------------------------------------------------------

void Player::updateSplash()
{
   F32 speed = getVelocity().len();
   if( speed < mDataBlock->splashVelocity || isMounted() ) return;

   Point3F curPos = getPosition();

   if ( curPos.equal( mLastPos ) )
      return;

   if (pointInWater( curPos )) {
      if (!pointInWater( mLastPos )) {
         Point3F norm = getVelocity();
         norm.normalize();

         // make sure player is moving vertically at good pace before playing splash
         F32 splashAng = mDataBlock->splashAngle / 360.0;
         if( mDot( norm, Point3F(0.0, 0.0, -1.0) ) < splashAng )
            return;


         RayInfo rInfo;
         if (gClientContainer.castRay(mLastPos, curPos,
               WaterObjectType, &rInfo)) {
            createSplash( rInfo.point, speed );
            mBubbleEmitterTime = 0.0;
         }

      }
   }
}


//--------------------------------------------------------------------------

void Player::updateFroth( F32 dt )
{
   // update bubbles
   Point3F moveDir = getVelocity();
   mBubbleEmitterTime += dt;

   if (mBubbleEmitterTime < mDataBlock->bubbleEmitTime) {
      if (mSplashEmitter[PlayerData::BUBBLE_EMITTER]) {
         Point3F emissionPoint = getRenderPosition();
         U32 emitNum = PlayerData::BUBBLE_EMITTER;
         mSplashEmitter[emitNum]->emitParticles(mLastPos, emissionPoint,
            Point3F( 0.0, 0.0, 1.0 ), moveDir, (U32)(dt * 1000.0));
      }
   }

   Point3F contactPoint;
   if (!collidingWithWater(contactPoint)) {
      mLastWaterPos = mLastPos;
      return;
   }

   F32 speed = moveDir.len();
   if( speed < mDataBlock->splashVelEpsilon ) speed = 0.0;
   U32 emitRate = (U32) (speed * mDataBlock->splashFreqMod * dt);

   U32 i;
   for ( i=0; i<PlayerData::BUBBLE_EMITTER; i++ ) {
      if (mSplashEmitter[i] )
         mSplashEmitter[i]->emitParticles( mLastWaterPos,
            contactPoint, Point3F( 0.0, 0.0, 1.0 ),
            moveDir, emitRate );
   }
   mLastWaterPos = contactPoint;
}

void Player::updateWaterSounds(F32 dt)
{
   if ( mWaterCoverage < 1.0f || mDamageState != Enabled )
   {
      // Stop everything
      if ( mMoveBubbleSound )
         mMoveBubbleSound->stop();
      if ( mWaterBreathSound )
         mWaterBreathSound->stop();
      return;
   }

   if ( mMoveBubbleSound )
   {
      // We're under water and still alive, so let's play something
      if ( mVelocity.len() > 1.0f )
      {
         if ( !mMoveBubbleSound->isPlaying() )
            mMoveBubbleSound->play();

         mMoveBubbleSound->setTransform( getTransform() );
      }
      else
         mMoveBubbleSound->stop();
   }

   if ( mWaterBreathSound )
   {
      if ( !mWaterBreathSound->isPlaying() )
         mWaterBreathSound->play();

      mWaterBreathSound->setTransform( getTransform() );
   }
}


//--------------------------------------------------------------------------
// Returns true if player is intersecting a water surface
//--------------------------------------------------------------------------
bool Player::collidingWithWater( Point3F &waterHeight )
{

   Point3F curPos = getPosition();

   F32 height = mFabs( mObjBox.maxExtents.z - mObjBox.minExtents.z );

   RayInfo rInfo;
   if( gClientContainer.castRay( curPos + Point3F(0.0, 0.0, height), curPos, WaterObjectType, &rInfo) )
   {
      WaterBlock* pBlock = dynamic_cast<WaterBlock*>(rInfo.object);

      if( !pBlock )
         return false;

      //if( !pBlock->isWater( pBlock->getLiquidType() ))
      //   return false;

      waterHeight = rInfo.point;
      return true;
   }


   return false;
}

//--------------------------------------------------------------------------
bool Player::pointInWater( Point3F &point )
{
   // TODO: WaterObject changes, fix this
   /*
   SimpleQueryList sql;
   if (isServerObject())
      gServerSceneGraph->getWaterObjectList(sql);
   else
      gClientSceneGraph->getWaterObjectList(sql);

   for (U32 i = 0; i < sql.mList.size(); i++)
   {
      WaterBlock* pBlock = dynamic_cast<WaterBlock*>(sql.mList[i]);

      if (pBlock)// && pBlock->getLiquidType() == WaterBlock::eWater )
      {
         if (pBlock->isUnderwater( point ))
            return true;
      }

   }
   */

   return false;
}

//--------------------------------------------------------------------------

void Player::createSplash( Point3F &pos, F32 speed )
{
   if ( speed >= mDataBlock->hardSplashSoundVel )
      SFX->playOnce( mDataBlock->sound[PlayerData::ImpactWaterHard], &getTransform() );
   else if ( speed >= mDataBlock->medSplashSoundVel )
      SFX->playOnce( mDataBlock->sound[PlayerData::ImpactWaterMedium], &getTransform() );
   else
      SFX->playOnce( mDataBlock->sound[PlayerData::ImpactWaterEasy], &getTransform() );

   if( mDataBlock->splash )
   {
      MatrixF trans = getTransform();
      trans.setPosition( pos );
      Splash *splash = new Splash;
      splash->onNewDataBlock( mDataBlock->splash );
      splash->setTransform( trans );
      splash->setInitialState( trans.getPosition(), Point3F( 0.0, 0.0, 1.0 ) );
      if (!splash->registerObject())
         delete splash;
   }
}


bool Player::isControlObject()
{
   GameConnection* connection = GameConnection::getConnectionToServer();
   if( !connection ) return false;
   ShapeBase *obj = dynamic_cast<ShapeBase*>(connection->getControlObject());
   return ( obj == this );
}


bool Player::prepRenderImage( SceneState* state, const U32 stateKey,
                              const U32 startZone, const bool modifyBaseState )
{
   bool renderPlayer = true;
   bool renderItems = true;

   /*
   if ( mPhysicsPlayer && Con::getBoolVariable("$PhysicsPlayer::DebugRender",false) )
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( mPhysicsPlayer, &PhysicsPlayer::renderDebug );
      ri->objectIndex = -1;
      ri->type = RenderPassManager::RIT_Object;
      state->getRenderPass()->addInst( ri );
   }
   */

   if ( Con::getBoolVariable("$PlayerConvex::DebugRender",false) )
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &Player::renderConvex );
      ri->objectIndex = -1;
      ri->type = RenderPassManager::RIT_Object;
      state->getRenderPass()->addInst( ri );
   }

   GameConnection* connection = GameConnection::getConnectionToServer();
   if( connection && connection->getControlObject() == this && connection->isFirstPerson() )
   {
      renderPlayer = mDataBlock->renderFirstPerson;

      if( !sRenderMyPlayer )
         renderPlayer = false;
      if( !sRenderMyItems )
         renderItems = false;
   }

   // Call the protected base class to do the work 
   // now that we know if we're rendering the player
   // and mounted shapes.
   return ShapeBase::_prepRenderImage( state, 
                                       stateKey, 
                                       startZone, 
                                       modifyBaseState, 
                                       renderPlayer, 
                                       renderItems );
}

void Player::renderConvex( ObjectRenderInst *ri, SceneState *state, BaseMatInstance *overrideMat )
{
   GFX->enterDebugEvent( ColorI(255,0,255), "Player_renderConvex" );
   mConvex.renderWorkingList();
   GFX->leaveDebugEvent();
}

