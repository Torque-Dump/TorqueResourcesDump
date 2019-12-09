//
// This tutorial will show you how to add a server-side melee system to your Torque project.
// By Josh Moore AKA The_Force
// This code can support shields, sowrd on sowrd collisions,
// and simple locational damage(head, torso, legs only) with no other c++ modifications.
//

// The screenshots included show off what you can do with this system.
// melee1 shows me blocking two attacks with my shield(he gets thrown back).
// melee2 shows me and my target after our swors collided(we both get thrown back).
// melee3 shows another example of what a sowrd on sword collision can look like.

// Back up your player.cc(.h), shapebase.cc(.h), and shapeimage.cc incase you can't get this to work
// Ok, now onto the code!

// Step One: Melee Weapon Itegration
// Replace your weapon.cs with this:

//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// This file contains Weapon and Ammo Class/"namespace" helper methods
// as well as hooks into the inventory system. These functions are not
// attached to a specific C++ class or datablock, but define a set of
// methods which are part of dynamic namespaces "class". The Items
// include these namespaces into their scope using the  ItemData and
// ItemImageData "className" variable.

// All ShapeBase images are mounted into one of 8 slots on a shape.
// This weapon system assumes all primary weapons are mounted into
// this specified slot:
$WeaponSlot = 0;


//----------------------------d-------------------------------------------------
// Audio profiles

datablock AudioProfile(WeaponUseSound)
{
   filename = "~/data/sound/weapon_switch.wav";
   description = AudioClose3d;
	preload = true;
};

datablock AudioProfile(WeaponPickupSound)
{
   filename = "~/data/sound/weapon_pickup.wav";
   description = AudioClose3d;
	preload = true;
};

datablock AudioProfile(AmmoPickupSound)
{
   filename = "~/data/sound/ammo_pickup.wav";
   description = AudioClose3d;
	preload = true;
};

//-----------------------------------------------------------------------------
// Weapon Class
//-----------------------------------------------------------------------------

function Weapon::onUse(%data,%obj)
{
   // Default behavoir for all weapons is to mount it into the
   // this object's weapon slot, which is currently assumed
   // to be slot 0

   if (%obj.getMountedImage($WeaponSlot) != %data.image.getId()) {
      ServerPlay3D(WeaponUseSound,%obj.getTransform());
      %obj.mountImage(%data.image, $WeaponSlot);

	//if (%data.itemType $= "melee")
       //commandToClient(%obj.client, 'force3rdPerson', 1);
	//else
       //commandToClient(%obj.client, 'force3rdPerson', 0);
   }
}

function Weapon::onPickup(%this, %obj, %shape, %amount)
{
   // The parent Item method performs the actual pickup.
   // For player's we automatically use the weapon if the
   // player does not already have one in hand.
   if (Parent::onPickup(%this, %obj, %shape, %amount)) {
      ServerPlay3D(WeaponPickupSound,%obj.getTransform());
      if (%shape.getClassName() $= "Player" &&
            %shape.getMountedImage($WeaponSlot) == 0)  {
         %shape.use(%this);
      }
   }
}

function Weapon::onInventory(%this,%obj,%amount)
{
   // Weapon inventory has changed, make sure there are no weapons
   // of this type mounted if there are none left in inventory.
   if (!%amount && (%slot = %obj.getMountSlot(%this.image)) != -1)
      %obj.unmountImage(%slot);
}


//-----------------------------------------------------------------------------
// Weapon Image Class
//-----------------------------------------------------------------------------

// phdana hth ->
// a 'hand to hand attack' is a sequence that gets played
// as a "play once look anim". Hand to Hand weapons, such
// as an axe, can have one or more 'hand to hand attacks'
// that they can play

datablock GameBaseData(OneHandedAttackSwing)
{
   seqName = "h1swing";
   timeScale = 1.5;
   damageAmount = 30;
   //startDamage = 0.2;
   //endDamage = 0.6;
   startDamage = 0.2;
   endDamage = 1.3;
};

