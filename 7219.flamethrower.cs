//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Flamethrower weapon. This file contains all the items related to this weapon
// including explosions, ammo, the item and the weapon item image.
// These objects rely on the item & inventory support system defined
// in item.cs and inventory.cs
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sounds profiles

datablock AudioProfile(FlamethrowerReloadSound)
{
filename = "~/data/sound/crossbow_reload.ogg";
description = AudioClose3d;
preload = true;
};

datablock AudioProfile(FlamethrowerFireSound)
{
filename = "~/data/sound/flamethrowerwoosh.wav";
description = AudioClose3d;
preload = true;
};

datablock AudioProfile(FlamethrowerFireEmptySound)
{
filename = "~/data/sound/crossbow_firing_empty.ogg";
description = AudioClose3d;
preload = true;
};

datablock AudioProfile(FlamethrowerExplosionSound)
{
filename = "~/data/sound/crossbow_explosion.ogg";
description = AudioDefault3d;
preload = true;
};

//-----------------------------------------------------------------------------
// Flamethrower bolt projectile splash

datablock ParticleData(FlamethrowerSplashMist)
{
   dragCoefficient      = 2.0;
   gravityCoefficient   = -0.05;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 400;
   lifetimeVarianceMS   = 100;
   useInvAlpha          = false;
   spinRandomMin        = -90.0;
   spinRandomMax        = 500.0;
   textureName          = "~/data/shapes/crossbow/splash";
   
   colors[0]     = "0.7 0.8 1.0 1.0";
   colors[1]     = "0.7 0.8 1.0 0.5";
   colors[2]     = "0.7 0.8 1.0 0.0";
   
   sizes[0]      = 0.5;
   sizes[1]      = 0.5;
   sizes[2]      = 0.8;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerSplashMistEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 2.0;
   ejectionOffset   = 0.0;
   thetaMin         = 85;
   thetaMax         = 85;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   lifetimeMS       = 250;
   particles = "FlamethrowerSplashMist";
};

