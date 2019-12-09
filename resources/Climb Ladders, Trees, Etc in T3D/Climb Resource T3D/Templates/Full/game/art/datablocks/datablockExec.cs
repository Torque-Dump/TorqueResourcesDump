//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Load up all datablocks.  This function is called when
// a server is constructed.
exec("./audioProfiles.cs");
exec("./chimneyfire.cs");
exec("./environment.cs");
exec("./ClimbableObjects.cs"); //Climb Resource
exec("./camera.cs");
exec("./markers.cs"); 
exec("./triggers.cs");
exec("./rigidShape.cs");
exec("./fxlights.cs");
exec("./car.cs");

// Load our supporting weapon datablocks
exec("./weapons/weapon.cs");
   
// Load our weapon datablocks
// Note: if you add a new weapon and you want it
// to show up in the weapon picker then be sure to add
// your exec's to loadWeaponPickerData() over in
// <$defaultGame>/client/init.cs
exec("./weapons/SwarmGun.cs");
exec("./weapons/crossbow.cs");
   
// Load our default player datablocks
exec("./players/player.cs");

// Load our other player datablocks
// Note: if you add a new PlayerData and you want it
// to show up in the Player picker then be sure to add
// your exec's to loadPlayerPickerData() over in
// <$defaultGame>/client/init.cs
exec("./players/BoomBot.cs");
