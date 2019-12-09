//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLAYER_H_
#define _PLAYER_H_

#ifndef _SHAPEBASE_H_
#include "T3D/shapeBase.h"
#endif
#ifndef _BOXCONVEX_H_
#include "collision/boxConvex.h"
#endif

#include "T3D/gameProcess.h"

class ParticleEmitter;
class ParticleEmitterData;
class DecalData;
class SplashData;
class PhysicsPlayer;

//----------------------------------------------------------------------------

struct PlayerData: public ShapeBaseData {
   typedef ShapeBaseData Parent;
   enum Constants {
      RecoverDelayBits = 7,
      JumpDelayBits = 7,
      NumSpineNodes = 6,
      ImpactBits = 3,
      NUM_SPLASH_EMITTERS = 3,
      BUBBLE_EMITTER = 2,
   };
   bool renderFirstPerson;    ///< Render the player shape in first person

   F32 pickupRadius;          ///< Radius around player for items (on server)
   F32 maxTimeScale;          ///< Max timeScale for action animations

   F32 minLookAngle;          ///< Lowest angle (radians) the player can look
   F32 maxLookAngle;          ///< Highest angle (radians) the player can look
   F32 maxFreelookAngle;      ///< Max left/right angle the player can look

   /// @name Physics constants
   /// @{

   F32 runForce;              ///< Force used to accelerate player
   F32 runEnergyDrain;        ///< Energy drain/tick
   F32 minRunEnergy;          ///< Minimum energy required to run
   F32 maxForwardSpeed;       ///< Maximum forward speed when running
   F32 maxBackwardSpeed;      ///< Maximum backward speed when running
   F32 maxSideSpeed;          ///< Maximum side speed when running

   F32 maxStepHeight;         ///< Maximum height the player can step up
   F32 runSurfaceAngle;       ///< Maximum angle from vertical in degrees the player can run up

   F32 horizMaxSpeed;         ///< Max speed attainable in the horizontal
   F32 horizResistSpeed;      ///< Speed at which resistance will take place
   F32 horizResistFactor;     ///< Factor of resistance once horizResistSpeed has been reached

   F32 upMaxSpeed;            ///< Max vertical speed attainable
   F32 upResistSpeed;         ///< Speed at which resistance will take place
   F32 upResistFactor;        ///< Factor of resistance once upResistSpeed has been reached

   S32 recoverDelay;          ///< # tick
   F32 recoverRunForceScale;  ///< RunForce multiplier in recover state

   // Jumping
   F32 jumpForce;             ///< Force exerted per jump
   F32 jumpEnergyDrain;       ///< Energy drained per jump
   F32 minJumpEnergy;         ///< Minimum energy required to jump
   F32 minJumpSpeed;          ///< Minimum speed needed to jump
   F32 maxJumpSpeed;          ///< Maximum speed before the player can no longer jump
   F32 jumpSurfaceAngle;      ///< Angle from vertical in degrees where the player can jump
   S32 jumpDelay;             ///< Delay time in ticks between jumps

   // Swimming
   F32 swimForce;                   ///< Force used to accelerate player while swimming
   F32 maxUnderwaterForwardSpeed;   ///< Maximum underwater forward speed when running
   F32 maxUnderwaterBackwardSpeed;  ///< Maximum underwater backward speed when running
   F32 maxUnderwaterSideSpeed;      ///< Maximum underwater side speed when running

   // Crouching
   F32 crouchForce;                 ///< Force used to accelerate player while crouching
   F32 maxCrouchForwardSpeed;       ///< Maximum forward speed when crouching
   F32 maxCrouchBackwardSpeed;      ///< Maximum backward speed when crouching
   F32 maxCrouchSideSpeed;          ///< Maximum side speed when crouching

   // Prone
   F32 proneForce;                  ///< Force used to accelerate player while prone
   F32 maxProneForwardSpeed;        ///< Maximum forward speed when prone
   F32 maxProneBackwardSpeed;       ///< Maximum backward speed when prone
   F32 maxProneSideSpeed;           ///< Maximum side speed when prone

   // Jetting
   F32 jetJumpForce;
   F32 jetJumpEnergyDrain;    ///< Energy per jump
   F32 jetMinJumpEnergy;
   F32 jetMinJumpSpeed;
   F32 jetMaxJumpSpeed;
   F32 jetJumpSurfaceAngle;   ///< Angle vertical degrees
   /// @}