datablock GameBaseData(OneHandedAttackSlice)
{
   seqName = "h1slice";
   timeScale = 1.0;
   damageAmount = 30;
   //startDamage = 0.3;
   //endDamage = 0.7;
   startDamage = 0.1;
   endDamage = 0.9;
};

datablock GameBaseData(OneHandedAttackThrust)
{
   seqName = "h1thrust";
   timeScale = 1.0;
   damageAmount = 30;
   //startDamage = 0.4;
   //endDamage = 0.8;
   startDamage = 0.1;
   endDamage = 0.9;
};

//datablock GameBaseData(OneHandedJumpAttack)
//{
   //seqName = "h1jumpattack";
   //timeScale = 1.0;
   //damageAmount = 30;
   //startDamage = 0.4;
   //endDamage = 0.8;
   //startDamage = 0.1;
   //endDamage = 0.9;
//};

// this is the default function to call when firing a hand-to-hand weapon
function WeaponImage::onFireHandToHand(%this, %obj, %slot)
{
    if(%obj.hthStun) //|| %obj.shielded)
       return;
    // there was code here for special attacks
       %action = "Normal";
    switch$(%action)
    {
       //case "JumpAttack":
          //%attack = %this.jumpAttack;
       case "Normal":
          // for now we randomly choose an attack
          %index = mFloor(getRandom()*(%this.hthNumAttacks-0.0001));
          if (%index > (%this.hthNumAttacks-1))
             %index = (%this.hthNumAttacks-1);
          %attack = %this.hthAttack[%index];
    }
    // setup the "play once look anim"
    %obj.hthDamageAttack = %attack;
    %obj.hthDamageSeqPlaying = 1;
    %obj.hthDamageStartMS =  $sim::Time;
    %obj.hthDamageLastId = -1;

    if (!%obj.setArmThreadPlayOnce(%attack.seqName))
       echo("ERROR in setArmThreadPlayOnce()");
    return;
}

// default weapon intersect
function WeaponImage::onImageIntersect(%this,%player,%slot,%startvec,%endvec)
{
    // if damage sequence is not playing then dont do damage
    if (!%player.hthDamageSeqPlaying || %player.getState() $= "Dead")
       return;

    // determine if damage is active or if we can say the seq is done playing
    // based on current server time
    %offset = $sim::Time - %player.hthDamageStartMS;

    // depending on which attack is playing...
    %attack = %player.hthDamageAttack;
    %startOffset = %attack.startDamage;
    %endOffset = %attack.endDamage;

    // how long until the last damage is done
    // at which point we can say the seq has "Stopped playing"
    if (%offset > %endOffset)
    {
       %player.hthDamageSeqPlaying = 0;
       %player.hthDamageActive = 0;
	//   echo("seq stopping (all damage done) %offset = " @ %offset);
       return;
    }

    // how long it takes for damage to start...for now we just
    // have one interval and damage is active all during that interval
    if (%offset >%startOffset)
       %player.hthDamageActive = 1;

    // no damage yet?
    if (!%player.hthDamageActive)
    {
	//   echo("seq playing (no damage) %offset = " @ %offset);
       return;
    }

    // search for just players to damage
    %searchMasks = $TypeMasks::PlayerObjectType | $TypeMasks::StaticShapeObjectType;
    // search for objects within the damage rays that fit the masks above
    %scanTarg = ContainerRayCast(%startvec, %endvec, %searchMasks, %slot);

    if(%scanTarg && (%scanTarg.getType() & $TypeMasks::PlayerObjectType))
    {
        // a target in range was found
        %target = firstWord(%scanTarg);

        // store end point from raycast return buffer
        %pos = getWords(%scanTarg, 1, 3);

        // if we have hit this person already...apply no more damage
        if (%target == %player.hthDamageLastId)
           return;

        // save who we last damaged
        %player.hthDamageLastId = %target;

        // Apply damage targetted object
   		// Works for all shapebase objects.
        if (%target.getState() !$= "Dead" && %target.getId() !$= %player.getId())
   		{
           %damage = %attack.damageAmount;
           %damageType = %this.item; // example: Axe / Sword etc

           %damLoc = firstWord(%target.getDamageLocation(%pos));
           // you can use this to add limited loacational damage, but the head is whats hit the most - TF
           //if(%damLoc $= "head")
              //error("object sliced on head");
           //else if(%damLoc $= "torso")
              //error("object sliced on torso");
           //else if(%damLoc $= "legs")
              //error("object sliced on legs");

           // code ripped from Armor::damgae
  		   %target.applyDamage(%damage);

           // this is in the Armor::damage
           //%location = "Body";

           // Deal with client callbacks here because we don't have this
           // information in the onDamage or onDisable methods
           %client = %target.client;
           %sourceObject = %this;
           %sourceClient = %sourceObject ? %sourceObject.client : 0;

           if (%target.getState() $= "Dead")
           {
              if(%client)
                 %client.onDeath(%sourceObject, %sourceClient, %damageType, %pos);
           }
           // phdana stun ->
           else
           {
              stunPlayer(%target,%attack);

              pushPlayerBack(%target,%pos,%player,%attack);
           }
           // <- phdana stun
      	}
    }
    else if(%scanTarg && (%scanTarg.getType() & $TypeMasks::StaticShapeObjectType))
    {
       %damage = %attack.damageAmount;
       %object = firstWord(%scanTarg);
       %object.applyDamage(%damage);
    }

}

