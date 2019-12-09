//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


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
   shapeFile = "art/shapes/sword/rune_blade01.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;

   // Dynamic properties defined by the scripts
   pickUpName = "a sword";
   image = SwordImage;

   itemType = "melee";
   trayIcon = "sword01.png";

   //Set the max distance a bot will fire this weapon from
   ignoreDistance = 5;
   //Set the min distance a bot will fire this weapon from
   minIgnoreDistance = 0;
   //Set if this weapon uses ammo or not
   usesAmmo = false;
   //Sets how long the bot waits between firing bursts
   fireDelay = 100;
   //Sets how long the bot holds down the trigger when firing. Use longer pulses for spray and pray type weapons.
   triggerDown = 1;
   //Sets the relative quality score of the weapon, how good is the weapon?
   //This should be set to a different value for every weapon.
   weapRating = 5;
   //AISK Changes: End
};


//--------------------------------------------------------------------------
// Crossbow image which does all the work.  Images do not normally exist in
// the world, they can only be mounted on ShapeBase objects.

datablock ShapeBaseImageData(SwordImage)
{
   // Basic Item properties
   shapeFile = "art/shapes/sword/rune_blade01.dts";
   emap = true;

   // Specify mount point & offset for 3rd person
   // Change eyeOffset to "0 0 0" for 1st person
   mountPoint = 0;
   //eyeOffset = "0.1 0.4 -0.6";

   // When firing from a point offset from the eye, muzzle correction
   // will adjust the muzzle vector to point to the eye LOS point.
   // Since this weapon doesn't actually fire from the muzzle point,
   // we need to turn this off.
   correctMuzzleVector = true;

   // Add the WeaponImage namespace as a parent, WeaponImage namespace
   // provides some hooks into the inventory system.
   className = "WeaponImage";

   // Projectile && Ammo.
   item = Sword;
   ammo = CrossbowAmmo;
   projectile = CrossbowProjectile;
   projectileType = Projectile;
   // we are a HAND TO HAND weapon so we have a custom look anim
   customLookAnim = "h1root";  // as a test
   //customLookAnim = "looknw";

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
   stateTimeoutValue[1]             = 0.1;
   //stateSequence[1]                 = "Activate";

   // Ready to fire, just waiting for the trigger
   stateName[2]                     = "Ready";
   stateTransitionOnTriggerDown[2]  = "Fire";

   // Fire the weapon. Calls the fire script which does the actual work.
   stateName[3]                     = "Fire";
   stateTransitionOnTimeout[3]      = "Reload";
   stateTimeoutValue[3]             = 0.8;
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

// Mount the sword if one is picked up
function Sword::onCollision(%this, %obj, %col)
{
   %col.mountImage(SwordImage,0);
}

//-----------------------------------------------------------------------------
// Ammo Item

datablock ItemData(SwordAmmo)
{
   // Mission editor category
   category = "Ammo";

   // Add the Ammo namespace as a parent.  The ammo namespace provides
   // common ammo related functions and hooks into the inventory system.
   className = "Ammo";

   // Basic Item properties
   shapeFile = "art/shapes/weapons/crossbow/ammo.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;

   // Dynamic properties defined by the scripts
   pickUpName = "sword ammo";
   maxInventory = 50;
};