   /// @name Hitboxes
   /// @{

   F32 boxHeadPercentage;
   F32 boxTorsoPercentage;

   S32 boxHeadLeftPercentage;
   S32 boxHeadRightPercentage;
   S32 boxHeadBackPercentage;
   S32 boxHeadFrontPercentage;
   /// @}

   F32 minImpactSpeed;        ///< Minimum impact speed required to apply fall damage

   F32 decalOffset;

   F32 groundImpactMinSpeed;      ///< Minimum impact speed required to apply fall damage with the ground
   VectorF groundImpactShakeFreq; ///< Frequency in each direction for the camera to shake
   VectorF groundImpactShakeAmp;  ///< How much to shake
   F32 groundImpactShakeDuration; ///< How long to shake
   F32 groundImpactShakeFalloff;  ///< How fast the shake disapates

   /// Zounds!
   enum Sounds {
      FootSoft,
      FootHard,
      FootMetal,
      FootSnow,
      FootShallowSplash,
      FootWading,
      FootUnderWater,
      FootBubbles,
      MoveBubbles,
      WaterBreath,
      ImpactSoft,
      ImpactHard,
      ImpactMetal,
      ImpactSnow,
      ImpactWaterEasy,
      ImpactWaterMedium,
      ImpactWaterHard,
      ExitWater,
      MaxSounds
   };
   SFXProfile* sound[MaxSounds];

   Point3F boxSize;           ///< Width, depth, height
   Point3F crouchBoxSize;
   Point3F proneBoxSize;
   Point3F swimBoxSize;

   /// Animation and other data initialized in onAdd
   struct ActionAnimationDef {
      const char* name;       ///< Sequence name
      struct Vector {
         F32 x,y,z;
      } dir;                  ///< Default direction
   };
   struct ActionAnimation {
      const char* name;       ///< Sequence name
      S32      sequence;      ///< Sequence index
      VectorF  dir;           ///< Dir of animation ground transform
      F32      speed;         ///< Speed in m/s
      bool     velocityScale; ///< Scale animation by velocity
      bool     death;         ///< Are we dying?
   };
   enum {
      // *** WARNING ***
      // These enum values are used to index the ActionAnimationList
      // array instantiated in player.cc
      // The first several are selected in the move state based on velocity
      RootAnim,
      RunForwardAnim,
      BackBackwardAnim,
      SideLeftAnim,

      CrouchRootAnim,
      CrouchForwardAnim,
      ProneRootAnim,
      ProneForwardAnim,
      SwimRootAnim,
      SwimForwardAnim,
      SwimBackwardAnim,
      SwimLeftAnim,
      SwimRightAnim,
      
      // These are set explicitly based on player actions
      FallAnim,
      JumpAnim,
      StandJumpAnim,
      LandAnim,
      JetAnim,
      
      // 
      NumMoveActionAnims = SideLeftAnim + 1,
      NumTableActionAnims = JetAnim + 1,

      NumExtraActionAnims = 512 - NumTableActionAnims,
      NumActionAnims = NumTableActionAnims + NumExtraActionAnims,
      ActionAnimBits = 9,
      NullAnimation = (1 << ActionAnimBits) - 1
   };

   static ActionAnimationDef ActionAnimationList[NumTableActionAnims];
   ActionAnimation actionList[NumActionAnims];
   U32 actionCount;
   U32 lookAction;
   S32 spineNode[NumSpineNodes];
   S32 pickupDelta;           ///< Base off of pcikupRadius
   F32 runSurfaceCos;         ///< Angle from vertical in cos(runSurfaceAngle)
   F32 jumpSurfaceCos;        ///< Angle from vertical in cos(jumpSurfaceAngle)

   enum Impacts {
      ImpactNone,
      ImpactNormal,
   };

   enum Recoil {
      LightRecoil,
      MediumRecoil,
      HeavyRecoil,
      NumRecoilSequences
   };
   S32 recoilSequence[NumRecoilSequences];

   /// @name Particles
   /// All of the data relating to environmental effects
   /// @{

   ParticleEmitterData * footPuffEmitter;
   S32 footPuffID;
   S32 footPuffNumParts;
   F32 footPuffRadius;

   DecalData* decalData;
   S32 decalID;

   ParticleEmitterData * dustEmitter;
   S32 dustID;

