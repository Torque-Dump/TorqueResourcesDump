/* This tutorial will show you how to add a server-side melee system to your Torque project.
/* By Josh Moore AKA The_Force
/* This code can support shields, sword on sword collisions, and
/* simple locational damage (head, torso, legs only) with no other c++ modifications.

/* Back up your player.cc(.h), shapebase.cc(.h), and shapeimage.cc incase you can't get this to work
/* Instructions use "/*" whereas normal code comments use "//"
/* If farther help is needed, check http://www.garagegames.com/community/resources/view/5377
/* Ok, now onto the code!

/* Step One: Melee Weapon Itegration
/* Replace your weapon.cs with the one supplied.

/* Step Two: Sword Script
/* Place sword.cs in the same folder as weapon.cs. Then add: */
   exec("./sword.cs");
/* to the bottom of the file executes in game.cs

/* Step Three:
/* Into the engine!
/* First we'll be working in player.cc.
/* At the end of Player::Player() add: */
   mArmThreadPlayOnce = false;

/* Next, find Player::updateLookAnimation() and replace: */
if (mArmAnimation.thread) {
   // TG: Adjust arm position to avoid collision.
   F32 tp = mControlObject? 0.5:
      (mHead.x - mArmRange.min) / mArmRange.delta;
   mShapeInstance->setPos(mArmAnimation.thread,mClampF(tp,0,1));
   }

/* with: */
   if (!mArmThreadPlayOnce)
   {
      // if we are doing play once anim then we dont adjust arm thread based on head tilt...
      if (mArmAnimation.thread) {
         // TG: Adjust arm position to avoid collision.
         F32 tp = mControlObject? 0.5:
            (mHead.x - mArmRange.min) / mArmRange.delta;
         mShapeInstance->setPos(mArmAnimation.thread,mClampF(tp,0,1));
      }
   }

/* Now add this after void Player::SetActionThread(): */
// phdana hth play once arm thread->
void Player::startPlayOnce(F32 timeScale)
{
   mShapeInstance->setTimeScale(mArmAnimation.thread,timeScale);
}

void Player::stopPlayOnce()
{
   mShapeInstance->setTimeScale(mArmAnimation.thread,0.0);

   // only do this if on server
   if (!isGhost())
      setArmThread(mArmThreadSavedAction);
}

// called on server
bool Player::setArmThreadPlayOnce(const char* sequence)
{
   // if we are already playing arm thread...ignore
   if (mArmThreadPlayOnce)
      return true;

   // save old sequence
   mArmThreadSavedAction = mArmAnimation.action;
   if (!setArmThread(sequence))
      return false;

   // flag that we are playing once!
   mArmThreadPlayOnce = true;
   // melee
   //for (int i = 0; i < MaxMountedImages; i++)
      //if (mMountedImageList[i].dataBlock)
         //UpdateImageRaycastDamage(i, TickSec);
   // melee
   startPlayOnce(1.0);
   return true;
}

// phdana stun->
// called on server
bool Player::setArmThreadTransitionOnce(const char* sequence)
{
   // generally this will only be called when we are ALREADY
   // playing a play once arm thread...but in case we
   // are not..then just play normally
   if (!mArmThreadPlayOnce)
      return setArmThreadPlayOnce(sequence);

   S32 seq = mDataBlock->shape->findSequence("h1stun");
   F32 time = 0.25;

   if (seq == -1)
      return setArmThreadPlayOnce(sequence);

   // otherwise lets transition to the new sequence
   F32 pos = mShapeInstance->getPos(mArmAnimation.thread);
   mShapeInstance->transitionToSequence(mArmAnimation.thread, seq, pos, time, true);

   return true;
}
// <- phdana stun

// advance time on a play once arm thread...if we have
// reached the end of time on this animation then
// clear the play once status and restore old thread
// NOTE: only call this method when mArmThreadPlayOnce is true
void Player::advancePlayOnceTime(F32 dt)
{
   // advance time
   mShapeInstance->advanceTime(dt,mArmAnimation.thread);

   // if we reached end...
   if (mShapeInstance->getPos(mArmAnimation.thread) == 1.0)
   {
      // phdana hth twitch fix ->
      if (!isGhost())
      {
         stopPlayOnce();
         mArmThreadPlayOnce = false;
      }
      // <- phdana hth twitch fix
   }
}
// <- phdana hth play once arm thread