// phdana stun ->
// call when %victim is hit with %attack but does not die
function stunPlayer(%vplayer, %attack)
{
   // for now we stun every time...

   // get the player for this object
   //if (!%victim.client || !%victim.client.player)
   if(!%vplayer.getType() & $TypeMasks::PlayerObjectType)
   {
      error("ATTEMPTING to STUN a non-player");
      return;
   }

   // if this player is in the middle of a hth swing themself, then
   // their swing is aborted. firstly we have to make sure they dont
   // do any damage, secondly we must blend their swing anim into
   // the stun anim
   if (%vplayer.hthDamageSeqPlaying)
   {
      // make sure they wont do damage....
      %vplayer.hthDamageSeqPlaying = false;

      // blend into the stun animation
      //error("STUN: victim: " @ %vplayer @ " DOING transition once...");
      %vplayer.setArmThreadTransitionOnce("h1stun");
   }
   else
   {
      // just start the stun animation
      //error("STUN: victim: " @ %vplayer @ " only doing a playonce...");
      if(%vplayer.shielded)// not while stuned!
         %vplayer.setImageTrigger(1,false);
      %vplayer.setArmThreadPlayOnce("h1stun");
   }

   // the victim is now in a stun state
   //%vplayer.hthStunSequencePlaying = true;
   %vplayer.hthStun = true;
   schedule(1000, %vplayer, "resetStun", %vplayer);
   //%vplayer.hthStunStartMS = $sim::Time;
}

function resetStun(%obj)
{
   %obj.hthStun = false;
}

function pushPlayerBack(%victim, %pos, %attacker, %attack)
{
  // the push back is relative to the attacker
  // a straight push back would be along the attackers
  // Y axis....

  // right now we always push the victim at his center
  // we could explore what happnes if we push at the
  // point of contact instead (might turn or do something intersting)

  // get the usual direction to push...we could get the Y axis of
  // the attacker with getTransform() then grabbing the rotation part
  // and passing that to VectorOrthoBasis() and then using column 1
  // whichi would be words 3,4,5 (couting from 0)...but that's overkill
  // for something that can be approximated pretty good by a line drawn
  // from attacker to victim...so let's use that instead
  %vpos = %victim.getWorldBoxCenter();
  %pushDirection = VectorSub(%vpos,%attacker.getWorldBoxCenter());
  %pushDirection = VectorNormalize(%pushDirection);

  // hardoded impluse
  %impulse = 15.0;

  // ok apply impulse to victim's center
  %mass = %victim.getDataBlock().mass;
  %pushVec = VectorScale(%pushDirection,%impulse * %mass);

  //error("Applying, to player " @ %victim @ " of mass " @ %mass @ ", an impulseVec: " @ %pushVec);

  %victim.applyImpulse(%vpos, %pushVec);
}