   SplashData* splash;
   S32 splashId;
   F32 splashVelocity;
   F32 splashAngle;
   F32 splashFreqMod;
   F32 splashVelEpsilon;
   F32 bubbleEmitTime;

   F32 medSplashSoundVel;
   F32 hardSplashSoundVel;
   F32 exitSplashSoundVel;
   F32 footSplashHeight;

   // Air control
   F32 airControl;

   // Jump off surfaces at their normal rather than straight up
   bool jumpTowardsNormal;

   // For use if/when mPhysicsPlayer is created
   StringTableEntry physicsPlayerType;

   ParticleEmitterData* splashEmitterList[NUM_SPLASH_EMITTERS];
   S32 splashEmitterIDList[NUM_SPLASH_EMITTERS];
   /// @}

   //
   DECLARE_CONOBJECT(PlayerData);
   PlayerData();
   bool preload(bool server, String &errorStr);
   void getGroundInfo(TSShapeInstance*,TSThread*,ActionAnimation*);
   bool isTableSequence(S32 seq);
   bool isJumpAction(U32 action);

   static void initPersistFields();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
};


//----------------------------------------------------------------------------

class Player: public ShapeBase
{
   typedef ShapeBase Parent;

protected:

   /// Bit masks for different types of events
   enum MaskBits {
      ActionMask   = Parent::NextFreeMask << 0,
      MoveMask     = Parent::NextFreeMask << 1,
      ImpactMask   = Parent::NextFreeMask << 2,
      NextFreeMask = Parent::NextFreeMask << 3
   };

   struct Range {
      Range(F32 _min,F32 _max) {
         min = _min;
         max = _max;
         delta = _max - _min;
      };
      F32 min,max;
      F32 delta;
   };

   SimObjectPtr<ParticleEmitter> mSplashEmitter[PlayerData::NUM_SPLASH_EMITTERS];
   F32 mBubbleEmitterTime;

   /// Client interpolation/warp data
   struct StateDelta {
      Move move;                    ///< Last move from server
      F32 dt;                       ///< Last interpolation time
      /// @name Interpolation data
     /// @{

      Point3F pos;
      Point3F rot;
      Point3F head;
      VectorF posVec;
      VectorF rotVec;
      VectorF headVec;
     /// @}

     /// @name Warp data
     /// @{

      S32 warpTicks;
      Point3F warpOffset;
      Point3F rotOffset;
     /// @}
   };
   StateDelta delta;                ///< Used for interpolation on the client.  @see StateDelta
   S32 mPredictionCount;            ///< Number of ticks to predict

   // Current pos, vel etc.
   Point3F mHead;                   ///< Head rotation, uses only x & z
   Point3F mRot;                    ///< Body rotation, uses only z
   VectorF mVelocity;               ///< Velocity
   Point3F mAnchorPoint;            ///< Pos compression anchor
   static F32 mGravity;             ///< Gravity
   S32 mImpactSound;

   S32 mMountPending;               ///< mMountPending suppresses tickDelay countdown so players will sit until
                                    ///< their mount, or another animation, comes through (or 13 seconds elapses).

   /// Main player state
   enum ActionState {
      NullState,
      MoveState,
      RecoverState,
      NumStateBits = 3
   };
   ActionState mState;              ///< What is the player doing? @see ActionState
   bool mFalling;                   ///< Falling in mid-air?
   S32 mJumpDelay;                  ///< Delay till next jump   
   
   enum Pose {
      StandPose = 0,
      CrouchPose,
      PronePose,
      SwimPose,
      NumPoseBits = 3
   };
   Pose mPose;
   
   S32 mContactTimer;               ///< Ticks since last contact

   Point3F mJumpSurfaceNormal;      ///< Normal of the surface the player last jumped on
   U32 mJumpSurfaceLastContact;     ///< How long it's been since the player landed (ticks)
   F32  mWeaponBackFraction;        ///< Amount to slide the weapon back (if it's up against something)

   SFXSource* mMoveBubbleSound;   ///< Sound for moving bubbles
   SFXSource* mWaterBreathSound;  ///< Sound for underwater breath

   SimObjectPtr<ShapeBase> mControlObject; ///< Controlling object

   /// @name Animation threads & data
   /// @{

   struct ActionAnimation {
      U32 action;
      TSThread* thread;
      S32 delayTicks;               // before picking another.
      bool forward;
      bool firstPerson;
      bool waitForEnd;
      bool holdAtEnd;
      bool animateOnServer;
      bool atEnd;
   } mActionAnimation;