/* Ok, now in Player::updateAnimation(), add this right after the first two ifs. */
   // phdana hth play once arm thread ->
   if (mArmThreadPlayOnce)
   {
      advancePlayOnceTime(dt);

      // if we are doing a "play once" for the purpose of hand-to-hand
      // then we must also ensure that mounted images get updated correctly
      // when doing this on the server
      if (!isGhost())
         mShapeInstance->animate();
   }

/* In Player::packupdate(), after: */
stream->writeInt(mArmAnimation.action,PlayerData::ActionAnimBits);

/* add: */
   // phdana hth play once arm thread ->
   stream->writeFlag(mArmThreadPlayOnce);
   // <- phdana hth play once arm thread

/* Player::unpackupdate() after: */
else
   mArmAnimation.action = action;

/* add: */
      // phdana hth play once arm thread ->
      mArmThreadPlayOnce = stream->readFlag();
      if (mArmThreadPlayOnce)
         startPlayOnce(1.0);
      // phdana hth twitch fix ->
      else
         stopPlayOnce();
       // <- phdana hth twitch fix
      // <- phdana hth play once arm thread

/* Almost done in player.cc, before: */
ConsoleMethod( Player, getState, const char*, 2, 2, "Return the current state name.")
{
   return object->getStateName();
}

/* add: */
// phdana hth play once arm thread ->
ConsoleMethod( Player, setArmThreadPlayOnce, bool, 3, 3, "(string sequenceName)")
{
   return object->setArmThreadPlayOnce(argv[2]);
}
ConsoleMethod( Player, SetArmThreadTransitionOnce, bool, 3, 3, "(string sequenceName)")
{
   return object->setArmThreadTransitionOnce(argv[2]);
}

/* Step Four: player.h
/* in call Player: Public, after: */
TSThread* mArmThread;

/* add: */
   // phdana hth play once arm thread->
   bool mArmThreadPlayOnce;   // if true then special case where we actaully PLAY arm thread
   U32 mArmThreadSavedAction; // arm thread that was active prior to play once thread
   // <- phdana hth play once arm thread

/* further down, after: */
Box3F          mWorkingQueryBox;

/* add: */
   // phdana hth play once arm thread->
   void startPlayOnce(F32 timeScale);
   void stopPlayOnce();
   bool setArmThreadPlayOnce(const char* sequence);
   // phdana stun ->
   bool setArmThreadTransitionOnce(const char* sequence);
   // <- phdana stun
   void advancePlayOnceTime(F32 dt);
   // <- phdana hth play once arm thread

/* Step Five: shapeBase.h
/* in struct ShapeBaseImageData: public, after: */
S32 fireState;                   ///< The ID of the fire state.

/* add: */
   // phdana
   S32  damageStartNode;            // PC: the node which is used to start the damage raycast
   S32  damageEndNode;              // the end node to raycast to.. anything between start and end
                                    // is damaged.
   // phdana

/* Then in class ShapeBase : public GameBase, after: */
virtual void onImpact(VectorF vec);

/* add: */
   virtual void UpdateImageRaycastDamage(F32 dt, U32 imageSlot );

/* then after: */
SoundMaskN      = Parent::NextFreeMask << 8,       ///< Extends + MaxSoundThreads bits

/* add: */
      ServerIdMask    = Parent::NextFreeMask << 9,

/* further down, after: */
const char* getSkinName();

/* add: */
   // PC server id for this ghost? should be a base method somewehere.
   // phdana
   S32 mShapeServerId;
   void setShapeServerId(S32 Id);
   S32 getShapeServerId(){ return mShapeServerId; };
   // phdana

/* Step Six: shapeBase.cc
/* in ShapeBase::ShapeBase(), after: */
mAppliedForce.set(0, 0, 0);

/* add: */
   mShapeServerId = 0;

/* in ShapeBase::processTick(), after: */
updateServerAudio();

/* add: */
      // phdana hth server side axe ->
      // this function was previously only called on client side
      for (int i = 0; i < MaxMountedImages; i++)
         if (mMountedImageList[i].dataBlock)
            UpdateImageRaycastDamage( TickSec, i );
      // <- phdana hth server side axe

/* in ShapeBase::packUpdate(), replace: */
if(!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask |
         ThreadMask | ImageMask | CloakMask | MountedMask | InvincibleMask |
         ShieldMask | SkinMask))
      return retMask;

/* with: */
   if(!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask |
         ThreadMask | ImageMask | CloakMask | MountedMask | InvincibleMask |
         ShieldMask | SkinMask | ServerIdMask)))
      return retMask;

