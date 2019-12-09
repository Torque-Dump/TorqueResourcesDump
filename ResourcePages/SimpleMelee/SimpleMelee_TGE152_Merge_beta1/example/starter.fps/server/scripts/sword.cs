//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
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
   shapeFile = "~/data/shapes/sword/rune_blade01.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;

	// Dynamic properties defined by the scripts
	pickUpName = "a sword";
	image = SwordImage;

	itemType= "melee";
};


//--------------------------------------------------------------------------
// Sword image which does all the work.  Images do not normally exist in
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
   customLookAnim = "looknw";

   // Here are the Attacks we support
   hthNumAttacks = 3;
   hthAttack[0]                     = OneHandedAttackSwing;
   hthAttack[1]                     = OneHandedAttackSlice;
   hthAttack[2]                     = OneHandedAttackThrust;

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
 