   struct ArmAnimation {
      U32 action;
      TSThread* thread;
   } mArmAnimation;
   TSThread* mArmThread;

   TSThread* mHeadVThread;
   TSThread* mHeadHThread;
   TSThread* mRecoilThread;
   static Range mArmRange;
   static Range mHeadVRange;
   static Range mHeadHRange;
   /// @}

   bool mInMissionArea;       ///< Are we in the mission area?
   //
   S32 mRecoverTicks;         ///< same as recoverTicks in the player datablock
   U32 mReversePending;

   bool mInWater;            ///< Is true if WaterCoverage is greater than zero
   bool mSwimming;            ///< Is true if WaterCoverage is above the swimming threshold
   //
   PlayerData* mDataBlock;    ///< MMmmmmm...datablock...

   Point3F mLastPos;          ///< Holds the last position for physics updates
   Point3F mLastWaterPos;     ///< Same as mLastPos, but for water

   struct ContactInfo 
   {
      bool contacted, jump, run;
      SceneObject *contactObject;
      VectorF  contactNormal;

      void clear()
      {
         contacted=jump=run=false; 
         contactObject = NULL; 
         contactNormal.set(1,1,1);
      }

      ContactInfo() { clear(); }

   } mContactInfo;

   struct Death {
      F32      lastPos;
      Point3F  posAdd;
      VectorF  rotate;
      VectorF  curNormal;
      F32      curSink;
      void     clear()           {dMemset(this, 0, sizeof(*this)); initFall();}
      VectorF  getPosAdd()       {VectorF ret(posAdd); posAdd.set(0,0,0); return ret;}
      bool     haveVelocity()    {return posAdd.x != 0 || posAdd.y != 0;}
      void     initFall()        {curNormal.set(0,0,1); curSink = 0;}
      Death()                    {clear();}
      MatrixF* fallToGround(F32 adjust, const Point3F& pos, F32 zrot, F32 boxRad);
   } mDeath;

   PhysicsPlayer *mPhysicsPlayer;

   
  public:
  
   // New collision
   OrthoBoxConvex mConvex;
   Box3F          mWorkingQueryBox;

// SphyxGames -> Melee
   virtual bool setArmThreadPlayOnce(const char* sequence);
   virtual bool setArmThreadTransitionOnce(const char* sequence);
   bool mArmThreadPlayOnce;


   void startPlayOnce(F32 timeScale);
   void stopPlayOnce();
   void advancePlayOnceTime(F32 dt);

   U32 mArmThreadSavedAction;

// SphyxGames <- Melee
   /// Standing / Crouched / Prone or Swimming   
   Pose getPose() const { return mPose; }
   
   /// Setting this from script directly might not actually work,
   /// This is really just a helper for the player class so that its bounding box
   /// will get resized appropriately when the pose changes
   void setPose( Pose pose );

   PhysicsPlayer* getPhysicsPlayer() const { return mPhysicsPlayer; }

  protected:
   void setState(ActionState state, U32 ticks=0);
   void updateState();

   // Jetting
   bool mJetting;

   ///Update the movement
   virtual void updateMove(const Move *move);

   ///Interpolate movement
   Point3F _move( const F32 travelTime, Collision *outCol );
   void _handleCollision( const Collision &collision );
   virtual bool updatePos(const F32 travelTime = TickSec);

   ///Update head animation
   void updateLookAnimation(F32 dT = 0.f);

   ///Update other animations
   void updateAnimation(F32 dt);
   void updateAnimationTree(bool firstPerson);
   bool step(Point3F *pos,F32 *maxStep,F32 time);

   ///See if the player is still in the mission area
   void checkMissionArea();

   virtual bool setArmThread(U32 action);
   virtual void setActionThread(U32 action,bool forward,bool hold = false,bool wait = false,bool fsp = false, bool forceSet = false);
   virtual void updateActionThread();
   virtual void pickActionAnimation();

   /// @name Mounted objects
   /// @{
   virtual void onUnmount( ShapeBase *obj, S32 node );
   /// @}