// <- phdana hth

function WeaponImage::onMount(%this,%obj,%slot)
{
   // Images assume a false ammo state on load.  We need to
   // set the state according to the current inventory.
   if (%obj.getInventory(%this.ammo))
      %obj.setImageAmmo(%slot,true);

   if (%this.customLookAnim !$= "")
   {
      %obj.setArmThread(%this.customLookAnim);
   }
   else
   {
      %obj.setArmThread("look");
   }
}


//-----------------------------------------------------------------------------
// Ammmo Class
//-----------------------------------------------------------------------------

function Ammo::onPickup(%this, %obj, %shape, %amount)
{
   // The parent Item method performs the actual pickup.
   if (Parent::onPickup(%this, %obj, %shape, %amount)) {
      ServerPlay3D(AmmoPickupSound,%obj.getTransform());
   }
}

function Ammo::onInventory(%this,%obj,%amount)
{
   // The ammo inventory state has changed, we need to update any
   // mounted images using this ammo to reflect the new state.
   for (%i = 0; %i < 8; %i++) {
      if ((%image = %obj.getMountedImage(%i)) > 0)
         if (isObject(%image.ammo) && %image.ammo.getId() == %this.getId())
            %obj.setImageAmmo(%i,%amount != 0);
   }
}

// Step Two: Sword Script
// Make a .cs file called sword.cs and put this in it:

//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------
//

//--------------------------------------------------------------------------
// Weapon Item.  This is the item that exists in the world, i.e. when it's
// been dropped, thrown or is acting as re-spawnable item.  When the weapon
// is mounted onto a shape, the CrossbowImage is used.

datablock ItemData(Sword)
{
   // Mission editor category
   category = "Weapon";

   // Hook into Item Weapon class hierarchy. The weapon namespace
   // provides common weapon handling functions in addition to hooks
   // into the inventory system.
   className = "Weapon";

   // Basic Item properties
   shapeFile = "~/data/shapes/sword/rune_blade01.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;

	// Dynamic properties defined by the scripts
	pickUpName = "a sword";
	image = SwordImage;

	itemType="melee";
	trayIcon = "sword01.png";
};


//--------------------------------------------------------------------------
// Crossbow image which does all the work.  Images do not normally exist in
// the world, they can only be mounted on ShapeBase objects.

// phdana hth ->
datablock ShapeBaseImageData(SwordImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/sword/rune_blade01.dts";
   emap = true;

   // Specify mount point & offset for 3rd person, and eye offset
   // for first person rendering.
   mountPoint = 0;
   eyeOffset = "0.1 0.4 -0.6";

   // When firing from a point offset from the eye, muzzle correction
   // will adjust the muzzle vector to point to the eye LOS point.
   // Since this weapon doesn't actually fire from the muzzle point,
   // we need to turn this off.
   correctMuzzleVector = false;

   // Add the WeaponImage namespace as a parent, WeaponImage namespace
   // provides some hooks into the inventory system.
   className = "WeaponImage";

   // Projectile && Ammo.
   item = Sword;
   ammo = CrossbowAmmo;
   projectile = CrossbowProjectile;
   projectileType = Projectile;
   // we are a HAND TO HAND weapon so we have a custom look anim
   //customLookAnim = "h1root";  // as a test
   customLookAnim = "looknw";

   // Here are the Attacks we support
   hthNumAttacks = 3;
   hthAttack[0]                     = OneHandedAttackSwing;
   hthAttack[1]                     = OneHandedAttackSlice;
   hthAttack[2]                     = OneHandedAttackThrust;
   //jumpAttack                       = OneHandedJumpAttack;

   // Images have a state system which controls how the animations
   // are run, which sounds are played, script callbacks, etc. This
   // state system is downloaded to the client so that clients can
   // predict state changes and animate accordingly.  The following
   // system supports basic ready->fire->reload transitions as
   // well as a no-ammo->dryfire idle state. In this case we are a
   // HAND to HAND weapon and there is no ammo but we can use the
   // reload time to limit how often the weapon can be fired

   // Initial start up state
   stateName[0]                     = "Preactivate";
   stateTransitionOnLoaded[0]       = "Activate";

   // Activating the gun.  Called when the weapon is first mounted
   stateName[1]                     = "Activate";
   stateTransitionOnTimeout[1]      = "Ready";
   stateTimeoutValue[1]             = 0.6;
   //stateSequence[1]                 = "Activate";

   // Ready to fire, just waiting for the trigger
   stateName[2]                     = "Ready";
   stateTransitionOnTriggerDown[2]  = "Fire";

   // Fire the weapon. Calls the fire script which does the actual work.
   stateName[3]                     = "Fire";
   stateTransitionOnTimeout[3]      = "Reload";
   stateTimeoutValue[3]             = 0.2;
   stateFire[3]                     = true;
   stateAllowImageChange[3]         = false;
   //stateSequence[3]                 = "Fire";
   stateScript[3]                   = "onFire";
   //stateSound[3]                    = CrossbowFireSound;

   // Play the relead animation, and transition into
   stateName[4]                     = "Reload";
   stateTransitionOnTimeout[4]      = "Ready";
   stateTimeoutValue[4]             = 0.8;
   stateAllowImageChange[4]         = false;
   //stateSequence[4]                 = "Reload";
   stateEjectShell[4]               = false;
   //stateSound[4]                    = CrossbowReloadSound;
};