datablock ParticleData( FlamethrowerSplashParticle )
{
   dragCoefficient      = 1;
   gravityCoefficient   = 0.2;
   inheritedVelFactor   = 0.2;
   constantAcceleration = -0.0;
   lifetimeMS           = 600;
   lifetimeVarianceMS   = 0;
   colors[0]     = "0.7 0.8 1.0 1.0";
   colors[1]     = "0.7 0.8 1.0 0.5";
   colors[2]     = "0.7 0.8 1.0 0.0";
   sizes[0]      = 0.5;
   sizes[1]      = 0.5;
   sizes[2]      = 0.5;
   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData( FlamethrowerSplashEmitter )
{
   ejectionPeriodMS = 1;
   periodVarianceMS = 0;
   ejectionVelocity = 3;
   velocityVariance = 1.0;
   ejectionOffset   = 0.0;
   thetaMin         = 60;
   thetaMax         = 80;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvance = false;
   orientParticles  = true;
   lifetimeMS       = 100;
   particles = "FlamethrowerSplashParticle";
};

datablock SplashData(FlamethrowerSplash)
{
   numSegments = 15;
   ejectionFreq = 15;
   ejectionAngle = 40;
   ringLifetime = 0.5;
   lifetimeMS = 300;
   velocity = 4.0;
   startRadius = 0.0;
   acceleration = -3.0;
   texWrap = 5.0;

   texture = "~/data/shapes/crossbow/splash";

   emitter[0] = FlamethrowerSplashEmitter;
   emitter[1] = FlamethrowerSplashMistEmitter;

   colors[0] = "0.7 0.8 1.0 0.0";
   colors[1] = "0.7 0.8 1.0 0.3";
   colors[2] = "0.7 0.8 1.0 0.7";
   colors[3] = "0.7 0.8 1.0 0.0";
   times[0] = 0.0;
   times[1] = 0.4;
   times[2] = 0.8;
   times[3] = 1.0;
};

//-----------------------------------------------------------------------------
// Flamethrower bolt projectile particles

datablock ParticleData(FlamethrowerBoltParticle)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoefficient     = 0.0;
   gravityCoefficient   = 0.2; 
   inheritedVelFactor   = 0.0;
   lifetimeMS           = 550;
   lifetimeVarianceMS   = 50;   // ...more or less
   useInvAlpha = false;
   spinRandomMin = -30.0;
   spinRandomMax = 30.0;

   colors[0]     = "0.5 0.4 0.1 0.3";
   colors[1]     = "0.6 0.4 0.0 0.2";
   colors[2]     = "0.4 0.3 0.0 0";

   sizes[0]      = 0.01;
   sizes[1]      = 4.00;
   sizes[2]      = 0.05;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleData(FlamethrowerBubbleParticle)
{
   textureName          = "~/data/shapes/particles/bubble";
   dragCoefficient      = 0.0;
   gravityCoefficient   = -0.25;   // rises slowly
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 1500;
   lifetimeVarianceMS   = 600;    // ...more or less
   useInvAlpha          = false;
   spinRandomMin        = -100.0;
   spinRandomMax        = 100.0;

   colors[0]     = "0.7 0.8 1.0 0.4";
   colors[1]     = "0.7 0.8 1.0 1.0";
   colors[2]     = "0.7 0.8 1.0 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.2;
   sizes[2]      = 0.2;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerBoltEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;

   ejectionVelocity = 0.0;
   velocityVariance = 0.10;

   thetaMin         = 0.0;
   thetaMax         = 90.0;  
   
   
   particles = FlamethrowerBoltParticle;
   
   
};

datablock ParticleEmitterData(FlamethrowerBoltBubbleEmitter)
{
   ejectionPeriodMS = 9;
   periodVarianceMS = 0;

   ejectionVelocity = 1.0;
   ejectionOffset   = 0.1;
   velocityVariance = 0.5;

   thetaMin         = 0.0;
   thetaMax         = 80.0;

   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;  

   particles = FlamethrowerBubbleParticle;
};


//-----------------------------------------------------------------------------
// Explosion Debris

// Debris "spark" explosion
datablock ParticleData(FlamethrowerDebrisSpark)
{
   textureName          = "~/data/shapes/particles/fire";
   dragCoefficient      = 0;
   gravityCoefficient   = 0.0;
   windCoefficient      = 0;
   inheritedVelFactor   = 0.5;
   constantAcceleration = 0.0;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 50;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;
   useInvAlpha   = false;

   colors[0]     = "0.8 0.2 0 1.0";
   colors[1]     = "0.8 0.2 0 1.0";
   colors[2]     = "0 0 0 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.3;
   sizes[2]      = 0.1;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerDebrisSparkEmitter)
{
   ejectionPeriodMS = 20;
   periodVarianceMS = 0;
   ejectionVelocity = 0.5;
   velocityVariance = 0.25;
   ejectionOffset   = 0.0;
   thetaMin         = 0;
   thetaMax         = 90;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   orientParticles  = false;
   lifetimeMS       = 300;
   particles = "FlamethrowerDebrisSpark";
};

datablock ExplosionData(FlamethrowerDebrisExplosion)
{
   emitter[0] = FlamethrowerDebrisSparkEmitter;

   // Turned off..
   shakeCamera = false;
   impulseRadius = 0;
   lightStartRadius = 0;
   lightEndRadius = 0;
};

// Debris smoke trail
datablock ParticleData(FlamethrowerDebrisTrail)
{
   textureName          = "~/data/shapes/particles/fire";
   dragCoefficient      = 1;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 800;
   lifetimeVarianceMS   = 100;
   spinSpeed     = 0;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;
   useInvAlpha   = true;

   colors[0]     = "0.8 0.3 0.0 1.0";
   colors[1]     = "0.1 0.1 0.1 0.7";
   colors[2]     = "0.1 0.1 0.1 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.3;
   sizes[2]      = 0.4;

   times[0]      = 0.1;
   times[1]      = 0.2;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerDebrisTrailEmitter)
{
   ejectionPeriodMS = 30;
   periodVarianceMS = 0;
   ejectionVelocity = 0.0;
   velocityVariance = 0.0;
   ejectionOffset   = 0.0;
   thetaMin         = 170;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 5000;
   particles = "FlamethrowerDebrisTrail";
};

// Debris object
datablock DebrisData(FlamethrowerExplosionDebris)
{
   shapeFile = "~/data/shapes/crossbow/debris.dts";
   emitters = "FlamethrowerDebrisTrailEmitter";
   explosion = FlamethrowerDebrisExplosion;
   
   elasticity = 0.6;
   friction = 0.5;
   numBounces = 1;
   bounceVariance = 1;
   explodeOnMaxBounce = true;
   staticOnMaxBounce = false;
   snapOnMaxBounce = false;
   minSpinSpeed = 0;
   maxSpinSpeed = 700;
   render2D = false;
   lifetime = 4;
   lifetimeVariance = 0.4;
   velocity = 5;
   velocityVariance = 0.5;
   fade = false;
   useRadiusMass = true;
   baseRadius = 0.3;
   gravModifier = 0.5;
   terminalVelocity = 6;
   ignoreWater = true;
};


//-----------------------------------------------------------------------------
// Bolt Explosion

datablock ParticleData(FlamethrowerExplosionSmoke)
{
   textureName          = "~/data/shapes/particles/smoke";
   dragCoeffiecient     = 100.0;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0.25;
   constantAcceleration = -0.30;
   lifetimeMS           = 1200;
   lifetimeVarianceMS   = 300;
   useInvAlpha =  true;
   spinRandomMin = -80.0;
   spinRandomMax =  80.0;

   colors[0]     = "0.56 0.36 0.26 1.0";
   colors[1]     = "0.2 0.2 0.2 1.0";
   colors[2]     = "0.0 0.0 0.0 0.0";

   sizes[0]      = 4.0;
   sizes[1]      = 2.5;
   sizes[2]      = 1.0;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleData(FlamethrowerExplosionBubble)
{
   textureName          = "~/data/shapes/particles/bubble";
   dragCoeffiecient     = 0.0;
   gravityCoefficient   = -0.25;
   inheritedVelFactor   = 0.0;
   constantAcceleration = 0.0;
   lifetimeMS           = 1500;
   lifetimeVarianceMS   = 600;
   useInvAlpha          = false;
   spinRandomMin        = -100.0;
   spinRandomMax        =  100.0;

   colors[0]     = "0.7 0.8 1.0 0.4";
   colors[1]     = "0.7 0.8 1.0 0.4";
   colors[2]     = "0.7 0.8 1.0 0.0";

   sizes[0]      = 0.3;
   sizes[1]      = 0.3;
   sizes[2]      = 0.3;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerExplosionSmokeEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 4;
   velocityVariance = 0.5;
   thetaMin         = 0.0;
   thetaMax         = 180.0;
   lifetimeMS       = 250;
   particles = "FlamethrowerExplosionSmoke";
};

datablock ParticleEmitterData(FlamethrowerExplosionBubbleEmitter)
{
   ejectionPeriodMS = 9;
   periodVarianceMS = 0;
   ejectionVelocity = 1;
   ejectionOffset   = 0.1;
   velocityVariance = 0.5;
   thetaMin         = 0.0;
   thetaMax         = 80.0;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   particles = "FlamethrowerExplosionBubble";
};

datablock ParticleData(FlamethrowerExplosionFire)
{
   textureName          = "~/data/shapes/particles/fire";
   dragCoeffiecient     = 100.0;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0.25;
   constantAcceleration = 0.1;
   lifetimeMS           = 1200;
   lifetimeVarianceMS   = 300;
   useInvAlpha =  false;
   spinRandomMin = -80.0;
   spinRandomMax =  80.0;

   colors[0]     = "0.8 0.4 0 0.8";
   colors[1]     = "0.2 0.0 0 0.8";
   colors[2]     = "0.0 0.0 0.0 0.0";

   sizes[0]      = 1.5;
   sizes[1]      = 0.9;
   sizes[2]      = 0.5;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerExplosionFireEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 0.8;
   velocityVariance = 0.5;
   thetaMin         = 0.0;
   thetaMax         = 180.0;
   lifetimeMS       = 250;
   particles = "FlamethrowerExplosionFire";
};

datablock ParticleData(FlamethrowerExplosionSparks)
{
   textureName          = "~/data/shapes/particles/spark";
   dragCoefficient      = 1;
   gravityCoefficient   = 0.0;
   inheritedVelFactor   = 0.2;
   constantAcceleration = 0.0;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 350;

   colors[0]     = "0.60 0.40 0.30 1.0";
   colors[1]     = "0.60 0.40 0.30 1.0";
   colors[2]     = "1.0 0.40 0.30 0.0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.15;
   sizes[2]      = 0.15;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleData(FlamethrowerExplosionWaterSparks)
{
   textureName          = "~/data/shapes/particles/bubble";
   dragCoefficient      = 0;
   gravityCoefficient   = 0.0;
   inheritedVelFactor   = 0.2;
   constantAcceleration = 0.0;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 350;

   colors[0]     = "0.4 0.4 1.0 1.0";
   colors[1]     = "0.4 0.4 1.0 1.0";
   colors[2]     = "0.4 0.4 1.0 0.0";

   sizes[0]      = 0.5;
   sizes[1]      = 0.5;
   sizes[2]      = 0.5;

   times[0]      = 0.0;
   times[1]      = 0.5;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(FlamethrowerExplosionSparkEmitter)
{
   ejectionPeriodMS = 3;
   periodVarianceMS = 0;
   ejectionVelocity = 5;
   velocityVariance = 1;
   ejectionOffset   = 0.0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 100;
   particles = "FlamethrowerExplosionSparks";
};

datablock ParticleEmitterData(FlamethrowerExplosionWaterSparkEmitter)
{
   ejectionPeriodMS = 3;
   periodVarianceMS = 0;
   ejectionVelocity = 4;
   velocityVariance = 4;
   ejectionOffset   = 0.0;
   thetaMin         = 0;
   thetaMax         = 60;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 200;
   particles = "FlamethrowerExplosionWaterSparks";
};

datablock ExplosionData(FlamethrowerSubExplosion1)
{
   offset = 0;
   emitter[0] = FlamethrowerExplosionSmokeEmitter;
   emitter[1] = FlamethrowerExplosionSparkEmitter;
};

datablock ExplosionData(FlamethrowerSubExplosion2)
{
   offset = 1.0;
   emitter[0] = FlamethrowerExplosionSmokeEmitter;
   emitter[1] = FlamethrowerExplosionSparkEmitter;
};

datablock ExplosionData(FlamethrowerSubWaterExplosion1)
{
   delayMS   = 100;
   offset    = 1.2;
   playSpeed = 1.5;

   emitter[0] = FlamethrowerExplosionBubbleEmitter;
   emitter[1] = FlamethrowerExplosionWaterSparkEmitter;
   
   sizes[0] = "0.75 0.75 0.75";
   sizes[1] = "1.0 1.0 1.0";
   sizes[2] = "0.5 0.5 0.5";
   times[0] = 0.0;
   times[1] = 0.5;
   times[2] = 1.0;
};

datablock ExplosionData(FlamethrowerSubWaterExplosion2)
{
   delayMS   = 50;
   offset    = 1.2;
   playSpeed = 0.75;

   emitter[0] = FlamethrowerExplosionBubbleEmitter;
   emitter[1] = FlamethrowerExplosionWaterSparkEmitter;

   sizes[0] = "1.5 1.5 1.5";
   sizes[1] = "1.5 1.5 1.5";
   sizes[2] = "1.0 1.0 1.0";
   times[0] = 0.0;
   times[1] = 0.5;
   times[2] = 1.0;
};

datablock ExplosionData(FlamethrowerExplosion)
{
   soundProfile = FlamethrowerExplosionSound;
   lifeTimeMS = 1200;

   // Volume particles
   particleEmitter = FlamethrowerExplosionFireEmitter;
   particleDensity = 75;
   particleRadius = 2;

   // Point emission
   emitter[0] = FlamethrowerExplosionSmokeEmitter;
   emitter[1] = FlamethrowerExplosionSparkEmitter;

   // Sub explosion objects
   subExplosion[0] = FlamethrowerSubExplosion1;
   subExplosion[1] = FlamethrowerSubExplosion2;
   
   // Camera Shaking
   shakeCamera = true;
   camShakeFreq = "10.0 11.0 10.0";
   camShakeAmp = "1.0 1.0 1.0";
   camShakeDuration = 0.5;
   camShakeRadius = 10.0;

   // Exploding debris
   debris = FlamethrowerExplosionDebris;
   debrisThetaMin = 0;
   debrisThetaMax = 60;
   debrisPhiMin = 0;
   debrisPhiMax = 360;
   debrisNum = 6;
   debrisNumVariance = 2;
   debrisVelocity = 1;
   debrisVelocityVariance = 0.5;
   
   // Impulse
   impulseRadius = 10;
   impulseForce = 15;

   // Dynamic light
   lightStartRadius = 6;
   lightEndRadius = 3;
   lightStartColor = "0.5 0.5 0";
   lightEndColor = "0 0 0";
};

datablock ExplosionData(FlamethrowerWaterExplosion)
{
   soundProfile = FlamethrowerExplosionSound;

   // Volume particles
   particleEmitter = FlamethrowerExplosionBubbleEmitter;
   particleDensity = 375;
   particleRadius = 2;


   // Point emission
   emitter[0] = FlamethrowerExplosionBubbleEmitter;
   emitter[1] = FlamethrowerExplosionWaterSparkEmitter;

   // Sub explosion objects
   subExplosion[0] = FlamethrowerSubWaterExplosion1;
   subExplosion[1] = FlamethrowerSubWaterExplosion2;
   
   // Camera Shaking
   shakeCamera = true;
   camShakeFreq = "8.0 9.0 7.0";
   camShakeAmp = "3.0 3.0 3.0";
   camShakeDuration = 1.3;
   camShakeRadius = 20.0;

   // Exploding debris
   debris = FlamethrowerExplosionDebris;
   debrisThetaMin = 0;
   debrisThetaMax = 60;
   debrisPhiMin = 0;
   debrisPhiMax = 360;
   debrisNum = 6;
   debrisNumVariance = 2;
   debrisVelocity = 0.5;
   debrisVelocityVariance = 0.2;
   
   // Impulse
   impulseRadius = 10;
   impulseForce = 15;

   // Dynamic light
   lightStartRadius = 6;
   lightEndRadius = 3;
   lightStartColor = "0 0.5 0.5";
   lightEndColor = "0 0 0";
};

//-----------------------------------------------------------------------------
// Projectile Object

datablock ProjectileData(FlamethrowerProjectile)
{
 //  projectileShapeName = "~/data/shapes/crossbow/bullet.dts";
   directDamage        = 3;
   radiusDamage        = 3;
   damageRadius        = 1.5;

   particleEmitter     = FlamethrowerBoltEmitter;


   muzzleVelocity      = 20;
   velInheritFactor    = 1.0;

   armingDelay         = 10000;
   isfluid             = true;
   lifetime            = 2000;
   fadeDelay           = 1500;
   bounceElasticity    = 0.0;
   bounceFriction      = 0;
   isBallistic         = false;
   gravityMod = 0.20;

   //hasLight    = true;
   //lightRadius = 4;
   //lightColor  = "0.75 0.75 0.15";

   
};

function FlamethrowerProjectile::onCollision(%this,%obj,%col,%fade,%pos,%normal)
{

//echo(" we are flaming enemies");
   // Apply damage to the object all shape base objects
   if (%col.getType() & $TypeMasks::ShapeBaseObjectType)
      %col.damage(%obj,%pos,%this.directDamage,"FlamethrowerBolt");

	  //if it were possible it would be nice to start a fire here...that would be great....
	  //I might see if I can do that next....it does that now...
	  
	  
   // Radius damage is a support scripts defined in radiusDamage.cs
   // Push the contact point away from the contact surface slightly
   // along the contact normal to derive the explosion center. -dbs
//   radiusDamage
  //   (%obj, VectorAdd(%pos, VectorScale(%normal, 0.01)),
  //    %this.damageRadius,%this.radiusDamage,"Radius",40);
}


//-----------------------------------------------------------------------------
// Ammo Item

datablock ItemData(FlamethrowerAmmo)
{
   // Mission editor category
   category = "Ammo";

   // Add the Ammo namespace as a parent.  The ammo namespace provides
   // common ammo related functions and hooks into the inventory system.
   className = "Ammo";

   // Basic Item properties
   shapeFile = "~/data/shapes/crossbow/ammo.dts";
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;

	// Dynamic properties defined by the scripts
	pickUpName = "Flamethrower bolts";
   maxInventory = 20;
};


//--------------------------------------------------------------------------
// Weapon Item.  This is the item that exists in the world, i.e. when it's
// been dropped, thrown or is acting as re-spawnable item.  When the weapon
// is mounted onto a shape, the CrossbowImage is used.

datablock ItemData(Flamethrower)
{
   // Mission editor category
   category = "Weapon";

   // Hook into Item Weapon class hierarchy. The weapon namespace
   // provides common weapon handling functions in addition to hooks
   // into the inventory system.
   className = "Weapon";

   // Basic Item properties
   shapeFile = "~/data/shapes/AS_Test/gun.dts";  //for testing purposes
   mass = 1;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;

	// Dynamic properties defined by the scripts
	pickUpName = "a Flamethrower";
	image = FlamethrowerImage;
};


//--------------------------------------------------------------------------
// Flamethrower image which does all the work.  Images do not normally exist in
// the world, they can only be mounted on ShapeBase objects.

datablock ShapeBaseImageData(FlamethrowerImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/AS_Test/gun.dts"; //for testing purposes.
   emap = true;

   // Specify mount point & offset for 3rd person, and eye offset
   // for first person rendering.
   mountPoint = 0;
   eyeOffset = "0.1 0.4 -0.35";

   // When firing from a point offset from the eye, muzzle correction
   // will adjust the muzzle vector to point to the eye LOS point.
   // Since this weapon doesn't actually fire from the muzzle point,
   // we need to turn this off.  
   correctMuzzleVector = false;

   // Add the WeaponImage namespace as a parent, WeaponImage namespace
   // provides some hooks into the inventory system.
   className = "WeaponImage";

   // Projectile && Ammo.
   item = Flamethrower;
   ammo = FlamethrowerAmmo;
   projectile = FlamethrowerProjectile;
   projectileType = Projectile;

   // Images have a state system which controls how the animations
   // are run, which sounds are played, script callbacks, etc. This
   // state system is downloaded to the client so that clients can
   // predict state changes and animate accordingly.  The following
   // system supports basic ready->fire->reload transitions as
   // well as a no-ammo->dryfire idle state.

   // Initial start up state
   stateName[0]                     = "Preactivate";
   stateTransitionOnLoaded[0]       = "Activate";
   stateTransitionOnNoAmmo[0]       = "NoAmmo";

   // Activating the gun.  Called when the weapon is first
   // mounted and there is ammo.
   stateName[1]                     = "Activate";
   stateTransitionOnTimeout[1]      = "Ready";
   stateTimeoutValue[1]             = 0.6;
   stateSequence[1]                 = "Activate";

   // Ready to fire, just waiting for the trigger
   stateName[2]                     = "Ready";
   stateTransitionOnNoAmmo[2]       = "NoAmmo";
   stateTransitionOnTriggerDown[2]  = "Fire";

   // Fire the weapon. Calls the fire script which does 
   // the actual work.
   stateName[3]                     = "Fire";
   stateTransitionOnTimeout[3]      = "Reload";
   stateTimeoutValue[3]             = 0.1;
   stateFire[3]                     = true;
   stateRecoil[3]                   = LightRecoil;
   stateAllowImageChange[3]         = false;
   stateSequence[3]                 = "Fire";
   stateScript[3]                   = "onFire";
   stateSound[3]                    = FlamethrowerFireSound;

   // Play the relead animation, and transition into
   stateName[4]                     = "Reload";
   stateTransitionOnNoAmmo[4]       = "NoAmmo";
   stateTransitionOnTimeout[4]      = "Ready";
   stateTimeoutValue[4]             = 0.1;
   stateAllowImageChange[4]         = false;
   stateSequence[4]                 = "Reload";
   stateEjectShell[4]               = true;
   stateSound[4]                    = FlamethrowerReloadSound;

   // No ammo in the weapon, just idle until something
   // shows up. Play the dry fire sound if the trigger is
   // pulled.
   stateName[5]                     = "NoAmmo";
   stateTransitionOnAmmo[5]         = "Reload";
   stateSequence[5]                 = "NoAmmo";
   stateTransitionOnTriggerDown[5]  = "DryFire";

   // No ammo dry fire
   stateName[6]                     = "DryFire";
   stateTimeoutValue[6]             = 1.0;
   stateTransitionOnTimeout[6]      = "NoAmmo";
   stateSound[6]                    = FlamethrowerFireEmptySound;
};


//-----------------------------------------------------------------------------

function FlamethrowerImage::onFire(%this, %obj, %slot)
{
   %projectile = %this.projectile;

   // Decrement inventory ammo. The image's ammo state is update
   // automatically by the ammo inventory hooks.
//   %obj.decInventory(%this.ammo,1);

   // Determin initial projectile velocity based on the 
   // gun's muzzle point and the object's current velocity
   
   %muzzleVector = %obj.getMuzzleVector(%slot);
   
   %objectVelocity = %obj.getVelocity();
   %muzzleVelocity = VectorAdd(
      VectorScale(%muzzleVector, %projectile.muzzleVelocity),
      VectorScale(%objectVelocity, %projectile.velInheritFactor));

   // Create the projectile object
   %p = new (%this.projectileType)() {
      dataBlock        = %projectile;
      initialVelocity  = %muzzleVelocity;
      initialPosition  = %obj.getMuzzlePoint(%slot);
      sourceObject     = %obj;
      sourceSlot       = %slot;
      client           = %obj.client;
   };
   MissionCleanup.add(%p);
   return %p;
}