   void setPosition(const Point3F& pos,const Point3F& viewRot);
   void setRenderPosition(const Point3F& pos,const Point3F& viewRot,F32 dt=-1);
   void _findContact( SceneObject **contactObject, VectorF *contactNormal );
   void findContact( bool *run, bool *jump, VectorF *contactNormal );
   virtual void onImageRecoil(U32 imageSlot,ShapeBaseImageData::StateData::RecoilState);
   virtual void updateDamageLevel();
   virtual void updateDamageState();
   /// Set which client is controlling this player
   void setControllingClient(GameConnection* client);

   void calcClassRenderData();
   /// Play a footstep sound
   void playFootstepSound(bool triggeredLeft, S32 sound);
   /// Play an impact sound
   void playImpactSound();

   /// Are we in the process of dying?
   bool inDeathAnim();
   F32  deathDelta(Point3F &delta);
   void updateDeathOffsets();
   bool inSittingAnim();

   /// @name Water
   /// @{

   void updateSplash();                             ///< Update the splash effect
   void updateFroth( F32 dt );                      ///< Update any froth
   void updateWaterSounds( F32 dt );                ///< Update water sounds
   void createSplash( Point3F &pos, F32 speed );    ///< Creates a splash
   bool collidingWithWater( Point3F &waterHeight ); ///< Are we colliding with water?
   /// @}

public:
   DECLARE_CONOBJECT(Player);

   Player();
   ~Player();
   static void consoleInit();

   /// @name Transforms
   /// @{

   void setTransform(const MatrixF &mat);
   void getEyeTransform(MatrixF* mat);
   void getRenderEyeTransform(MatrixF* mat);
   void getCameraParameters(F32 *min, F32 *max, Point3F *offset, MatrixF *rot);
   void getMuzzleTransform(U32 imageSlot,MatrixF* mat);
   void getRenderMuzzleTransform(U32 imageSlot,MatrixF* mat);   

   /// @}

   F32 getSpeed() const;
   Point3F getVelocity() const;
   void setVelocity(const VectorF& vel);
   /// Apply an impulse at the given point, with magnitude/direction of vec
   void applyImpulse(const Point3F& pos,const VectorF& vec);
   /// Get the rotation of the player
   const Point3F& getRotation() { return mRot; }
   /// Get the rotation of the head of the player
   const Point3F& getHeadRotation() { return mHead; }
   void getDamageLocation(const Point3F& in_rPos, const char *&out_rpVert, const char *&out_rpQuad);

   bool canJump();                                         ///< Can the player jump?
   bool canJetJump();                                      ///< Can the player jet?
   bool canSwim();                                         ///< Can the player swim?
   bool canCrouch();
   bool canStand();
   bool canProne();
   bool haveContact() const { return !mContactTimer; }         ///< Is it in contact with something
   void getMuzzlePointAI( U32 imageSlot, Point3F *point );
   F32 getMaxForwardVelocity() const { return (mDataBlock != NULL ? mDataBlock->maxForwardSpeed : 0); }

   virtual bool    isDisplacable() const;
   virtual Point3F getMomentum() const;
   virtual void    setMomentum(const Point3F &momentum);
   virtual bool    displaceObject(const Point3F& displaceVector);
   virtual bool    getAIMove(Move*);

   bool checkDismountPosition(const MatrixF& oldPos, const MatrixF& newPos);  ///< Is it safe to dismount here?

   //
   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData* dptr);
   void onScaleChanged();
   Box3F mScaledBox;

   // Animation
   const char* getStateName();
   bool setActionThread(const char* sequence,bool hold,bool wait,bool fsp = false);
   bool setArmThread(const char* sequence);

   // Object control
   void setControlObject(ShapeBase *obj);
   ShapeBase* getControlObject();
   
   virtual void disableCollision();   
   virtual void enableCollision();

   //
   void updateWorkingCollisionSet();
   virtual void processTick(const Move *move);
   void interpolateTick(F32 delta);
   void advanceTime(F32 dt);
   bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);
   bool buildPolyList(AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere);
   void buildConvex(const Box3F& box, Convex* convex);
   bool isControlObject();

   void onCameraScopeQuery(NetConnection *cr, CameraScopeQuery *);
   void writePacketData(GameConnection *conn, BitStream *stream);
   void readPacketData (GameConnection *conn, BitStream *stream);
   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);

   virtual bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
   virtual void renderConvex( ObjectRenderInst *ri, SceneState *state, BaseMatInstance *overrideMat );   
   virtual void renderMountedImage( U32 imageSlot, TSRenderState &rstate );
};


#endif