//-----------------------------------------------------------------------------

function SwordImage::onFire(%this, %obj, %slot)
{
    // default hand to hand weapon code
    WeaponImage::onFireHandToHand(%this, %obj, %slot);

    return;
}
// <- phdana hth

// Step Three:
// Into the engine!
// First we'll be working in player.cc.
// At the end of Player::Player() add:
mArmThreadPlayOnce = false;

// Next, find Player::updateLookAnimation() and replace:

// if we are doing play once anim then we dont adjust arm thread based on head tilt...
if (mArmAnimation.thread) {
   // TG: Adjust arm position to avoid collision.
   F32 tp = mControlObject? 0.5:
      (mHead.x - mArmRange.min) / mArmRange.delta;
   mShapeInstance->setPos(mArmAnimation.thread,mClampF(tp,0,1));
   }
}

// with:
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

// Now add this after Player::SetActionThread:

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

// Ok, now in Player::updateAnimation, add this right after the first 2 ifs.
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
   
// In player::packupdate, after
stream->writeInt(mArmAnimation.action,PlayerData::ActionAnimBits);

// add:
// phdana hth play once arm thread
stream->writeFlag(mArmThreadPlayOnce);
// <- phdana hth play once arm thread->

// player::unpackupdate after:
else
   mArmAnimation.action = action;
   
//add:
// phdana hth play once arm thread
mArmThreadPlayOnce = stream->readFlag();
if (mArmThreadPlayOnce)
   startPlayOnce(1.0);
// phdana hth twitch fix ->
else
   stopPlayOnce();
// <- phdana hth twitch fix
// <- phdana hth play once arm thread->

// Almost done in player.cc, before:
ConsoleMethod( Player, getState, const char*, 2, 2, "Return the current state name.")
{
   return object->getStateName();
}
// add:
// phdana hth play once arm thread ->

ConsoleMethod( Player, setArmThreadPlayOnce, bool, 3, 3, "(string sequenceName)")
{
   return object->setArmThreadPlayOnce(argv[2]);
}
ConsoleMethod( Player, SetArmThreadTransitionOnce, bool, 3, 3, "(string sequenceName)")
{
   return object->setArmThreadTransitionOnce(argv[2]);
}

// Step Four: player.h
// in call Player: Public, after:
TSThread* mArmThread;

// add
   // phdana hth play once arm thread->
   bool mArmThreadPlayOnce;   // if true then special case where we actaully PLAY arm thread
   U32 mArmThreadSavedAction; // arm thread that was active prior to play once thread
   // <- phdana hth play once arm thread
   
// further down is that, after:
Box3F          mWorkingQueryBox;