/* further down, replace: */
// Group some of the uncommon stuff together.
   if (stream->writeFlag(mask & (NameMask | ShieldMask | CloakMask | InvincibleMask | SkinMask))) {
      if (stream->writeFlag(mask & CloakMask)) {

/* with: */
   // Group some of the uncommon stuff together.
   if (stream->writeFlag(mask & (NameMask | ShieldMask | CloakMask | InvincibleMask | SkinMask | ServerIdMask ))) {
      if (stream->writeFlag(mask & CloakMask)) {

/* down a few lines, after: */
// piggyback control update
stream->writeFlag(bool(getControllingClient()));

/* add: */
         // PC: added ServerIdMask and support code to send ServerId value.
        if (stream->writeFlag(mask & ServerIdMask)) {
            stream->writeInt(mShapeServerId,32);
        }

/* now in ShapeBase::unpackUpdate(), after: */
setCloakedState(stream->readFlag());
         mIsControlled = stream->readFlag();

/* add: */
         // PC: read team id value (8 bits signed integer)
         if(stream->readFlag())
         {
            mShapeServerId = stream->readInt(32);
         }

/* now, add this before shapebase::consoleinit() */
//----------------------------------------------------------------------------
// PC: shape id thats valid on the client for any server ident needed.
void ShapeBase::setShapeServerId(S32 Id)
{
   if (!isGhost())
   {
      mShapeServerId = Id;
      // dirty the team mask so the team id gets updated
      setMaskBits(ServerIdMask);
   }
}

ConsoleMethod( ShapeBase, setShapeServerId, void, 3, 3, "(id)")
{
   if (object->isServerObject())
      object->setShapeServerId(dAtoi(argv[2]));
}

/* Step Seven: ShapeImage.cc
/* this is very IMPORTANT, with out this code the engine will crash
/* in function ShapeBaseImageData::preload(), BEFORE */
// Resolve nodes & build mount transform
      ejectNode = shape->findNode("ejectPoint");
      muzzleNode = shape->findNode("muzzlePoint");

/* Add: */
      // PC: find our damage location nodes..
      damageStartNode = shape->findNode("damageStart");
      damageEndNode = shape->findNode("damageEnd");

/* Add this functions to the bottom of the file: */
void ShapeBase::UpdateImageRaycastDamage( F32 dt,U32 imageSlot)
{
   MountedImage& image = mMountedImageList[imageSlot];
   ShapeBaseImageData* imageData = image.dataBlock;

   if (!image.dataBlock) return;

    // test if our mount nodes are available.
   if (imageData->damageStartNode == -1) return;
   if (imageData->damageEndNode == -1) return;

   // temporarily say this is false so we get
   // the image transforms from the MOUNT points
   ShapeBaseImageData& data = *image.dataBlock;
   bool oldUse = data.useEyeOffset;
   data.useEyeOffset = false;

   // if we have our mount nodes, transform them to world space
   MatrixF startTrans;
   getImageTransform( imageSlot, imageData->damageStartNode, &startTrans );
   VectorF vecStart = startTrans.getPosition();
   MatrixF endTrans;
   getImageTransform( imageSlot, imageData->damageEndNode, &endTrans );
   VectorF vecEnd = endTrans.getPosition();

   // restore
   data.useEyeOffset = oldUse;

   // then call the raycast function in script to do the damage. lets call it onImageIntersect
   char buff1[32];
   char buff2[100];
   char buff3[100];

   dSprintf(buff1,sizeof(buff1),"%d",getShapeServerId());
   dSprintf(buff2,sizeof(buff2),"%f %f %f",vecStart.x, vecStart.y, vecStart.z);
   dSprintf(buff3,sizeof(buff3),"%f %f %f",vecEnd.x, vecEnd.y, vecEnd.z);

   // call the script function!
   Con::executef(image.dataBlock, 5, "onImageIntersect",scriptThis(),buff1,buff2,buff3);
}

/* Step Eight: Art
/* Place the sword folder inside of the folder data/shapes/
/* Then place the contents of the orc folder in data/shapes/player (just the contents, not the folder itself)

/* Step Nine: Inventory Script
/* In server/scripts/game.cs funcion GameConnection::createPlayer(), replace: */
%player.mountImage(CrossbowImage,0);

/* with: */
   %player.setInventory(Sword,1);
   %player.setInventory(SwordAmmo,50);
   %player.mountImage(SwordImage,0);

/* In server/scripts/player.cs datablock PlayerData(PlayerBody) add this to the bottom of the other maxInv's: */
   maxInv[Sword] = 1;

/* Step Ten: Compile
/* Now do a full recompile of the source code and delete all of your old .dso files, then load a mission. */