// add:
   // phdana hth play once arm thread->
   void startPlayOnce(F32 timeScale);
   void stopPlayOnce();
   bool setArmThreadPlayOnce(const char* sequence);
   // phdana stun ->
   bool setArmThreadTransitionOnce(const char* sequence);
   // <- phdana stun
   void advancePlayOnceTime(F32 dt);

   // <- phdana hth play once arm thread

// Step Five: shapeBase.h
// in struct ShapeBaseImageData: public, after;
S32 fireState;                   ///< The ID of the fire state.

// add:
   // phdana
   S32  damageStartNode;            // PC: the node which is used to start the damage raycast
   S32  damageEndNode;              // the end node to raycast to.. anything between start and end
                                    // is damaged.
   // phdana
   
// Then in class ShapeBase : public GameBase, after;
virtual void onImpact(VectorF vec);

// add:
virtual void UpdateImageRaycastDamage(F32 dt, U32 imageSlot );

// further down, after:
const char* getSkinName();

// add:
   // PC server id for this ghost? should be a base method somewehere.
   // phdana
   S32 mShapeServerId;
   void setShapeServerId(S32 Id);
   S32 getShapeServerId(){ return mShapeServerId; };
   // phdana
   
// then after
SoundMaskN      = Parent::NextFreeMask << 8,       ///< Extends + MaxSoundThreads bits
// add
ServerIdMask    = Parent::NextFreeMask << 9,

// Step Six: shapeBase.cc
// in shapebase::shapebase(), after:
mAppliedForce.set(0, 0, 0);

// add:
mShapeServerId = 0;

// in ShapeBase::pack update, replace:
if(!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask |
         ThreadMask | ImageMask | CloakMask | MountedMask | InvincibleMask |
         ShieldMask | SkinMask))
      return retMask;
      
// with:
if(!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask |
         ThreadMask | ImageMask | CloakMask | MountedMask | InvincibleMask |
         ShieldMask | SkinMask | ServerIdMask)))
      return retMask;
// further down, replace:
// Group some of the uncommon stuff together.
   if (stream->writeFlag(mask & (NameMask | ShieldMask | CloakMask | InvincibleMask | SkinMask))) {
      if (stream->writeFlag(mask & CloakMask)) {
// with:
// Group some of the uncommon stuff together.
   if (stream->writeFlag(mask & (NameMask | ShieldMask | CloakMask | InvincibleMask | SkinMask | ServerIdMask ))) {
      if (stream->writeFlag(mask & CloakMask)) {
// down a few lines, after:

// piggyback control update
stream->writeFlag(bool(getControllingClient()));

// add:
         // PC: added ServerIdMask and support code to send ServerId value.
        if (stream->writeFlag(mask & ServerIdMask)) {
            stream->writeInt(mShapeServerId,32);
        }
// now in uppackupdate, after:
setCloakedState(stream->readFlag());
         mIsControlled = stream->readFlag();
// add:
         // PC: read team id value (8 bits signed integer)
         if(stream->readFlag())
         {
            mShapeServerId = stream->readInt(32);
         }
// now, add this before shapebase::consoleinti()

//--------------------------------------------------------------------------
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

// in shapebase::processTick, after:
updateServerAudio();

// add:
      // phdana hth server side axe ->
      // this function was previously only called on client side
      for (int i = 0; i < MaxMountedImages; i++)
         if (mMountedImageList[i].dataBlock)
            UpdateImageRaycastDamage( TickSec, i );
      // <- phdana hth server side axe
      
// Step Seven: ShapeImage.cc
// this is very IMPORTANT, with out this code the engine will crash
// in function shapeimage::preload, after BEFORE
// Resolve nodes & build mount transform
      ejectNode = shape->findNode("ejectPoint");
      muzzleNode = shape->findNode("muzzlePoint");
// ADD
     // PC: find our damage location nodes..
      damageStartNode = shape->findNode("damageStart");
      damageEndNode = shape->findNode("damageEnd");
      // RW ----------------------
// Add this functions to the bottom of the file.
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
