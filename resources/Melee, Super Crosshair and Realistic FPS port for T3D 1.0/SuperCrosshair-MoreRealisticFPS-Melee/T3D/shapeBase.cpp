//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/shapeBase.h"

#include "core/dnet.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxProfile.h"
#include "T3D/gameConnection.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "ts/tsPartInstance.h"
#include "ts/tsShapeInstance.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "T3D/fx/explosion.h"
#include "T3D/fx/particleEmitter.h"
#include "T3D/fx/cameraFXMgr.h"
#include "environment/waterBlock.h"
#include "T3D/debris.h"
#include "T3D/physicalZone.h"
#include "T3D/containerQuery.h"
#include "math/mathUtils.h"
#include "math/mMatrix.h"
#include "math/mRandom.h"
#include "platform/profiler.h"
#include "gfx/gfxCubemap.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "collision/earlyOutPolyList.h"
#include "core/resourceManager.h"
#include "sceneGraph/reflectionManager.h"
#include "gfx/sim/cubemapData.h"
#include "materials/materialManager.h"
#include "materials/materialFeatureTypes.h"
#include "renderInstance/renderOcclusionMgr.h"
#include "core/stream/fileStream.h"

IMPLEMENT_CO_DATABLOCK_V1(ShapeBaseData);


//----------------------------------------------------------------------------
// Timeout for non-looping sounds on a channel
static SimTime sAudioTimeout = 500;
bool ShapeBase::gRenderEnvMaps = true;
F32  ShapeBase::sWhiteoutDec = 0.007f;
F32  ShapeBase::sDamageFlashDec = 0.007f;
U32  ShapeBase::sLastRenderFrame = 0;

static const char *sDamageStateName[] =
{
   // Index by enum ShapeBase::DamageState
   "Enabled",
   "Disabled",
   "Destroyed"
};


//----------------------------------------------------------------------------

ShapeBaseData::ShapeBaseData()
 : shadowEnable( false ),
   shadowSize( 128 ),
   shadowMaxVisibleDistance( 80.0f ),
   shadowProjectionDistance( 10.0f ),
   shadowSphereAdjust( 1.0f ),
   shapeName( StringTable->insert("") ),
   cloakTexName( StringTable->insert("") ),
   mass( 1.0f ),
   drag( 0.0f ),
   density( 1.0f ),
   maxEnergy( 0.0f ),
   maxDamage( 1.0f ),
   disabledLevel( 1.0f ),
   destroyedLevel( 1.0f ),
   repairRate( 0.0033f ),
   eyeNode( -1 ),
   cameraNode( -1 ),
   damageSequence( -1 ),
   hulkSequence( -1 ),
   cameraMaxDist( 0.0f ),
   cameraMinDist( 0.2f ),
   cameraDefaultFov( 90.0f ),
   cameraMinFov( 5.0f ),
   cameraMaxFov( 120.f ),
   aiAvoidThis( false ),
   isInvincible( false ),
   renderWhenDestroyed( true ),
   debris( NULL ),
   debrisID( 0 ),
   debrisShapeName( StringTable->insert("") ),
   explosion( NULL ),
   explosionID( 0 ),
   underwaterExplosion( NULL ),
   underwaterExplosionID( 0 ),
   firstPersonOnly( false ),
   useEyePoint( false ),
   reflectorDesc( NULL ),
   observeThroughObject( false ),
   computeCRC( false ),
   inheritEnergyFromMount( false ),
   mCRC( 0 ),
   debrisDetail( -1 )
{      
   dMemset( hudRenderCenter, 0, sizeof( bool ) * NumHudRenderImages );
   dMemset( hudRenderModulated, 0, sizeof( bool ) * NumHudRenderImages );
   dMemset( hudRenderAlways, 0, sizeof( bool ) * NumHudRenderImages );
   dMemset( hudRenderDistance, 0, sizeof( bool ) * NumHudRenderImages );
   dMemset( hudRenderName, 0, sizeof( bool ) * NumHudRenderImages );

   StringTableEntry emptyStr = StringTable->insert("");

   for(U32 j = 0; j < NumHudRenderImages; j++)
   {
      hudImageNameFriendly[j] = emptyStr;
      hudImageNameEnemy[j] = emptyStr;
   }

   dMemset( mountPointNode, -1, sizeof( S32 ) * NumMountPoints );
}

struct ShapeBaseDataProto
{
   F32 mass;
   F32 drag;
   F32 density;
   F32 maxEnergy;
   F32 cameraMaxDist;
   F32 cameraMinDist;
   F32 cameraDefaultFov;
   F32 cameraMinFov;    
   F32 cameraMaxFov;    


   ShapeBaseDataProto()
   {
      mass = 1;
      drag = 0;
      density = 1;
      maxEnergy = 0;
      cameraMaxDist = 0;
      cameraMinDist = 0.2f;
      cameraDefaultFov = 90.f;
      cameraMinFov = 5.0f;
      cameraMaxFov = 120.f;
   }
};

static ShapeBaseDataProto gShapeBaseDataProto;

ShapeBaseData::~ShapeBaseData()
{

}

bool ShapeBaseData::preload(bool server, String &errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;
   bool shapeError = false;

   // Resolve objects transmitted from server
   if (!server) {

      if( !explosion && explosionID != 0 )
      {
         if( Sim::findObject( explosionID, explosion ) == false)
         {
            Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(explosion): 0x%x", explosionID );
         }
         AssertFatal(!(explosion && ((explosionID < DataBlockObjectIdFirst) || (explosionID > DataBlockObjectIdLast))),
            "ShapeBaseData::preload: invalid explosion data");
      }

      if( !underwaterExplosion && underwaterExplosionID != 0 )
      {
         if( Sim::findObject( underwaterExplosionID, underwaterExplosion ) == false)
         {
            Con::errorf( ConsoleLogEntry::General, "ShapeBaseData::preload: Invalid packet, bad datablockId(underwaterExplosion): 0x%x", underwaterExplosionID );
         }
         AssertFatal(!(underwaterExplosion && ((underwaterExplosionID < DataBlockObjectIdFirst) || (underwaterExplosionID > DataBlockObjectIdLast))),
            "ShapeBaseData::preload: invalid underwaterExplosion data");
      }

      if( !debris && debrisID != 0 )
      {
         Sim::findObject( debrisID, debris );
         AssertFatal(!(debris && ((debrisID < DataBlockObjectIdFirst) || (debrisID > DataBlockObjectIdLast))),
            "ShapeBaseData::preload: invalid debris data");
      }


      if( debrisShapeName && debrisShapeName[0] != '\0' && !bool(debrisShape) )
      {
         debrisShape = ResourceManager::get().load(debrisShapeName);
         if( bool(debrisShape) == false )
         {
            errorStr = String::ToString("ShapeBaseData::load: Couldn't load shape \"%s\"", debrisShapeName);
            return false;
         }
         else
         {
            if(!server && !debrisShape->preloadMaterialList(debrisShape.getPath()) && NetConnection::filesWereDownloaded())
               shapeError = true;

            TSShapeInstance* pDummy = new TSShapeInstance(debrisShape, !server);
            delete pDummy;
         }
      }
   }

   //
   if (shapeName && shapeName[0]) {
      S32 i;

      // Resolve shapename
      mShape = ResourceManager::get().load(shapeName);
      if (bool(mShape) == false)
      {
         errorStr = String::ToString("ShapeBaseData: Couldn't load shape \"%s\"",shapeName);
         return false;
      }
      if(!server && !mShape->preloadMaterialList(mShape.getPath()) && NetConnection::filesWereDownloaded())
         shapeError = true;

      if(computeCRC)
      {
         Con::printf("Validation required for shape: %s", shapeName);

         Torque::FS::FileNodeRef    fileRef = Torque::FS::GetFileNode(mShape.getPath());

         if (!fileRef)
            return false;

         if(server)
            mCRC = fileRef->getChecksum();
         else if(mCRC != fileRef->getChecksum())
         {
            errorStr = String::ToString("Shape \"%s\" does not match version on server.",shapeName);
            return false;
         }
      }
      // Resolve details and camera node indexes.
      static const String sCollisionStr( "collision-" );

      for (i = 0; i < mShape->details.size(); i++)
      {
         const String &name = mShape->names[mShape->details[i].nameIndex];

         if (name.compare( sCollisionStr, sCollisionStr.length(), String::NoCase ) == 0)
         {
            collisionDetails.push_back(i);
            collisionBounds.increment();

            mShape->computeBounds(collisionDetails.last(), collisionBounds.last());
            mShape->getAccelerator(collisionDetails.last());

            if (!mShape->bounds.isContained(collisionBounds.last()))
            {
               Con::warnf("Warning: shape %s collision detail %d (Collision-%d) bounds exceed that of shape.", shapeName, collisionDetails.size() - 1, collisionDetails.last());
               collisionBounds.last() = mShape->bounds;
            }
            else if (collisionBounds.last().isValidBox() == false)
            {
               Con::errorf("Error: shape %s-collision detail %d (Collision-%d) bounds box invalid!", shapeName, collisionDetails.size() - 1, collisionDetails.last());
               collisionBounds.last() = mShape->bounds;
            }

            // The way LOS works is that it will check to see if there is a LOS detail that matches
            // the the collision detail + 1 + MaxCollisionShapes (this variable name should change in
            // the future). If it can't find a matching LOS it will simply use the collision instead.
            // We check for any "unmatched" LOS's further down
            LOSDetails.increment();

            String   buff = String::ToString("LOS-%d", i + 1 + MaxCollisionShapes);
            U32 los = mShape->findDetail(buff);
            if (los == -1)
               LOSDetails.last() = i;
            else
               LOSDetails.last() = los;
         }
      }

      // Snag any "unmatched" LOS details
      static const String sLOSStr( "LOS-" );

      for (i = 0; i < mShape->details.size(); i++)
      {
         const String &name = mShape->names[mShape->details[i].nameIndex];

         if (name.compare( sLOSStr, sLOSStr.length(), String::NoCase ) == 0)
         {
            // See if we already have this LOS
            bool found = false;
            for (U32 j = 0; j < LOSDetails.size(); j++)
            {
               if (LOSDetails[j] == i)
               {
                     found = true;
                     break;
               }
            }

            if (!found)
               LOSDetails.push_back(i);
         }
      }

      debrisDetail = mShape->findDetail("Debris-17");
      eyeNode = mShape->findNode("eye");
      cameraNode = mShape->findNode("cam");
      if (cameraNode == -1)
         cameraNode = eyeNode;

      // Resolve mount point node indexes
      for (i = 0; i < NumMountPoints; i++) {
         char fullName[256];
         dSprintf(fullName,sizeof(fullName),"mount%d",i);
         mountPointNode[i] = mShape->findNode(fullName);
      }

        // find the AIRepairNode - hardcoded to be the last node in the array...
      mountPointNode[AIRepairNode] = mShape->findNode("AIRepairNode");

      //
      hulkSequence = mShape->findSequence("Visibility");
      damageSequence = mShape->findSequence("Damage");

      //
      F32 w = mShape->bounds.len_y() / 2;
      if (cameraMaxDist < w)
         cameraMaxDist = w;
   }

   if(!server)
   {
/*
      // grab all the hud images
      for(U32 i = 0; i < NumHudRenderImages; i++)
      {
         if(hudImageNameFriendly[i] && hudImageNameFriendly[i][0])
            hudImageFriendly[i] = TextureHandle(hudImageNameFriendly[i], BitmapTexture);

         if(hudImageNameEnemy[i] && hudImageNameEnemy[i][0])
            hudImageEnemy[i] = TextureHandle(hudImageNameEnemy[i], BitmapTexture);
      }
*/
   }

   // Resolve CubeReflectorDesc.
   if ( !server && cubeDescName.isNotEmpty() )
   {
      Sim::findObject( cubeDescName, reflectorDesc );
   }

   return !shapeError;
}


void ShapeBaseData::initPersistFields()
{
   addGroup( "Shadows" );

      addField( "shadowEnable", TypeBool, Offset(shadowEnable, ShapeBaseData) );
      addField( "shadowSize", TypeS32, Offset(shadowSize, ShapeBaseData) );
      addField( "shadowMaxVisibleDistance", TypeF32, Offset(shadowMaxVisibleDistance, ShapeBaseData) );
      addField( "shadowProjectionDistance", TypeF32, Offset(shadowProjectionDistance, ShapeBaseData) );
      addField( "shadowSphereAdjust", TypeF32, Offset(shadowSphereAdjust, ShapeBaseData) );

   endGroup( "Shadows" );


   addGroup("Render");

      addField( "shapeFile",      TypeFilename, Offset(shapeName,      ShapeBaseData) );

   endGroup("Render");

   addGroup( "Destruction", "Parameters related to the destruction effects of this object." );

      addField( "explosion",      TypeExplosionDataPtr, Offset(explosion, ShapeBaseData) );
      addField( "underwaterExplosion", TypeExplosionDataPtr, Offset(underwaterExplosion, ShapeBaseData) );
      addField( "debris",         TypeDebrisDataPtr,    Offset(debris,   ShapeBaseData) );
      addField( "renderWhenDestroyed",   TypeBool,  Offset(renderWhenDestroyed,   ShapeBaseData) );
      addField( "debrisShapeName", TypeFilename,  Offset(debrisShapeName, ShapeBaseData) );

   endGroup( "Destruction" );

   addGroup( "Physics" );

      addField( "mass",           TypeF32,        Offset(mass,           ShapeBaseData) );
      addField( "drag",           TypeF32,        Offset(drag,           ShapeBaseData) );
      addField( "density",        TypeF32,        Offset(density,        ShapeBaseData) );

   endGroup( "Physics" );

   addGroup( "Damage/Energy" );

      addField( "maxEnergy",      TypeF32,        Offset(maxEnergy,      ShapeBaseData) );
      addField( "maxDamage",      TypeF32,        Offset(maxDamage,      ShapeBaseData) );
      addField( "disabledLevel",  TypeF32,        Offset(disabledLevel,  ShapeBaseData) );
      addField( "destroyedLevel", TypeF32,        Offset(destroyedLevel, ShapeBaseData) );
      addField( "repairRate",     TypeF32,        Offset(repairRate,     ShapeBaseData) );
      addField( "inheritEnergyFromMount", TypeBool, Offset(inheritEnergyFromMount, ShapeBaseData) );
      addField( "isInvincible",   TypeBool,       Offset(isInvincible,   ShapeBaseData) );

   endGroup( "Damage/Energy" );

   addGroup( "Camera" );

      addField( "cameraMaxDist",  TypeF32,        Offset(cameraMaxDist,  ShapeBaseData) );
      addField( "cameraMinDist",  TypeF32,        Offset(cameraMinDist,  ShapeBaseData) );
      addField( "cameraDefaultFov", TypeF32,      Offset(cameraDefaultFov, ShapeBaseData) );
      addField( "cameraMinFov",   TypeF32,        Offset(cameraMinFov,   ShapeBaseData) );
      addField( "cameraMaxFov",   TypeF32,        Offset(cameraMaxFov,   ShapeBaseData) );
      addField( "firstPersonOnly", TypeBool,      Offset(firstPersonOnly, ShapeBaseData) );
      addField( "useEyePoint",     TypeBool,      Offset(useEyePoint,     ShapeBaseData) );
      addField( "observeThroughObject", TypeBool, Offset(observeThroughObject, ShapeBaseData) );

   endGroup("Camera");

   // This hud code is going to get ripped out soon...
   addGroup( "HUD", "@deprecated Likely to be removed soon." );

      addField( "hudImageName",         TypeFilename,    Offset(hudImageNameFriendly, ShapeBaseData), NumHudRenderImages );
      addField( "hudImageNameFriendly", TypeFilename,    Offset(hudImageNameFriendly, ShapeBaseData), NumHudRenderImages );
      addField( "hudImageNameEnemy",    TypeFilename,    Offset(hudImageNameEnemy, ShapeBaseData), NumHudRenderImages );
      addField( "hudRenderCenter",      TypeBool,      Offset(hudRenderCenter, ShapeBaseData), NumHudRenderImages );
      addField( "hudRenderModulated",   TypeBool,      Offset(hudRenderModulated, ShapeBaseData), NumHudRenderImages );
      addField( "hudRenderAlways",      TypeBool,      Offset(hudRenderAlways, ShapeBaseData), NumHudRenderImages );
      addField( "hudRenderDistance",    TypeBool,      Offset(hudRenderDistance, ShapeBaseData), NumHudRenderImages );
      addField( "hudRenderName",        TypeBool,      Offset(hudRenderName, ShapeBaseData), NumHudRenderImages );

   endGroup("HUD");

   addGroup( "Misc" );

      addField( "aiAvoidThis",      TypeBool,        Offset(aiAvoidThis,    ShapeBaseData) );
      addField( "computeCRC",     TypeBool,       Offset(computeCRC,     ShapeBaseData) );      

   endGroup( "Misc" );

   addGroup("Reflection");

      addField( "cubeReflectorDesc", TypeRealString, Offset( cubeDescName, ShapeBaseData ) );      
      //addField( "reflectMaxRateMs", TypeS32, Offset( reflectMaxRateMs, ShapeBaseData ), "reflection will not be updated more frequently than this" );
      //addField( "reflectMaxDist", TypeF32, Offset( reflectMaxDist, ShapeBaseData ), "distance at which reflection is never updated" );
      //addField( "reflectMinDist", TypeF32, Offset( reflectMinDist, ShapeBaseData ), "distance at which reflection is always updated" );
      //addField( "reflectDetailAdjust", TypeF32, Offset( reflectDetailAdjust, ShapeBaseData ), "scale up or down the detail level for objects rendered in a reflection" );

   endGroup("Reflection");

   Parent::initPersistFields();
}

ConsoleMethod( ShapeBaseData, checkDeployPos, bool, 3, 3, "(Transform xform)")
{
   if (bool(object->mShape) == false)
      return false;

   Point3F pos(0, 0, 0);
   AngAxisF aa(Point3F(0, 0, 1), 0);
   dSscanf(argv[2],"%g %g %g %g %g %g %g",
           &pos.x,&pos.y,&pos.z,&aa.axis.x,&aa.axis.y,&aa.axis.z,&aa.angle);
   MatrixF mat;
   aa.setMatrix(&mat);
   mat.setColumn(3,pos);

   Box3F objBox = object->mShape->bounds;
   Point3F boxCenter = (objBox.minExtents + objBox.maxExtents) * 0.5f;
   objBox.minExtents = boxCenter + (objBox.minExtents - boxCenter) * 0.9f;
   objBox.maxExtents = boxCenter + (objBox.maxExtents - boxCenter) * 0.9f;

   Box3F wBox = objBox;
   mat.mul(wBox);

   EarlyOutPolyList polyList;
   polyList.mNormal.set(0,0,0);
   polyList.mPlaneList.clear();
   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].set(objBox.minExtents,VectorF(-1,0,0));
   polyList.mPlaneList[1].set(objBox.maxExtents,VectorF(0,1,0));
   polyList.mPlaneList[2].set(objBox.maxExtents,VectorF(1,0,0));
   polyList.mPlaneList[3].set(objBox.minExtents,VectorF(0,-1,0));
   polyList.mPlaneList[4].set(objBox.minExtents,VectorF(0,0,-1));
   polyList.mPlaneList[5].set(objBox.maxExtents,VectorF(0,0,1));

   for (U32 i = 0; i < 6; i++)
   {
      PlaneF temp;
      mTransformPlane(mat, Point3F(1, 1, 1), polyList.mPlaneList[i], &temp);
      polyList.mPlaneList[i] = temp;
   }

   if (gServerContainer.buildPolyList(wBox, InteriorObjectType | StaticShapeObjectType, &polyList))
      return false;
   return true;
}


ConsoleMethod(ShapeBaseData, getDeployTransform, const char *, 4, 4, "(Point3F pos, Point3F normal)")
{
   Point3F normal;
   Point3F position;
   dSscanf(argv[2], "%g %g %g", &position.x, &position.y, &position.z);
   dSscanf(argv[3], "%g %g %g", &normal.x, &normal.y, &normal.z);
   normal.normalize();

   VectorF xAxis;
   if( mFabs(normal.z) > mFabs(normal.x) && mFabs(normal.z) > mFabs(normal.y))
      mCross( VectorF( 0, 1, 0 ), normal, &xAxis );
   else
      mCross( VectorF( 0, 0, 1 ), normal, &xAxis );

   VectorF yAxis;
   mCross( normal, xAxis, &yAxis );

   MatrixF testMat(true);
   testMat.setColumn( 0, xAxis );
   testMat.setColumn( 1, yAxis );
   testMat.setColumn( 2, normal );
   testMat.setPosition( position );

   char *returnBuffer = Con::getReturnBuffer(256);
   Point3F pos;
   testMat.getColumn(3,&pos);
   AngAxisF aa(testMat);
   dSprintf(returnBuffer,256,"%g %g %g %g %g %g %g",
            pos.x,pos.y,pos.z,aa.axis.x,aa.axis.y,aa.axis.z,aa.angle);
   return returnBuffer;
}

void ShapeBaseData::packData(BitStream* stream)
{
   Parent::packData(stream);

   if(stream->writeFlag(computeCRC))
      stream->write(mCRC);

   stream->writeFlag(shadowEnable);
   stream->write(shadowSize);
   stream->write(shadowMaxVisibleDistance);
   stream->write(shadowProjectionDistance);
   stream->write(shadowSphereAdjust);


   stream->writeString(shapeName);
   stream->writeString(cloakTexName);
   if(stream->writeFlag(mass != gShapeBaseDataProto.mass))
      stream->write(mass);
   if(stream->writeFlag(drag != gShapeBaseDataProto.drag))
      stream->write(drag);
   if(stream->writeFlag(density != gShapeBaseDataProto.density))
      stream->write(density);
   if(stream->writeFlag(maxEnergy != gShapeBaseDataProto.maxEnergy))
      stream->write(maxEnergy);
   if(stream->writeFlag(cameraMaxDist != gShapeBaseDataProto.cameraMaxDist))
      stream->write(cameraMaxDist);
   if(stream->writeFlag(cameraMinDist != gShapeBaseDataProto.cameraMinDist))
      stream->write(cameraMinDist);
   cameraDefaultFov = mClampF(cameraDefaultFov, cameraMinFov, cameraMaxFov);
   if(stream->writeFlag(cameraDefaultFov != gShapeBaseDataProto.cameraDefaultFov))
      stream->write(cameraDefaultFov);
   if(stream->writeFlag(cameraMinFov != gShapeBaseDataProto.cameraMinFov))
      stream->write(cameraMinFov);
   if(stream->writeFlag(cameraMaxFov != gShapeBaseDataProto.cameraMaxFov))
      stream->write(cameraMaxFov);
   stream->writeString( debrisShapeName );

   stream->writeFlag(observeThroughObject);

   if( stream->writeFlag( debris != NULL ) )
   {
      stream->writeRangedU32(packed? SimObjectId(debris):
                             debris->getId(),DataBlockObjectIdFirst,DataBlockObjectIdLast);
   }

   stream->writeFlag(isInvincible);
   stream->writeFlag(renderWhenDestroyed);

   if( stream->writeFlag( explosion != NULL ) )
   {
      stream->writeRangedU32( explosion->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
   }

   if( stream->writeFlag( underwaterExplosion != NULL ) )
   {
      stream->writeRangedU32( underwaterExplosion->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
   }

   stream->writeFlag(inheritEnergyFromMount);
   stream->writeFlag(firstPersonOnly);
   stream->writeFlag(useEyePoint);
   
   stream->write(cubeDescName);
   //stream->write(reflectPriority);
   //stream->write(reflectMaxRateMs);
   //stream->write(reflectMinDist);
   //stream->write(reflectMaxDist);
   //stream->write(reflectDetailAdjust);
}

void ShapeBaseData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   computeCRC = stream->readFlag();
   if(computeCRC)
      stream->read(&mCRC);

   shadowEnable = stream->readFlag();
   stream->read(&shadowSize);
   stream->read(&shadowMaxVisibleDistance);
   stream->read(&shadowProjectionDistance);
   stream->read(&shadowSphereAdjust);

   shapeName = stream->readSTString();
   cloakTexName = stream->readSTString();
   if(stream->readFlag())
      stream->read(&mass);
   else
      mass = gShapeBaseDataProto.mass;

   if(stream->readFlag())
      stream->read(&drag);
   else
      drag = gShapeBaseDataProto.drag;

   if(stream->readFlag())
      stream->read(&density);
   else
      density = gShapeBaseDataProto.density;

   if(stream->readFlag())
      stream->read(&maxEnergy);
   else
      maxEnergy = gShapeBaseDataProto.maxEnergy;

   if(stream->readFlag())
      stream->read(&cameraMaxDist);
   else
      cameraMaxDist = gShapeBaseDataProto.cameraMaxDist;

   if(stream->readFlag())
      stream->read(&cameraMinDist);
   else
      cameraMinDist = gShapeBaseDataProto.cameraMinDist;

   if(stream->readFlag())
      stream->read(&cameraDefaultFov);
   else
      cameraDefaultFov = gShapeBaseDataProto.cameraDefaultFov;

   if(stream->readFlag())
      stream->read(&cameraMinFov);
   else
      cameraMinFov = gShapeBaseDataProto.cameraMinFov;

   if(stream->readFlag())
      stream->read(&cameraMaxFov);
   else
      cameraMaxFov = gShapeBaseDataProto.cameraMaxFov;

   debrisShapeName = stream->readSTString();

   observeThroughObject = stream->readFlag();

   if( stream->readFlag() )
   {
      debrisID = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
   }

   isInvincible = stream->readFlag();
   renderWhenDestroyed = stream->readFlag();

   if( stream->readFlag() )
   {
      explosionID = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
   }

   if( stream->readFlag() )
   {
      underwaterExplosionID = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
   }

   inheritEnergyFromMount = stream->readFlag();
   firstPersonOnly = stream->readFlag();
   useEyePoint = stream->readFlag();

   stream->read(&cubeDescName);
   //stream->read(&reflectPriority);
   //stream->read(&reflectMaxRateMs);
   //stream->read(&reflectMinDist);
   //stream->read(&reflectMaxDist);
   //stream->read(&reflectDetailAdjust);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

Chunker<ShapeBase::CollisionTimeout> sTimeoutChunker;
ShapeBase::CollisionTimeout* ShapeBase::sFreeTimeoutList = 0;


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(ShapeBase);

ShapeBase::ShapeBase()
 : mDrag( 0.0f ),
   mBuoyancy( 0.0f ),
   mWaterCoverage( 0.0f ),
   mLiquidHeight( 0.0f ),
   mControllingObject( NULL ),
   mGravityMod( 1.0f ),
   mAppliedForce( Point3F::Zero ),
// SphyxGames -> Melee
   mShapeServerId ( 0 ),
// SphyxGames <- Melee
   mTimeoutList( NULL ),
   mDataBlock( NULL ),
   mShapeInstance( NULL ),
   mEnergy( 0.0f ),
   mRechargeRate( 0.0f ),
   mDamage( 0.0f ),
   mRepairRate( 0.0f ),
   mRepairReserve( 0.0f ),
   mDamageState( Enabled ),
   mDamageThread( NULL ),
   mHulkThread( NULL ),
   mLastRenderFrame( 0 ),
   mLastRenderDistance( 0.0f ),
   mCloaked( false ),
   mCloakLevel( 0.0f ),
   mHidden( false ),
   mDamageFlash( 0.0f ),
   mWhiteOut( 0.0f ),
   mInvincibleEffect( 0.0f ),
   mInvincibleDelta( 0.0f ),
   mInvincibleCount( 0.0f ),
   mInvincibleSpeed( 0.0f ),
   mInvincibleTime( 0.0f ),
   mInvincibleFade( 0.1f ),
   mInvincibleOn( false ),
   mIsControlled( false ),
   mConvexList( new Convex ),
   mCameraFov( 90.0f ),
   mShieldNormal( 0.0f, 0.0f, 1.0f ),
   mFadeOut( true ),
   mFading( false ),
   mFadeVal( 1.0f ),
   mFadeTime( 1.0f ),
   mFadeElapsedTime( 0.0f ),
   mFadeDelay( 0.0f ),
   mFlipFadeVal( false ),
   damageDir( 0.0f, 0.0f, 1.0f ),
   mShapeBaseMount( NULL ),
   mMass( 1.0f ),
   mOneOverMass( 1.0f ),
   mSkinHash( 0 )
{
   mTypeMask |= ShapeBaseObjectType | LightObjectType;   

   S32 i;

   for (i = 0; i < MaxSoundThreads; i++) {
      mSoundThread[i].play = false;
      mSoundThread[i].profile = 0;
      mSoundThread[i].sound = 0;
   }

   for (i = 0; i < MaxScriptThreads; i++) {
      mScriptThread[i].sequence = -1;
      mScriptThread[i].thread = 0;
      mScriptThread[i].sound = 0;
      mScriptThread[i].state = Thread::Stop;
      mScriptThread[i].atEnd = false;
	   mScriptThread[i].timescale = 1.f;
	   mScriptThread[i].position = -1.f;
   }

   for (i = 0; i < MaxTriggerKeys; i++)
      mTrigger[i] = false;
}


ShapeBase::~ShapeBase()
{
   SAFE_DELETE( mConvexList );

   AssertFatal(mMount.link == 0,"ShapeBase::~ShapeBase: An object is still mounted");
   if( mShapeInstance && (mShapeInstance->getDebrisRefCount() == 0) )
   {
      delete mShapeInstance;
   }

   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      CollisionTimeout* cur = ptr;
      ptr = ptr->next;
      cur->next = sFreeTimeoutList;
      sFreeTimeoutList = cur;
   }
}


//----------------------------------------------------------------------------

bool ShapeBase::onAdd()
{
   if(!Parent::onAdd() || !mDataBlock)
      return false;

   // Resolve sounds that arrived in the initial update
   S32 i;
   for (i = 0; i < MaxSoundThreads; i++)
      updateAudioState(mSoundThread[i]);

   for (i = 0; i < MaxScriptThreads; i++)
   {
      Thread& st = mScriptThread[i];
      if(st.thread)
         updateThread(st);
   }   

/*
      if(mDataBlock->cloakTexName != StringTable->insert(""))
        mCloakTexture = TextureHandle(mDataBlock->cloakTexName, MeshTexture, false);
*/      

   return true;
}

void ShapeBase::onRemove()
{
   mConvexList->nukeList();

   unmount();
   Parent::onRemove();

   // Stop any running sounds on the client
   if (isGhost())
      for (S32 i = 0; i < MaxSoundThreads; i++)
         stopAudio(i);

   if ( isClientObject() )   
      mCubeReflector.unregisterReflector();      
}


void ShapeBase::onSceneRemove()
{
   mConvexList->nukeList();
   Parent::onSceneRemove();
}

bool ShapeBase::onNewDataBlock( GameBaseData *dptr )
{
   ShapeBaseData *prevDB = dynamic_cast<ShapeBaseData*>( mDataBlock );

   bool isInitialDataBlock = ( mDataBlock == 0 );

   if (Parent::onNewDataBlock(dptr) == false)
      return false;

   mDataBlock = dynamic_cast<ShapeBaseData*>(dptr);
   if (!mDataBlock)
      return false;

   setMaskBits(DamageMask);
   mDamageThread = 0;
   mHulkThread = 0;

   // Even if loadShape succeeds, there may not actually be
   // a shape assigned to this object.
   if (bool(mDataBlock->mShape)) {
      delete mShapeInstance;
      mShapeInstance = new TSShapeInstance(mDataBlock->mShape, isClientObject());
      if (isClientObject())
         mShapeInstance->cloneMaterialList();

      mObjBox = mDataBlock->mShape->bounds;
      resetWorldBox();

      // Set the initial mesh hidden state.
      mMeshHidden.setSize( mDataBlock->mShape->objects.size() );
      mMeshHidden.clear();

      // Initialize the threads
      for (U32 i = 0; i < MaxScriptThreads; i++) {
         Thread& st = mScriptThread[i];
         if (st.sequence != -1) {
            // TG: Need to see about suppressing non-cyclic sounds
            // if the sequences were activated before the object was
            // ghosted.
            // TG: Cyclic animations need to have a random pos if
            // they were started before the object was ghosted.

            // If there was something running on the old shape, the thread
            // needs to be reset. Otherwise we assume that it's been
            // initialized either by the constructor or from the server.
            bool reset = st.thread != 0;
            st.thread = 0;
            
            // New datablock/shape may not actually HAVE this sequence.
            // In that case stop playing it.
            
            AssertFatal( prevDB != NULL, "ShapeBase::onNewDataBlock - how did you have a sequence playing without a prior datablock?" );
   
            const TSShape *prevShape = prevDB->mShape;
            const TSShape::Sequence &prevSeq = prevShape->sequences[st.sequence];
            const String &prevSeqName = prevShape->names[prevSeq.nameIndex];

            st.sequence = mDataBlock->mShape->findSequence( prevSeqName );

            if ( st.sequence != -1 )
            {
               setThreadSequence( i, st.sequence, reset );                              
            }            
         }
      }

      if (mDataBlock->damageSequence != -1) {
         mDamageThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(mDamageThread,
                                     mDataBlock->damageSequence,0);
      }
      if (mDataBlock->hulkSequence != -1) {
         mHulkThread = mShapeInstance->addThread();
         mShapeInstance->setSequence(mHulkThread,
                                     mDataBlock->hulkSequence,0);
      }
   }

   if( isGhost() )
      reSkin();

   //
   mEnergy = 0;
   mDamage = 0;
   mDamageState = Enabled;
   mRepairReserve = 0;
   updateMass();
   updateDamageLevel();
   updateDamageState();

   mDrag = mDataBlock->drag;
   mCameraFov = mDataBlock->cameraDefaultFov;
   updateMass();

   if( !isInitialDataBlock && mLightPlugin )
      mLightPlugin->reset();
   
   if ( isClientObject() )
   {      
      mCubeReflector.unregisterReflector();

      if ( mDataBlock->reflectorDesc )
         mCubeReflector.registerReflector( this, mDataBlock->reflectorDesc );      
   }

   return true;
}

void ShapeBase::onDeleteNotify(SimObject* obj)
{
   if ( obj == getProcessAfter() )
      clearProcessAfter();
   Parent::onDeleteNotify( obj );
   if ( obj == mMount.object )
      unmount();
   if ( obj == mCurrentWaterObject )
      mCurrentWaterObject = NULL;
}

void ShapeBase::onImpact(SceneObject* obj, VectorF vec)
{
   if (!isGhost()) {
      char buff1[256];
      char buff2[32];

      dSprintf(buff1,sizeof(buff1),"%g %g %g",vec.x, vec.y, vec.z);
      dSprintf(buff2,sizeof(buff2),"%g",vec.len());

      Con::executef(mDataBlock,"onImpact",scriptThis(), obj ? obj->getIdString() : "", buff1, buff2);
   }
}

void ShapeBase::onImpact(VectorF vec)
{
   if (!isGhost()) {
      char buff1[256];
      char buff2[32];

      dSprintf(buff1,sizeof(buff1),"%g %g %g",vec.x, vec.y, vec.z);
      dSprintf(buff2,sizeof(buff2),"%g",vec.len());
      Con::executef(mDataBlock,"onImpact",scriptThis(), "0", buff1, buff2);
   }
}


//----------------------------------------------------------------------------

void ShapeBase::processTick(const Move* move)
{
   // Energy management
   if (mDamageState == Enabled && mDataBlock->inheritEnergyFromMount == false) {
      F32 store = mEnergy;
      mEnergy += mRechargeRate;
      if (mEnergy > mDataBlock->maxEnergy)
         mEnergy = mDataBlock->maxEnergy;
      else
         if (mEnergy < 0)
            mEnergy = 0;

      // Virtual setEnergyLevel is used here by some derived classes to
      // decide whether they really want to set the energy mask bit.
      if (mEnergy != store)
         setEnergyLevel(mEnergy);
   }

   // Repair management
   if (mDataBlock->isInvincible == false)
   {
      F32 store = mDamage;
      mDamage -= mRepairRate;
      mDamage = mClampF(mDamage, 0.f, mDataBlock->maxDamage);

      if (mRepairReserve > mDamage)
         mRepairReserve = mDamage;
      if (mRepairReserve > 0.0)
      {
         F32 rate = getMin(mDataBlock->repairRate, mRepairReserve);
         mDamage -= rate;
         mRepairReserve -= rate;
      }

      if (store != mDamage)
      {
         updateDamageLevel();
         if (isServerObject()) {
            char delta[100];
            dSprintf(delta,sizeof(delta),"%g",mDamage - store);
            setMaskBits(DamageMask);
            Con::executef(mDataBlock, "onDamage",scriptThis(),delta);
         }
      }
   }

   if (isServerObject()) {
      // Server only...
      advanceThreads(TickSec);
      updateServerAudio();
// SphyxGames -> Melee
      for (int i = 0; i < MaxMountedImages; i++)
         if (mMountedImageList[i].dataBlock)
            UpdateImageRaycastDamage( TickSec, i );
// SphyxGames <- Melee

      // update wet state
      setImageWetState(0, mWaterCoverage > 0.4); // more than 40 percent covered

      if(mFading)
      {
         F32 dt = TickMs / 1000.0f;
         F32 newFadeET = mFadeElapsedTime + dt;
         if(mFadeElapsedTime < mFadeDelay && newFadeET >= mFadeDelay)
            setMaskBits(HideCloakMask);
         mFadeElapsedTime = newFadeET;
         if(mFadeElapsedTime > mFadeTime + mFadeDelay)
         {
            mFadeVal = F32(!mFadeOut);
            mFading = false;
         }
      }
   }

   // Advance images
   for (int i = 0; i < MaxMountedImages; i++)
   {
      if (mMountedImageList[i].dataBlock != NULL)
         updateImageState(i, TickSec);
   }

   // Call script on trigger state changes
   if (move && mDataBlock && isServerObject()) 
   {
      for (S32 i = 0; i < MaxTriggerKeys; i++) 
      {
         if (move->trigger[i] != mTrigger[i]) 
         {
            mTrigger[i] = move->trigger[i];
            char buf1[20],buf2[20];
            dSprintf(buf1,sizeof(buf1),"%d",i);
            dSprintf(buf2,sizeof(buf2),"%d",(move->trigger[i]?1:0));
            Con::executef(mDataBlock, "onTrigger",scriptThis(),buf1,buf2);
         }
      }
   }

   // Update the damage flash and the whiteout
   //
   if (mDamageFlash > 0.0)
   {
      mDamageFlash -= sDamageFlashDec;
      if (mDamageFlash <= 0.0)
         mDamageFlash = 0.0;
   }
   if (mWhiteOut > 0.0)
   {
      mWhiteOut -= sWhiteoutDec;
      if (mWhiteOut <= 0.0)
         mWhiteOut = 0.0;
   }
}

void ShapeBase::advanceTime(F32 dt)
{
   // On the client, the shape threads and images are
   // advanced at framerate.
   advanceThreads(dt);
   updateAudioPos();
   for (int i = 0; i < MaxMountedImages; i++)
      if (mMountedImageList[i].dataBlock)
         updateImageAnimation(i,dt);

   // Cloaking takes 0.5 seconds
   if (mCloaked && mCloakLevel != 1.0) {
      mCloakLevel += dt * 2;
      if (mCloakLevel >= 1.0)
         mCloakLevel = 1.0;
   } else if (!mCloaked && mCloakLevel != 0.0) {
      mCloakLevel -= dt * 2;
      if (mCloakLevel <= 0.0)
         mCloakLevel = 0.0;
   }
   if(mInvincibleOn)
      updateInvincibleEffect(dt);

   if(mFading)
   {
      mFadeElapsedTime += dt;
      if(mFadeElapsedTime > mFadeTime)
      {
         mFadeVal = F32(!mFadeOut);
         mFading = false;
      }
      else
      {
         mFadeVal = mFadeElapsedTime / mFadeTime;
         if(mFadeOut)
            mFadeVal = 1 - mFadeVal;
      }
   }
}

void ShapeBase::setControllingObject(ShapeBase* obj)
{
   if (obj) {
      setProcessTick(false);
      // Even though we don't processTick, we still need to
      // process after the controller in case anyone is mounted
      // on this object.
      processAfter(obj);
   }
   else {
      setProcessTick(true);
      clearProcessAfter();
      // Catch the case of the controlling object actually
      // mounted on this object.
      if (mControllingObject->mMount.object == this)
         mControllingObject->processAfter(this);
   }
   mControllingObject = obj;
}

ShapeBase* ShapeBase::getControlObject()
{
   return 0;
}

void ShapeBase::setControlObject(ShapeBase*)
{
}

bool ShapeBase::isFirstPerson()
{
   // Always first person as far as the server is concerned.
   if (!isGhost())
      return true;

   if (GameConnection* con = getControllingClient())
      return con->getControlObject() == this && con->isFirstPerson();
   return false;
}

// Camera: (in degrees) ------------------------------------------------------
F32 ShapeBase::getCameraFov()
{
   return(mCameraFov);
}

F32 ShapeBase::getDefaultCameraFov()
{
   return(mDataBlock->cameraDefaultFov);
}

bool ShapeBase::isValidCameraFov(F32 fov)
{
   return((fov >= mDataBlock->cameraMinFov) && (fov <= mDataBlock->cameraMaxFov));
}

void ShapeBase::setCameraFov(F32 fov)
{
   mCameraFov = mClampF(fov, mDataBlock->cameraMinFov, mDataBlock->cameraMaxFov);
}

//----------------------------------------------------------------------------
static void scopeCallback(SceneObject* obj, void *conPtr)
{
   NetConnection * ptr = reinterpret_cast<NetConnection*>(conPtr);
   if (obj->isScopeable())
      ptr->objectInScope(obj);
}

void ShapeBase::onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query)
{
   // update the camera query
   query->camera = this;
   // bool grabEye = true;
   if(GameConnection * con = dynamic_cast<GameConnection*>(cr))
   {
      // get the fov from the connection (in deg)
      F32 fov;
      if (con->getControlCameraFov(&fov))
      {
         query->fov = mDegToRad(fov/2);
         query->sinFov = mSin(query->fov);
         query->cosFov = mCos(query->fov);
      }
   }

   // use eye rather than camera transform (good enough and faster)
   MatrixF eyeTransform;
   getEyeTransform(&eyeTransform);
   eyeTransform.getColumn(3, &query->pos);
   eyeTransform.getColumn(1, &query->orientation);

   // Get the visible distance.
   query->visibleDistance = gServerSceneGraph->getVisibleDistance();

   // First, we are certainly in scope, and whatever we're riding is too...
   cr->objectInScope(this);
   if (isMounted())
      cr->objectInScope(mMount.object);

   if (mSceneManager == NULL)
   {
      // Scope everything...
      gServerContainer.findObjects(0xFFFFFFFF,scopeCallback,cr);
      return;
   }

   // update the scenemanager
   mSceneManager->scopeScene(query->pos, query->visibleDistance, cr);

   // let the (game)connection do some scoping of its own (commandermap...)
   cr->doneScopingScene();
}


//----------------------------------------------------------------------------
F32 ShapeBase::getEnergyLevel()
{
   if ( mDataBlock->inheritEnergyFromMount && mShapeBaseMount )
      return mShapeBaseMount->getEnergyLevel();
   return mEnergy; 
}

F32 ShapeBase::getEnergyValue()
{
   if ( mDataBlock->inheritEnergyFromMount && mShapeBaseMount )
   {
      F32 maxEnergy = mShapeBaseMount->mDataBlock->maxEnergy;
      if ( maxEnergy > 0.f )
         return (mShapeBaseMount->getEnergyLevel() / maxEnergy);
   }
   else
   {
      F32 maxEnergy = mDataBlock->maxEnergy;
      if ( maxEnergy > 0.f )
         return (mEnergy / mDataBlock->maxEnergy);   
   }

   return 0.0f;
}

void ShapeBase::setEnergyLevel(F32 energy)
{
   if (mDataBlock->inheritEnergyFromMount == false || !mShapeBaseMount) {
      if (mDamageState == Enabled) {
         mEnergy = (energy > mDataBlock->maxEnergy)?
            mDataBlock->maxEnergy: (energy < 0)? 0: energy;
      }
   } else {
      // Pass the set onto whatever we're mounted to...
      if ( mShapeBaseMount )
      {
         mShapeBaseMount->setEnergyLevel(energy);
      }
   }
}

void ShapeBase::setDamageLevel(F32 damage)
{
   if (!mDataBlock->isInvincible) {
      F32 store = mDamage;
      mDamage = mClampF(damage, 0.f, mDataBlock->maxDamage);

      if (store != mDamage) {
         updateDamageLevel();
         if (isServerObject()) {
            setMaskBits(DamageMask);
            char delta[100];
            dSprintf(delta,sizeof(delta),"%g",mDamage - store);
            Con::executef(mDataBlock, "onDamage",scriptThis(),delta);
         }
      }
   }
}

void ShapeBase::updateContainer()
{
   // Update container drag and buoyancy properties

   // Set default values.
   mDrag = mDataBlock->drag;
   mBuoyancy = 0.0f;      
   mGravityMod = 1.0;
   mAppliedForce.set(0,0,0);
   
   ContainerQueryInfo info;
   info.box = getWorldBox();
   info.mass = getMass();

   mContainer->findObjects(info.box, WaterObjectType|PhysicalZoneObjectType,findRouter,&info);
      
   mWaterCoverage = info.waterCoverage;
   mLiquidType    = info.liquidType;
   mLiquidHeight  = info.waterHeight;   
   setCurrentWaterObject( info.waterObject );
   
   // This value might be useful as a datablock value,
   // This is what allows the player to stand in shallow water (below this coverage)
   // without jiggling from buoyancy
   if (mWaterCoverage >= 0.25f) 
   {      
      // water viscosity is used as drag for in water.
      // ShapeBaseData drag is used for drag outside of water.
      // Combine these two components to calculate this ShapeBase object's 
      // current drag.
      mDrag = ( info.waterCoverage * info.waterViscosity ) + 
              ( 1.0f - info.waterCoverage ) * mDataBlock->drag;
      mBuoyancy = (info.waterDensity / mDataBlock->density) * info.waterCoverage;
   }

   mAppliedForce = info.appliedForce;
   mGravityMod = info.gravityScale;

   //Con::printf( "WaterCoverage: %f", mWaterCoverage );
   //Con::printf( "Drag: %f", mDrag );
}


//----------------------------------------------------------------------------

void ShapeBase::applyRepair(F32 amount)
{
   // Repair increases the repair reserve
   if (amount > 0 && ((mRepairReserve += amount) > mDamage))
      mRepairReserve = mDamage;
}

void ShapeBase::applyDamage(F32 amount)
{
   if (amount > 0)
      setDamageLevel(mDamage + amount);
}

F32 ShapeBase::getDamageValue()
{
   // Return a 0-1 damage value.
   return mDamage / mDataBlock->maxDamage;
}

void ShapeBase::updateDamageLevel()
{
   if (mDamageThread) {
      // mDamage is already 0-1 on the client
      if (mDamage >= mDataBlock->destroyedLevel) {
         if (getDamageState() == Destroyed)
            mShapeInstance->setPos(mDamageThread, 0);
         else
            mShapeInstance->setPos(mDamageThread, 1);
      } else {
         mShapeInstance->setPos(mDamageThread, mDamage / mDataBlock->destroyedLevel);
      }
   }
}


//----------------------------------------------------------------------------

void ShapeBase::setDamageState(DamageState state)
{
   if (mDamageState == state)
      return;

   const char* script = 0;
   const char* lastState = 0;

   if (!isGhost()) {
      if (state != getDamageState())
         setMaskBits(DamageMask);

      lastState = getDamageStateName();
      switch (state) {
         case Destroyed: {
            if (mDamageState == Enabled)
               setDamageState(Disabled);
            script = "onDestroyed";
            break;
         }
         case Disabled:
            if (mDamageState == Enabled)
               script = "onDisabled";
            break;
         case Enabled:
            script = "onEnabled";
            break;
         default:
            AssertFatal(false, "Invalid damage state");
            break;
      }
   }

   mDamageState = state;
   if (mDamageState != Enabled) {
      mRepairReserve = 0;
      mEnergy = 0;
   }
   if (script) {
      // Like to call the scripts after the state has been intialize.
      // This should only end up being called on the server.
      Con::executef(mDataBlock, script,scriptThis(),lastState);
   }
   updateDamageState();
   updateDamageLevel();
}

bool ShapeBase::setDamageState(const char* state)
{
   for (S32 i = 0; i < NumDamageStates; i++)
      if (!dStricmp(state,sDamageStateName[i])) {
         setDamageState(DamageState(i));
         return true;
      }
   return false;
}

const char* ShapeBase::getDamageStateName()
{
   return sDamageStateName[mDamageState];
}

void ShapeBase::updateDamageState()
{
   if (mHulkThread) {
      F32 pos = (mDamageState == Destroyed) ? 1.0f : 0.0f;
      if (mShapeInstance->getPos(mHulkThread) != pos) {
         mShapeInstance->setPos(mHulkThread,pos);

         if (isClientObject())
            mShapeInstance->animate();
      }
   }
}


//----------------------------------------------------------------------------

void ShapeBase::blowUp()
{
   Point3F center;
   mObjBox.getCenter(&center);
   center += getPosition();
   MatrixF trans = getTransform();
   trans.setPosition( center );

   // explode
   Explosion* pExplosion = NULL;

   if( pointInWater( (Point3F &)center ) && mDataBlock->underwaterExplosion )
   {
      pExplosion = new Explosion;
      pExplosion->onNewDataBlock(mDataBlock->underwaterExplosion);
   }
   else
   {
      if (mDataBlock->explosion)
      {
         pExplosion = new Explosion;
         pExplosion->onNewDataBlock(mDataBlock->explosion);
      }
   }

   if( pExplosion )
   {
      pExplosion->setTransform(trans);
      pExplosion->setInitialState(center, damageDir);
      if (pExplosion->registerObject() == false)
      {
         Con::errorf(ConsoleLogEntry::General, "ShapeBase(%s)::explode: couldn't register explosion",
                     mDataBlock->getName() );
         delete pExplosion;
         pExplosion = NULL;
      }
   }

   TSShapeInstance *debShape = NULL;

   if( mDataBlock->debrisShape == NULL )
   {
      return;
   }
   else
   {
      debShape = new TSShapeInstance( mDataBlock->debrisShape, true);
   }


   Vector< TSPartInstance * > partList;
   TSPartInstance::breakShape( debShape, 0, partList, NULL, NULL, 0 );

   if( !mDataBlock->debris )
   {
      mDataBlock->debris = new DebrisData;
   }

   // cycle through partlist and create debris pieces
   for( U32 i=0; i<partList.size(); i++ )
   {
      //Point3F axis( 0.0, 0.0, 1.0 );
      Point3F randomDir = MathUtils::randomDir( damageDir, 0, 50 );

      Debris *debris = new Debris;
      debris->setPartInstance( partList[i] );
      debris->init( center, randomDir );
      debris->onNewDataBlock( mDataBlock->debris );
      debris->setTransform( trans );

      if( !debris->registerObject() )
      {
         Con::warnf( ConsoleLogEntry::General, "Could not register debris for class: %s", mDataBlock->getName() );
         delete debris;
         debris = NULL;
      }
      else
      {
         debShape->incDebrisRefCount();
      }
   }

   damageDir.set(0, 0, 1);
}


//----------------------------------------------------------------------------
void ShapeBase::mountObject(SceneObject* obj,U32 node)
{
   //if (obj->mMount.object == this)
   //   return;

   if (obj->mMount.object)
      obj->unmount();

   obj->mMount.object = this;
   obj->mMount.node = (node >= 0 && node < ShapeBaseData::NumMountPoints)? node: 0;
   obj->mMount.link = mMount.list;
   mMount.list = obj;

   obj->onMount( this, node );
}


void ShapeBase::unmountObject(SceneObject* obj)
{   
   if ( obj->mMount.object == this ) 
   {
      // Find and unlink the object
      for ( SceneObject **ptr = &mMount.list; *ptr; ptr = &(*ptr)->mMount.link )
      {
         if ( *ptr == obj )
         {
            *ptr = obj->mMount.link;
            break;
         }
      }
      
      obj->mMount.object = 0;
      obj->mMount.link = 0;         
      
      obj->onUnmount( this, obj->mMount.node );
   }
}

void ShapeBase::unmount()
{
   if (mMount.object)
      mMount.object->unmountObject(this);
}

void ShapeBase::onMount( SceneObject *obj, S32 node )
{   
   mConvexList->nukeList();
   deleteNotify(obj);

   // Are we mounting to a ShapeBase object?
   mShapeBaseMount = dynamic_cast<ShapeBase*>( obj );

   if ( mShapeBaseMount && mShapeBaseMount->getControllingObject() != this )
      processAfter( mShapeBaseMount );

   if (!isGhost()) {      
      setMaskBits(MountedMask);
      char buff1[32];
      dSprintf(buff1,sizeof(buff1),"%d",node);
      Con::executef(mDataBlock, "onMount",scriptThis(),obj->scriptThis(),buff1);
   }
}

void ShapeBase::onUnmount( SceneObject *obj, S32 node )
{
   clearNotify(obj);

   if ( mShapeBaseMount && mShapeBaseMount->getControlObject() != this )
      clearProcessAfter();

   mShapeBaseMount = NULL;

   if (!isGhost()) {           
      setMaskBits(MountedMask);
      char buff1[32];
      dSprintf(buff1,sizeof(buff1),"%d",node);
      Con::executef(mDataBlock, "onUnmount",scriptThis(),obj->scriptThis(),buff1);
   }
}

Point3F ShapeBase::getAIRepairPoint()
{
   if (mDataBlock->mountPointNode[ShapeBaseData::AIRepairNode] < 0)
        return Point3F(0, 0, 0);
   MatrixF xf(true);
   getMountTransform(ShapeBaseData::AIRepairNode,&xf);
   Point3F pos(0, 0, 0);
   xf.getColumn(3,&pos);
   return pos;
}

//----------------------------------------------------------------------------

void ShapeBase::getEyeTransform(MatrixF* mat)
{
   // Returns eye to world space transform
   S32 eyeNode = mDataBlock->eyeNode;
   if (eyeNode != -1)
      mat->mul(getTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
   else
      *mat = getTransform();
}

void ShapeBase::getRenderEyeTransform(MatrixF* mat)
{
   // Returns eye to world space transform
   S32 eyeNode = mDataBlock->eyeNode;
   if (eyeNode != -1)
      mat->mul(getRenderTransform(), mShapeInstance->mNodeTransforms[eyeNode]);
   else
      *mat = getRenderTransform();
}

void ShapeBase::getCameraTransform(F32* pos,MatrixF* mat)
{
   // Returns camera to world space transform
   // Handles first person / third person camera position

   if (isServerObject() && mShapeInstance)
      mShapeInstance->animateNodeSubtrees(true);

   if (*pos != 0)
   {
      F32 min,max;
      Point3F offset;
      MatrixF eye,rot;
      getCameraParameters(&min,&max,&offset,&rot);
      getRenderEyeTransform(&eye);
      mat->mul(eye,rot);

      // Use the eye transform to orient the camera
      VectorF vp,vec;
      vp.x = vp.z = 0;
      vp.y = -(max - min) * *pos;
      eye.mulV(vp,&vec);

      // Use the camera node's pos.
      Point3F osp,sp;
      if (mDataBlock->cameraNode != -1) {
         mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);

         // Scale the camera position before applying the transform
         const Point3F& scale = getScale();
         osp.convolve( scale );

         getRenderTransform().mulP(osp,&sp);
      }
      else
         getRenderTransform().getColumn(3,&sp);

      // Make sure we don't extend the camera into anything solid
      Point3F ep = sp + vec + offset;
      disableCollision();
      if (isMounted())
         getObjectMount()->disableCollision();
      RayInfo collision;
      if( mContainer && mContainer->castRay(sp, ep,
                              (0xFFFFFFFF & ~(WaterObjectType      |
                                              GameBaseObjectType   |
                                              TriggerObjectType    |
                                              DefaultObjectType)),
                              &collision) == true) {
         F32 vecLenSq = vec.lenSquared();
         F32 adj = (-mDot(vec, collision.normal) / vecLenSq) * 0.1;
         F32 newPos = getMax(0.0f, collision.t - adj);
         if (newPos == 0.0f)
            eye.getColumn(3,&ep);
         else
            ep = sp + offset + (vec * newPos);
      }
      mat->setColumn(3,ep);
      if (isMounted())
         getObjectMount()->enableCollision();
      enableCollision();
   }
   else
   {
      getRenderEyeTransform(mat);
   }

   // Apply Camera FX.
   mat->mul( gCamFXMgr.getTrans() );
}

// void ShapeBase::getCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }


// void ShapeBase::getRenderCameraTransform(F32* pos,MatrixF* mat)
// {
//    // Returns camera to world space transform
//    // Handles first person / third person camera position

//    if (isServerObject() && mShapeInstance)
//       mShapeInstance->animateNodeSubtrees(true);

//    if (*pos != 0) {
//       F32 min,max;
//       Point3F offset;
//       MatrixF eye,rot;
//       getCameraParameters(&min,&max,&offset,&rot);
//       getRenderEyeTransform(&eye);
//       mat->mul(eye,rot);

//       // Use the eye transform to orient the camera
//       VectorF vp,vec;
//       vp.x = vp.z = 0;
//       vp.y = -(max - min) * *pos;
//       eye.mulV(vp,&vec);

//       // Use the camera node's pos.
//       Point3F osp,sp;
//       if (mDataBlock->cameraNode != -1) {
//          mShapeInstance->mNodeTransforms[mDataBlock->cameraNode].getColumn(3,&osp);
//          getRenderTransform().mulP(osp,&sp);
//       }
//       else
//          getRenderTransform().getColumn(3,&sp);

//       // Make sure we don't extend the camera into anything solid
//       Point3F ep = sp + vec;
//       ep += offset;
//       disableCollision();
//       if (isMounted())
//          getObjectMount()->disableCollision();
//       RayInfo collision;
//       if (mContainer->castRay(sp,ep,(0xFFFFFFFF & ~(WaterObjectType|ForceFieldObjectType|GameBaseObjectType|DefaultObjectType)),&collision)) {
//          *pos = collision.t *= 0.9;
//          if (*pos == 0)
//             eye.getColumn(3,&ep);
//          else
//             ep = sp + vec * *pos;
//       }
//       mat->setColumn(3,ep);
//       if (isMounted())
//          getObjectMount()->enableCollision();
//       enableCollision();
//    }
//    else
//    {
//       getRenderEyeTransform(mat);
//    }
// }

void ShapeBase::getCameraParameters(F32 *min,F32* max,Point3F* off,MatrixF* rot)
{
   *min = mDataBlock->cameraMinDist;
   *max = mDataBlock->cameraMaxDist;
   off->set(0,0,0);
   rot->identity();
}


//----------------------------------------------------------------------------
F32 ShapeBase::getDamageFlash() const
{
   return mDamageFlash;
}

void ShapeBase::setDamageFlash(const F32 flash)
{
   mDamageFlash = flash;
   if (mDamageFlash < 0.0)
      mDamageFlash = 0;
   else if (mDamageFlash > 1.0)
      mDamageFlash = 1.0;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getWhiteOut() const
{
   return mWhiteOut;
}

void ShapeBase::setWhiteOut(const F32 flash)
{
   mWhiteOut = flash;
   if (mWhiteOut < 0.0)
      mWhiteOut = 0;
   else if (mWhiteOut > 1.5)
      mWhiteOut = 1.5;
}


//----------------------------------------------------------------------------

bool ShapeBase::onlyFirstPerson() const
{
   return mDataBlock->firstPersonOnly;
}

bool ShapeBase::useObjsEyePoint() const
{
   return mDataBlock->useEyePoint;
}


//----------------------------------------------------------------------------
F32 ShapeBase::getInvincibleEffect() const
{
   return mInvincibleEffect;
}

void ShapeBase::setupInvincibleEffect(F32 time, F32 speed)
{
   if(isClientObject())
   {
      mInvincibleCount = mInvincibleTime = time;
      mInvincibleSpeed = mInvincibleDelta = speed;
      mInvincibleEffect = 0.0f;
      mInvincibleOn = true;
      mInvincibleFade = 1.0f;
   }
   else
   {
      mInvincibleTime  = time;
      mInvincibleSpeed = speed;
      setMaskBits(InvincibleMask);
   }
}

void ShapeBase::updateInvincibleEffect(F32 dt)
{
   if(mInvincibleCount > 0.0f )
   {
      if(mInvincibleEffect >= ((0.3 * mInvincibleFade) + 0.05f) && mInvincibleDelta > 0.0f)
         mInvincibleDelta = -mInvincibleSpeed;
      else if(mInvincibleEffect <= 0.05f && mInvincibleDelta < 0.0f)
      {
         mInvincibleDelta = mInvincibleSpeed;
         mInvincibleFade = mInvincibleCount / mInvincibleTime;
      }
      mInvincibleEffect += mInvincibleDelta;
      mInvincibleCount -= dt;
   }
   else
   {
      mInvincibleEffect = 0.0f;
      mInvincibleOn = false;
   }
}

//----------------------------------------------------------------------------
void ShapeBase::setVelocity(const VectorF&)
{
}

void ShapeBase::applyImpulse(const Point3F&,const VectorF&)
{
}


//----------------------------------------------------------------------------

void ShapeBase::playAudio(U32 slot,SFXProfile* profile)
{
   AssertFatal( slot < MaxSoundThreads, "ShapeBase::playAudio() bad slot index" );
   Sound& st = mSoundThread[slot];
   if ( profile && ( !st.play || st.profile != profile ) ) 
   {
      setMaskBits(SoundMaskN << slot);
      st.play = true;
      st.profile = profile;
      updateAudioState(st);
   }
}

void ShapeBase::stopAudio(U32 slot)
{
   AssertFatal( slot < MaxSoundThreads, "ShapeBase::stopAudio() bad slot index" );

   Sound& st = mSoundThread[slot];
   if ( st.play ) 
   {
      st.play = false;
      setMaskBits(SoundMaskN << slot);
      updateAudioState(st);
   }
}

void ShapeBase::updateServerAudio()
{
   // Timeout non-looping sounds
   for (int i = 0; i < MaxSoundThreads; i++) {
      Sound& st = mSoundThread[i];
      if (st.play && st.timeout && st.timeout < Sim::getCurrentTime()) {
         clearMaskBits(SoundMaskN << i);
         st.play = false;
      }
   }
}

void ShapeBase::updateAudioState(Sound& st)
{
   SFX_DELETE( st.sound );

   if ( st.play && st.profile ) 
   {
      if ( isGhost() ) 
      {
         if ( Sim::findObject( SimObjectId( st.profile ), st.profile ) )
         {
            st.sound = SFX->createSource( st.profile, &getTransform() );
            if ( st.sound )
               st.sound->play();
         }
         else
            st.play = false;
      }
      else 
      {
         // Non-looping sounds timeout on the server
         st.timeout = 0;
         if ( !st.profile->getDescription()->mIsLooping )
            st.timeout = Sim::getCurrentTime() + sAudioTimeout;
      }
   }
   else
      st.play = false;
}

void ShapeBase::updateAudioPos()
{
   for (int i = 0; i < MaxSoundThreads; i++)
   {
      SFXSource* source = mSoundThread[i].sound;
      if ( source )
         source->setTransform( getTransform() );
   }
}

//----------------------------------------------------------------------------

const char *ShapeBase::getThreadSequenceName( U32 slot )
{
	Thread& st = mScriptThread[slot];
	if ( st.sequence == -1 )
	{
		// Invalid Animation.
		return "";
	}

	// Name Index
	const U32 nameIndex = getShape()->sequences[st.sequence].nameIndex;

	// Return Name.
	return getShape()->getName( nameIndex );
}

bool ShapeBase::setThreadSequence(U32 slot,S32 seq,bool reset)
{
   Thread& st = mScriptThread[slot];
   if (st.thread && st.sequence == seq && st.state == Thread::Play)
      return true;

   if (seq < MaxSequenceIndex) {
      setMaskBits(ThreadMaskN << slot);
      st.sequence = seq;
      if (reset) {
         st.state = Thread::Play;
         st.atEnd = false;
		 st.timescale = 1.f;
		 st.position = 0.f;
      }
      if (mShapeInstance) {
         if (!st.thread)
            st.thread = mShapeInstance->addThread();
         mShapeInstance->setSequence(st.thread,seq,0);
         stopThreadSound(st);
         updateThread(st);
      }
      return true;
   }
   return false;
}

void ShapeBase::updateThread(Thread& st)
{
	switch (st.state)
	{
		case Thread::Stop:
			{
				mShapeInstance->setTimeScale( st.thread, 1.f );
				mShapeInstance->setPos( st.thread, 0.f );
			} // Drop through to pause state

		case Thread::Pause:
			{
				if ( st.position != -1.f )
				{
					mShapeInstance->setTimeScale( st.thread, 1.f );
					mShapeInstance->setPos( st.thread, st.position );
				}

				mShapeInstance->setTimeScale( st.thread, 0.f );
				stopThreadSound( st );
			} break;

		case Thread::Play:
			{
				if (st.atEnd)
				{
					mShapeInstance->setTimeScale(st.thread,1);
					mShapeInstance->setPos( st.thread, ( st.timescale > 0.f ) ? 1.0f : 0.0f );
					mShapeInstance->setTimeScale(st.thread,0);
					stopThreadSound(st);
               st.state = Thread::Stop;
				}
				else
				{
					if ( st.position != -1.f )
					{
						mShapeInstance->setTimeScale( st.thread, 1.f );
						mShapeInstance->setPos( st.thread, st.position );
					}

					mShapeInstance->setTimeScale(st.thread, st.timescale );
					if (!st.sound)
					{
						startSequenceSound(st);
					}
				}
			} break;
	}
}

bool ShapeBase::stopThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Stop) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Stop;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::pauseThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Pause) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Pause;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::playThread(U32 slot)
{
   Thread& st = mScriptThread[slot];
   if (st.sequence != -1 && st.state != Thread::Play) {
      setMaskBits(ThreadMaskN << slot);
      st.state = Thread::Play;
      updateThread(st);
      return true;
   }
   return false;
}

bool ShapeBase::setThreadPosition( U32 slot, F32 pos )
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		setMaskBits(ThreadMaskN << slot);
		st.position = pos;
		st.atEnd = false;
		updateThread(st);

		return true;
	}
	return false;
}

bool ShapeBase::setThreadDir(U32 slot,bool forward)
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		if ( ( st.timescale >= 0.f ) != forward )
		{
			setMaskBits(ThreadMaskN << slot);
			st.timescale *= -1.f ;
			st.atEnd = false;
			updateThread(st);
		}
		return true;
	}
	return false;
}

bool ShapeBase::setThreadTimeScale( U32 slot, F32 timeScale )
{
	Thread& st = mScriptThread[slot];
	if (st.sequence != -1)
	{
		if (st.timescale != timeScale)
		{
			setMaskBits(ThreadMaskN << slot);
			st.timescale = timeScale;
			updateThread(st);
		}
		return true;
	}
	return false;
}

void ShapeBase::stopThreadSound(Thread& thread)
{
   if (thread.sound) {
   }
}

void ShapeBase::startSequenceSound(Thread& thread)
{
   if (!isGhost() || !thread.thread)
      return;
   stopThreadSound(thread);
}

void ShapeBase::advanceThreads(F32 dt)
{
   for (U32 i = 0; i < MaxScriptThreads; i++) {
      Thread& st = mScriptThread[i];
      if (st.thread) {
         if (!mShapeInstance->getShape()->sequences[st.sequence].isCyclic() && !st.atEnd &&
			 ( ( st.timescale > 0.f )? mShapeInstance->getPos(st.thread) >= 1.0:
              mShapeInstance->getPos(st.thread) <= 0)) {
            st.atEnd = true;
            updateThread(st);
            if (!isGhost()) {
               char slot[16];
               dSprintf(slot,sizeof(slot),"%d",i);
               Con::executef(mDataBlock, "onEndSequence",scriptThis(),slot);
            }
         }
         mShapeInstance->advanceTime(dt,st.thread);
      }
   }
}

//----------------------------------------------------------------------------

/// Emit particles on the given emitter, if we're within triggerHeight above some static surface with a
/// material that has 'showDust' set to true.  The particles will have a lifetime of 'numMilliseconds'
/// and be emitted at the given offset from the contact point having a direction of 'axis'.

void ShapeBase::emitDust( ParticleEmitter* emitter, F32 triggerHeight, const Point3F& offset, U32 numMilliseconds, const Point3F& axis )
{
   if( !emitter )
      return;

   Point3F startPos = getPosition();
   Point3F endPos = startPos + Point3F( 0.0, 0.0, - triggerHeight );

   RayInfo rayInfo;
   if( getContainer()->castRay( startPos, endPos, STATIC_COLLISION_MASK, &rayInfo ) )
   {
      Material* material = ( rayInfo.material ? dynamic_cast< Material* >( rayInfo.material->getMaterial() ) : 0 );
      if( material && material->mShowDust )
      {
         ColorF colorList[ ParticleData::PDC_NUM_KEYS ];

         for( U32 x = 0; x < getMin( Material::NUM_EFFECT_COLOR_STAGES, ParticleData::PDC_NUM_KEYS ); ++ x )
            colorList[ x ] = material->mEffectColor[ x ];
         for( U32 x = Material::NUM_EFFECT_COLOR_STAGES; x < ParticleData::PDC_NUM_KEYS; ++ x )
            colorList[ x ].set( 1.0, 1.0, 1.0, 0.0 );

         emitter->setColors( colorList );

         Point3F contactPoint = rayInfo.point + offset;
         emitter->emitParticles( contactPoint, true, ( axis == Point3F::Zero ? rayInfo.normal : axis ),
                                 getVelocity(), numMilliseconds );
      }
   }
}

//----------------------------------------------------------------------------

TSShape const* ShapeBase::getShape()
{
   return mShapeInstance? mShapeInstance->getShape(): 0;
}

bool ShapeBase::prepRenderImage( SceneState *state, 
                                 const U32 stateKey,
                                 const U32 startZone, 
                                 const bool modifyBaseState)
{
   return _prepRenderImage( state, stateKey, startZone, modifyBaseState, true, true );
}

bool ShapeBase::_prepRenderImage(   SceneState *state, 
                                    const U32 stateKey,
                                    const U32 startZone, 
                                    const bool modifyBaseState,
                                    bool renderSelf, 
                                    bool renderMountedImages )
{
   AssertFatal(modifyBaseState == false, "Error, should never be called with this parameter set");
   AssertFatal(startZone == 0xFFFFFFFF, "Error, startZone should indicate -1");

   PROFILE_SCOPE( ShapeBase_PrepRenderImage );

   if (isLastState(state, stateKey))
      return false;

   setLastState(state, stateKey);

   //if ( mIsCubemapUpdate )
   //   return false;

   if( ( getDamageState() == Destroyed ) && ( !mDataBlock->renderWhenDestroyed ) )
      return false;

   // We don't need to render if all the meshes are forced hidden.
   if ( mMeshHidden.testAll() )   
      return false;
      
   // If we're rendering shadows don't render the mounted
   // images unless the shape is also rendered.
   if ( state->isShadowPass() && !renderSelf )
      return false;

   // If we're currently rendering our own reflection we
   // don't want to render ourselves into it.
   if ( mCubeReflector.isRendering() )
      return false;

   // We force all the shapes to use the highest detail
   // if we're the control object or mounted.
   bool forceHighestDetail = false;
   {
      GameConnection *con = GameConnection::getConnectionToServer();
      ShapeBase *co = NULL;
      if(con && ( (co = dynamic_cast<ShapeBase*>(con->getControlObject())) != NULL) )
      {
         if(co == this || co->getObjectMount() == this)
            forceHighestDetail = true;
      }
   }

   if ( state->isObjectRendered(this) || state->isReflectPass() )
   {
      mLastRenderFrame = sLastRenderFrame;

      // get shape detail...we might not even need to be drawn
      Point3F cameraOffset = getWorldBox().getClosestPoint( state->getDiffuseCameraPosition() ) - state->getDiffuseCameraPosition();
      F32 dist = cameraOffset.len();
      if (dist < 0.01f)
         dist = 0.01f;

      F32 invScale = (1.0f/getMax(getMax(mObjScale.x,mObjScale.y),mObjScale.z));

      if (mShapeInstance)
      {
         if ( forceHighestDetail )         
            mShapeInstance->setCurrentDetail( 0 );
         else
            mShapeInstance->setDetailFromDistance( state, dist * invScale );
                                 
         mShapeInstance->animate();
      }
      
      if (  ( mShapeInstance && mShapeInstance->getCurrentDetail() < 0 ) ||
            ( !mShapeInstance && !gShowBoundingBox ) ) 
      {
         // no, don't draw anything
         return false;
      }

      if( renderMountedImages )
      {
         for (U32 i = 0; i < MaxMountedImages; i++)
         {
            MountedImage& image = mMountedImageList[i];
            if (image.dataBlock && image.shapeInstance)
            {
               // Select detail levels on mounted items but... always 
               // draw the control object's mounted images in high detail.
               if ( forceHighestDetail )
                  image.shapeInstance->setCurrentDetail( 0 );
               else
                  image.shapeInstance->setDetailFromDistance( state, dist * invScale );

               if (mCloakLevel == 0.0f && image.shapeInstance->hasSolid() && mFadeVal == 1.0f)
               {
                  prepBatchRender( state, i );

                  // Debug rendering of the mounted shape bounds.
                  if ( gShowBoundingBox )
                  {
                     ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
                     ri->renderDelegate.bind( this, &ShapeBase::_renderBoundingBox );
                     ri->objectIndex = i;
                     ri->type = RenderPassManager::RIT_Object;
                     state->getRenderPass()->addInst( ri );
                  }
               }
            }
         }
      }

      // Debug rendering of the shape bounding box.
      if ( gShowBoundingBox )
      {
         ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
         ri->renderDelegate.bind( this, &ShapeBase::_renderBoundingBox );
         ri->objectIndex = -1;
         ri->type = RenderPassManager::RIT_Object;
         state->getRenderPass()->addInst( ri );
      }

      if ( mShapeInstance && renderSelf )
         prepBatchRender( state, -1 );

      calcClassRenderData();
   }

   return false;
}

//----------------------------------------------------------------------------
// prepBatchRender
//----------------------------------------------------------------------------
void ShapeBase::prepBatchRender(SceneState* state, S32 mountedImageIndex )
{
   // CHANGES IN HERE SHOULD BE DUPLICATED IN TSSTATIC!
   
   GFXTransformSaver saver;
   
   // Set up our TS render state. 
   TSRenderState rdata;
   rdata.setSceneState( state );
   if ( mCubeReflector.isEnabled() )
      rdata.setCubemap( mCubeReflector.getCubemap() );
   rdata.setFadeOverride( mFadeVal );

   LightManager *lm = gClientSceneGraph->getLightManager();
   if ( !state->isShadowPass() )
      lm->setupLights( this, getWorldSphere() );

   if( mountedImageIndex != -1 )
   {
      MountedImage& image = mMountedImageList[mountedImageIndex];

      if( image.dataBlock && image.shapeInstance )
      {
         renderMountedImage( mountedImageIndex, rdata );
      }
   }
   else
   {
      MatrixF mat = getRenderTransform();
      mat.scale( mObjScale );
      GFX->setWorldMatrix( mat );

      if ( state->isDiffusePass() && mCubeReflector.isEnabled() && mCubeReflector.getOcclusionQuery() )
      {
         RenderPassManager *pass = state->getRenderPass();

         OccluderRenderInst *ri = pass->allocInst<OccluderRenderInst>();   

         ri->type = RenderPassManager::RIT_Occluder;
         ri->query = mCubeReflector.getOcclusionQuery();   
         mObjToWorld.mulP( mObjBox.getCenter(), &ri->position );
         ri->scale.set( mObjBox.getExtents() );
         ri->orientation = pass->allocUniqueXform( mObjToWorld );        
         ri->isSphere = false;
         state->getRenderPass()->addInst( ri );
      }

      mShapeInstance->animate();
      mShapeInstance->render( rdata );
   }
   
   lm->resetLights();
}

void ShapeBase::renderMountedImage( U32 imageSlot, TSRenderState &rstate )
{
   GFX->pushWorldMatrix();

   MatrixF mat;
   getRenderImageTransform(imageSlot, &mat, rstate.getSceneState()->isShadowPass());
   GFX->setWorldMatrix( mat );

   MountedImage& image = mMountedImageList[imageSlot];
   image.shapeInstance->animate();
   image.shapeInstance->render( rstate );

   GFX->popWorldMatrix();
}

void ShapeBase::_renderBoundingBox( ObjectRenderInst *ri, SceneState *state, BaseMatInstance *overrideMat )
{
   // If we got an override mat then this some
   // special rendering pass... skip out of it.
   if ( overrideMat )
      return;

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.fillMode = GFXFillWireframe;

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if ( ri->objectIndex != -1 )
   {
      MountedImage &image = mMountedImageList[ ri->objectIndex ];

      if ( image.shapeInstance )
      {
         MatrixF mat;
         getRenderImageTransform( ri->objectIndex, &mat );         

         const Box3F &objBox = image.shapeInstance->getShape()->bounds;

         drawer->drawCube( desc, objBox, ColorI( 255, 255, 255 ), &mat );
      }
   }
   else   
      drawer->drawCube( desc, mObjBox, ColorI( 255, 255, 255 ), &mRenderObjToWorld );
}

bool ShapeBase::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (mShapeInstance) 
   {
      RayInfo shortest;
      shortest.t = 1e8;

      info->object = NULL;
      for (U32 i = 0; i < mDataBlock->LOSDetails.size(); i++)
      {
         mShapeInstance->animate(mDataBlock->LOSDetails[i]);
         if (mShapeInstance->castRay(start, end, info, mDataBlock->LOSDetails[i]))
         {
            info->object = this;
            if (info->t < shortest.t)
               shortest = *info;
         }
      }

      if (info->object == this) 
      {
         // Copy out the shortest time...
         *info = shortest;
         return true;
      }
   }

   return false;
}

bool ShapeBase::castRayRendered(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (mShapeInstance) 
   {
      RayInfo localInfo;
      mShapeInstance->animate();
      bool res = mShapeInstance->castRayRendered(start, end, &localInfo, mShapeInstance->getCurrentDetail());
      if (res)
      {
         *info = localInfo;
         info->object = this;
         return true;
      }
   }

   return false;
}


//----------------------------------------------------------------------------

bool ShapeBase::buildPolyList(AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
   if (mShapeInstance) {
      bool ret = false;

      polyList->setTransform(&mObjToWorld, mObjScale);
      polyList->setObject(this);

      for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
      {
            mShapeInstance->buildPolyList(polyList,mDataBlock->collisionDetails[i]);
            ret = true;
         }

      return ret;
   }

   return false;
}

bool ShapeBase::buildRenderedPolyList(AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
   if (mShapeInstance) {
      polyList->setTransform(&mObjToWorld, mObjScale);
      polyList->setObject(this);

      mShapeInstance->animate();
      mShapeInstance->buildPolyList(polyList,mShapeInstance->getCurrentDetail());

      return true;
   }

   return false;
}


void ShapeBase::buildConvex(const Box3F& box, Convex* convex)
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

   for (U32 i = 0; i < mDataBlock->collisionDetails.size(); i++)
   {
         Box3F newbox = mDataBlock->collisionBounds[i];
         newbox.minExtents.convolve(mObjScale);
         newbox.maxExtents.convolve(mObjScale);
         mObjToWorld.mul(newbox);
         if (box.isOverlapped(newbox) == false)
            continue;

         // See if this hull exists in the working set already...
         Convex* cc = 0;
         CollisionWorkingList& wl = convex->getWorkingList();
         for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
            if (itr->mConvex->getType() == ShapeBaseConvexType &&
                (static_cast<ShapeBaseConvex*>(itr->mConvex)->pShapeBase == this &&
                 static_cast<ShapeBaseConvex*>(itr->mConvex)->hullId     == i)) {
               cc = itr->mConvex;
               break;
            }
         }
         if (cc)
            continue;

         // Create a new convex.
         ShapeBaseConvex* cp = new ShapeBaseConvex;
         mConvexList->registerObject(cp);
         convex->addToWorkingList(cp);
         cp->mObject    = this;
         cp->pShapeBase = this;
         cp->hullId     = i;
         cp->box        = mDataBlock->collisionBounds[i];
         cp->transform = 0;
         cp->findNodeTransform();
   }
}


//----------------------------------------------------------------------------

void ShapeBase::queueCollision( SceneObject *obj, const VectorF &vec)
{
   // Add object to list of collisions.
   SimTime time = Sim::getCurrentTime();
   S32 num = obj->getId();

   CollisionTimeout** adr = &mTimeoutList;
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      if (ptr->objectNumber == num) {
         if (ptr->expireTime < time) {
            ptr->expireTime = time + CollisionTimeoutValue;
            ptr->object = obj;
            ptr->vector = vec;
         }
         return;
      }
      // Recover expired entries
      if (ptr->expireTime < time) {
         CollisionTimeout* cur = ptr;
         *adr = ptr->next;
         ptr = ptr->next;
         cur->next = sFreeTimeoutList;
         sFreeTimeoutList = cur;
      }
      else {
         adr = &ptr->next;
         ptr = ptr->next;
      }
   }

   // New entry for the object
   if (sFreeTimeoutList != NULL)
   {
      ptr = sFreeTimeoutList;
      sFreeTimeoutList = ptr->next;
      ptr->next = NULL;
   }
   else
   {
      ptr = sTimeoutChunker.alloc();
   }

   ptr->object = obj;
   ptr->objectNumber = obj->getId();
   ptr->vector = vec;
   ptr->expireTime = time + CollisionTimeoutValue;
   ptr->next = mTimeoutList;

   mTimeoutList = ptr;
}

void ShapeBase::notifyCollision()
{
   // Notify all the objects that were just stamped during the queueing
   // process.
   SimTime expireTime = Sim::getCurrentTime() + CollisionTimeoutValue;
   for (CollisionTimeout* ptr = mTimeoutList; ptr; ptr = ptr->next)
   {
      if (ptr->expireTime == expireTime && ptr->object)
      {
         SimObjectPtr<SceneObject> safePtr(ptr->object);
         SimObjectPtr<ShapeBase> safeThis(this);
         onCollision(ptr->object,ptr->vector);
         ptr->object = 0;

         if(!bool(safeThis))
            return;

         if(bool(safePtr))
            safePtr->onCollision(this,ptr->vector);

         if(!bool(safeThis))
            return;
      }
   }
}

void ShapeBase::onCollision( SceneObject *object, const VectorF &vec )
{
   if (!isGhost())  {
      char buff1[256];
      char buff2[32];

      dSprintf(buff1,sizeof(buff1),"%g %g %g",vec.x, vec.y, vec.z);
      dSprintf(buff2,sizeof(buff2),"%g",vec.len());
      Con::executef(mDataBlock, "onCollision",scriptThis(),object->scriptThis(), buff1, buff2);
   }
}

//--------------------------------------------------------------------------
bool ShapeBase::pointInWater( Point3F &point )
{
   if ( mCurrentWaterObject == NULL )
      return false;

   return mCurrentWaterObject->isUnderwater( point );   
}

//----------------------------------------------------------------------------

void ShapeBase::writePacketData(GameConnection *connection, BitStream *stream)
{
   Parent::writePacketData(connection, stream);

   stream->write(getEnergyLevel());
   stream->write(mRechargeRate);
}

void ShapeBase::readPacketData(GameConnection *connection, BitStream *stream)
{
   Parent::readPacketData(connection, stream);

   F32 energy;
   stream->read(&energy);
   setEnergyLevel(energy);

   stream->read(&mRechargeRate);
}

F32 ShapeBase::getUpdatePriority(CameraScopeQuery *camInfo, U32 updateMask, S32 updateSkips)
{
   // If it's the scope object, must be high priority
   if (camInfo->camera == this) {
      // Most priorities are between 0 and 1, so this
      // should be something larger.
      return 10.0f;
   }
   if (camInfo->camera && (camInfo->camera->getType() & ShapeBaseObjectType))
   {
      // see if the camera is mounted to this...
      // if it is, this should have a high priority
      if(((ShapeBase *) camInfo->camera)->getObjectMount() == this)
         return 10.0f;
   }
   return Parent::getUpdatePriority(camInfo, updateMask, updateSkips);
}

U32 ShapeBase::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (mask & InitialUpdateMask) {
      // mask off sounds that aren't playing
      S32 i;
      for (i = 0; i < MaxSoundThreads; i++)
         if (!mSoundThread[i].play)
            mask &= ~(SoundMaskN << i);

      // mask off threads that aren't running
      for (i = 0; i < MaxScriptThreads; i++)
         if (mScriptThread[i].sequence == -1)
            mask &= ~(ThreadMaskN << i);

      // mask off images that aren't updated
      for(i = 0; i < MaxMountedImages; i++)
         if(!mMountedImageList[i].dataBlock)
            mask &= ~(ImageMaskN << i);
   }

   if(!stream->writeFlag(mask & (NameMask | DamageMask | SoundMask | MeshHiddenMask |
         ThreadMask | ImageMask | HideCloakMask | MountedMask | InvincibleMask |
         ShieldMask | SkinMask | ServerIdMask)))
      return retMask;

   if (stream->writeFlag(mask & DamageMask)) {
      stream->writeFloat(mClampF(mDamage / mDataBlock->maxDamage, 0.f, 1.f), DamageLevelBits);
      stream->writeInt(mDamageState,NumDamageStateBits);
      stream->writeNormalVector( damageDir, 8 );
   }

   if (stream->writeFlag(mask & ThreadMask)) {
      for (int i = 0; i < MaxScriptThreads; i++) {
         Thread& st = mScriptThread[i];
         if (stream->writeFlag(st.sequence != -1 && (mask & (ThreadMaskN << i)))) {
            stream->writeInt(st.sequence,ThreadSequenceBits);
            stream->writeInt(st.state,2);
			stream->write(st.timescale);
			stream->write(st.position);
            stream->writeFlag(st.atEnd);

			// Clear Update.
			st.position = -1.f;
         }
      }
   }

   if (stream->writeFlag(mask & SoundMask)) {
      for (int i = 0; i < MaxSoundThreads; i++) {
         Sound& st = mSoundThread[i];
         if (stream->writeFlag(mask & (SoundMaskN << i)))
            if (stream->writeFlag(st.play))
               stream->writeRangedU32(st.profile->getId(),DataBlockObjectIdFirst,
                                      DataBlockObjectIdLast);
      }
   }

   if (stream->writeFlag(mask & ImageMask)) {
      for (int i = 0; i < MaxMountedImages; i++)
         if (stream->writeFlag(mask & (ImageMaskN << i))) {
            MountedImage& image = mMountedImageList[i];
            if (stream->writeFlag(image.dataBlock))
               stream->writeInt(image.dataBlock->getId() - DataBlockObjectIdFirst,
                                DataBlockObjectIdBitSize);
            con->packNetStringHandleU(stream, image.skinNameHandle);
            stream->writeFlag(image.wet);
            stream->writeFlag(image.ammo);
            stream->writeFlag(image.loaded);
            stream->writeFlag(image.target);
            stream->writeFlag(image.triggerDown);
            stream->writeFlag(image.altTriggerDown);
            stream->writeInt(image.fireCount,3);
            if (mask & InitialUpdateMask)
               stream->writeFlag(isImageFiring(i));
         }
   }

   // Group some of the uncommon stuff together.
   if (stream->writeFlag(mask & (NameMask | ShieldMask | HideCloakMask | InvincibleMask | SkinMask | MeshHiddenMask | ServerIdMask  ))) {
         
      if (stream->writeFlag(mask & HideCloakMask))
      {
         // Write hiding state.
         stream->writeFlag( mHidden );
         
         // cloaking
         stream->writeFlag( mCloaked );

         // piggyback control update
         stream->writeFlag(bool(getControllingClient()));

// SphyxGames -> Melee
        if (stream->writeFlag(mask & ServerIdMask)) {
            stream->writeInt(mShapeServerId,32);
        }
// SphyxGames <- Melee

         // fading
         if(stream->writeFlag(mFading && mFadeElapsedTime >= mFadeDelay)) {
            stream->writeFlag(mFadeOut);
            stream->write(mFadeTime);
         }
         else
            stream->writeFlag(mFadeVal == 1.0f);
      }
      if (stream->writeFlag(mask & NameMask)) {
         con->packNetStringHandleU(stream, mShapeNameHandle);
      }
      if (stream->writeFlag(mask & ShieldMask)) {
         stream->writeNormalVector(mShieldNormal, ShieldNormalBits);
         stream->writeFloat( getEnergyValue(), EnergyLevelBits );
      }
      if (stream->writeFlag(mask & InvincibleMask)) {
         stream->write(mInvincibleTime);
         stream->write(mInvincibleSpeed);
      }      

      if ( stream->writeFlag( mask & MeshHiddenMask ) )
         stream->writeBits( mMeshHidden );         

      if (stream->writeFlag(mask & SkinMask))
         con->packNetStringHandleU(stream, mSkinNameHandle);
   }

   if (mask & MountedMask) {
      if (mMount.object) {
         S32 gIndex = con->getGhostIndex(mMount.object);
         if (stream->writeFlag(gIndex != -1)) {
            stream->writeFlag(true);
            stream->writeInt(gIndex,NetConnection::GhostIdBitSize);
            stream->writeInt(mMount.node,ShapeBaseData::NumMountPointBits);
         }
         else
            // Will have to try again later
            retMask |= MountedMask;
      }
      else
         // Unmount if this isn't the initial packet
         if (stream->writeFlag(!(mask & InitialUpdateMask)))
            stream->writeFlag(false);
   }
   else
      stream->writeFlag(false);

   return retMask;
}

void ShapeBase::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
   mLastRenderFrame = sLastRenderFrame; // make sure we get a process after the event...

   if(!stream->readFlag())
      return;

   if (stream->readFlag()) {
      mDamage = mClampF(stream->readFloat(DamageLevelBits) * mDataBlock->maxDamage, 0.f, mDataBlock->maxDamage);
      DamageState prevState = mDamageState;
      mDamageState = DamageState(stream->readInt(NumDamageStateBits));
      stream->readNormalVector( &damageDir, 8 );
      if (prevState != Destroyed && mDamageState == Destroyed && isProperlyAdded())
         blowUp();
      updateDamageLevel();
      updateDamageState();
   }

   if (stream->readFlag()) {
      for (S32 i = 0; i < MaxScriptThreads; i++) {
         if (stream->readFlag()) {
            Thread& st = mScriptThread[i];
            U32 seq = stream->readInt(ThreadSequenceBits);
            st.state = stream->readInt(2);
			stream->read( &st.timescale );
			stream->read( &st.position );
            st.atEnd = stream->readFlag();
            if (st.sequence != seq)
               setThreadSequence(i,seq,false);
            else
               updateThread(st);
         }
      }
   }

   if ( stream->readFlag() ) 
   {
      for ( S32 i = 0; i < MaxSoundThreads; i++ ) 
      {
         if ( stream->readFlag() ) 
         {
            Sound& st = mSoundThread[i];
            st.play = stream->readFlag();
            if ( st.play ) 
            {
               st.profile = (SFXProfile*) stream->readRangedU32(  DataBlockObjectIdFirst,
                                                                  DataBlockObjectIdLast );
            }

            if ( isProperlyAdded() )
               updateAudioState( st );
         }
      }
   }

   if (stream->readFlag()) {
      for (int i = 0; i < MaxMountedImages; i++) {
         if (stream->readFlag()) {
            MountedImage& image = mMountedImageList[i];
            ShapeBaseImageData* imageData = 0;
            if (stream->readFlag()) {
               SimObjectId id = stream->readInt(DataBlockObjectIdBitSize) +
                  DataBlockObjectIdFirst;
               if (!Sim::findObject(id,imageData)) {
                  con->setLastError("Invalid packet (mounted images).");
                  return;
               }
            }

            NetStringHandle skinDesiredNameHandle = con->unpackNetStringHandleU(stream);

            image.wet = stream->readFlag();

            image.ammo = stream->readFlag();

            image.loaded = stream->readFlag();

            image.target = stream->readFlag();

            image.triggerDown = stream->readFlag();
            image.altTriggerDown = stream->readFlag();

            int count = stream->readInt(3);

            if ((image.dataBlock != imageData) || (image.skinNameHandle != skinDesiredNameHandle)) 
               setImage(   i, imageData, 
                           skinDesiredNameHandle, image.loaded, 
                           image.ammo, image.triggerDown, image.altTriggerDown );

            if (isProperlyAdded()) {
               // Normal processing
               if (count != image.fireCount)
               {
                  image.fireCount = count;
                  setImageState(i,getImageFireState(i),true);

                  if( imageData && imageData->lightType == ShapeBaseImageData::WeaponFireLight )
                  {
                     image.lightStart = Sim::getCurrentTime();                     
                  }
               }
               updateImageState(i,0);
            }
            else
            {
               bool firing = stream->readFlag();
               if(imageData)
               {
                  // Initial state
                  image.fireCount = count;
                  if (firing)
                     setImageState(i,getImageFireState(i),true);
               }
            }
         }
      }
   }

   if (stream->readFlag())
   {
      if(stream->readFlag())     // HideCloakMask and control
      {
         // Read hiding state.
         
         setHidden( stream->readFlag() );
         
         // Read cloaking state.
         
         setCloakedState(stream->readFlag());
         mIsControlled = stream->readFlag();

// SphyxGames -> Melee
         if(stream->readFlag())
         {
            mShapeServerId = stream->readInt(32);
         }
// SphyxGames <- Melee

         if (( mFading = stream->readFlag()) == true) {
            mFadeOut = stream->readFlag();
            if(mFadeOut)
               mFadeVal = 1.0f;
            else
               mFadeVal = 0;
            stream->read(&mFadeTime);
            mFadeDelay = 0;
            mFadeElapsedTime = 0;
         }
         else
            mFadeVal = F32(stream->readFlag());
      }
      if (stream->readFlag())  { // NameMask
         mShapeNameHandle = con->unpackNetStringHandleU(stream);
      }
      if(stream->readFlag())     // ShieldMask
      {
         // Cloaking, Shield, and invul masking
         Point3F shieldNormal;
         stream->readNormalVector(&shieldNormal, ShieldNormalBits);
         
         // CodeReview [bjg 4/6/07] This is our energy level - why aren't we storing it? Was in a
         // local variable called energyPercent.
         stream->readFloat(EnergyLevelBits);
      }

      if (stream->readFlag()) 
      {
         // InvincibleMask
         F32 time, speed;
         stream->read(&time);
         stream->read(&speed);
         setupInvincibleEffect(time, speed);
      }
      
      if ( stream->readFlag() ) // MeshHiddenMask
      {
         stream->readBits( &mMeshHidden );         
         _updateHiddenMeshes();
      }

      if (stream->readFlag()) {  // SkinMask

         NetStringHandle skinDesiredNameHandle = con->unpackNetStringHandleU(stream);;

         if (mSkinNameHandle != skinDesiredNameHandle) {

            mSkinNameHandle = skinDesiredNameHandle;
            reSkin();
         }

      }
   }

   if (stream->readFlag()) {
      if (stream->readFlag()) {
         S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);
         SceneObject* obj = dynamic_cast<SceneObject*>(con->resolveGhost(gIndex));
         S32 node = stream->readInt(ShapeBaseData::NumMountPointBits);
         if(!obj)
         {
            con->setLastError("Invalid packet from server.");
            return;
         }
         obj->mountObject(this,node);
      }
      else
         unmount();
   }
}


//--------------------------------------------------------------------------

void ShapeBase::forceUncloak(const char * reason)
{
   AssertFatal(isServerObject(), "ShapeBase::forceUncloak: server only call");
   if(!mCloaked)
      return;

   Con::executef(mDataBlock, "onForceUncloak", scriptThis(), reason ? reason : "");
}

void ShapeBase::setCloakedState(bool cloaked)
{
   if (cloaked == mCloaked)
      return;

   if (isServerObject())
      setMaskBits(HideCloakMask);

   // Have to do this for the client, if we are ghosted over in the initial
   //  packet as cloaked, we set the state immediately to the extreme
   if (isProperlyAdded() == false) {
      mCloaked = cloaked;
      if (mCloaked)
         mCloakLevel = 1.0;
      else
         mCloakLevel = 0.0;
   } else {
      mCloaked = cloaked;
   }
}


//--------------------------------------------------------------------------

void ShapeBase::setHidden( bool hidden )
{
   if( hidden != mHidden )
   {
      setMaskBits( HideCloakMask );

      if( mHidden )
      {
         addToScene();
         setProcessTick( true );
      }
      else
      {
         removeFromScene();
         setProcessTick( false );
      }

      mHidden = hidden;
   }
}

//--------------------------------------------------------------------------

void ShapeBaseConvex::findNodeTransform()
{
   S32 dl = pShapeBase->mDataBlock->collisionDetails[hullId];

   TSShapeInstance* si = pShapeBase->getShapeInstance();
   TSShape* shape = si->getShape();

   const TSShape::Detail* detail = &shape->details[dl];
   const S32 subs = detail->subShapeNum;
   const S32 start = shape->subShapeFirstObject[subs];
   const S32 end = start + shape->subShapeNumObjects[subs];

   // Find the first object that contains a mesh for this
   // detail level. There should only be one mesh per
   // collision detail level.
   for (S32 i = start; i < end; i++) 
   {
      const TSShape::Object* obj = &shape->objects[i];
      if (obj->numMeshes && detail->objectDetailNum < obj->numMeshes) 
      {
         nodeTransform = &si->mNodeTransforms[obj->nodeIndex];
         return;
      }
   }
   return;
}

const MatrixF& ShapeBaseConvex::getTransform() const
{
   // If the transform isn't specified, it's assumed to be the
   // origin of the shape.
   const MatrixF& omat = (transform != 0)? *transform: mObject->getTransform();

   // Multiply on the mesh shape offset
   // tg: Returning this static here is not really a good idea, but
   // all this Convex code needs to be re-organized.
   if (nodeTransform) {
      static MatrixF mat;
      mat.mul(omat,*nodeTransform);
      return mat;
   }
   return omat;
}

Box3F ShapeBaseConvex::getBoundingBox() const
{
   const MatrixF& omat = (transform != 0)? *transform: mObject->getTransform();
   return getBoundingBox(omat, mObject->getScale());
}

Box3F ShapeBaseConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
   Box3F newBox = box;
   newBox.minExtents.convolve(scale);
   newBox.maxExtents.convolve(scale);
   mat.mul(newBox);
   return newBox;
}

Point3F ShapeBaseConvex::support(const VectorF& v) const
{
   TSShape::ConvexHullAccelerator* pAccel =
      pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
   AssertFatal(pAccel != NULL, "Error, no accel!");

   F32 currMaxDP = mDot(pAccel->vertexList[0], v);
   U32 index = 0;
   for (U32 i = 1; i < pAccel->numVerts; i++) {
      F32 dp = mDot(pAccel->vertexList[i], v);
      if (dp > currMaxDP) {
         currMaxDP = dp;
         index = i;
      }
   }

   return pAccel->vertexList[index];
}


void ShapeBaseConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{
   cf->material = 0;
   cf->object = mObject;

   TSShape::ConvexHullAccelerator* pAccel =
      pShapeBase->mShapeInstance->getShape()->getAccelerator(pShapeBase->mDataBlock->collisionDetails[hullId]);
   AssertFatal(pAccel != NULL, "Error, no accel!");

   F32 currMaxDP = mDot(pAccel->vertexList[0], n);
   U32 index = 0;
   U32 i;
   for (i = 1; i < pAccel->numVerts; i++) {
      F32 dp = mDot(pAccel->vertexList[i], n);
      if (dp > currMaxDP) {
         currMaxDP = dp;
         index = i;
      }
   }

   const U8* emitString = pAccel->emitStrings[index];
   U32 currPos = 0;
   U32 numVerts = emitString[currPos++];
   for (i = 0; i < numVerts; i++) {
      cf->mVertexList.increment();
      U32 index = emitString[currPos++];
      mat.mulP(pAccel->vertexList[index], &cf->mVertexList.last());
   }

   U32 numEdges = emitString[currPos++];
   for (i = 0; i < numEdges; i++) {
      U32 ev0 = emitString[currPos++];
      U32 ev1 = emitString[currPos++];
      cf->mEdgeList.increment();
      cf->mEdgeList.last().vertex[0] = ev0;
      cf->mEdgeList.last().vertex[1] = ev1;
   }

   U32 numFaces = emitString[currPos++];
   for (i = 0; i < numFaces; i++) {
      cf->mFaceList.increment();
      U32 plane = emitString[currPos++];
      mat.mulV(pAccel->normalList[plane], &cf->mFaceList.last().normal);
      for (U32 j = 0; j < 3; j++)
         cf->mFaceList.last().vertex[j] = emitString[currPos++];
   }
}


void ShapeBaseConvex::getPolyList(AbstractPolyList* list)
{
   list->setTransform(&pShapeBase->getTransform(), pShapeBase->getScale());
   list->setObject(pShapeBase);

   pShapeBase->mShapeInstance->animate(pShapeBase->mDataBlock->collisionDetails[hullId]);
   pShapeBase->mShapeInstance->buildPolyList(list,pShapeBase->mDataBlock->collisionDetails[hullId]);
}


//--------------------------------------------------------------------------

bool ShapeBase::isInvincible()
{
   if( mDataBlock )
   {
      return mDataBlock->isInvincible;
   }
   return false;
}

void ShapeBase::startFade( F32 fadeTime, F32 fadeDelay, bool fadeOut )
{
   setMaskBits(HideCloakMask);
   mFadeElapsedTime = 0;
   mFading = true;
   if(fadeDelay < 0)
      fadeDelay = 0;
   if(fadeTime < 0)
      fadeTime = 0;
   mFadeTime = fadeTime;
   mFadeDelay = fadeDelay;
   mFadeOut = fadeOut;
   mFadeVal = F32(mFadeOut);
}

//--------------------------------------------------------------------------

void ShapeBase::setShapeName(const char* name)
{
   if (!isGhost()) {
      if (name[0] != '\0') {
         // Use tags for better network performance
         // Should be a tag, but we'll convert to one if it isn't.
         if (name[0] == StringTagPrefixByte)
            mShapeNameHandle = NetStringHandle(U32(dAtoi(name + 1)));
         else
            mShapeNameHandle = NetStringHandle(name);
      }
      else {
         mShapeNameHandle = NetStringHandle();
      }
      setMaskBits(NameMask);
   }
}


void ShapeBase::setSkinName(const char* name)
{
   if (!isGhost()) {
      if (name[0] != '\0') {

         // Use tags for better network performance
         // Should be a tag, but we'll convert to one if it isn't.
         if (name[0] == StringTagPrefixByte) {
            mSkinNameHandle = NetStringHandle(U32(dAtoi(name + 1)));
         }
         else {
            mSkinNameHandle = NetStringHandle(name);
         }
      }
      else {
         mSkinNameHandle = NetStringHandle();
      }
      setMaskBits(SkinMask);
   }
}

//----------------------------------------------------------------------------

void ShapeBase::reSkin()
{
   if ( isGhost() && mShapeInstance && mSkinNameHandle.isValidString() )
   {
      const char* newSkin = mSkinNameHandle.getString();
      mShapeInstance->reSkin( newSkin, mAppliedSkinName );
      mAppliedSkinName = newSkin;
      mSkinHash = _StringTable::hashString( newSkin );
   }
}

void ShapeBase::setCurrentWaterObject( WaterObject *obj )
{
   if ( obj )
      deleteNotify( obj );
   if ( mCurrentWaterObject )
      clearNotify( mCurrentWaterObject );

   mCurrentWaterObject = obj;
}

//--------------------------------------------------------------------------
//----------------------------------------------------------------------------
ConsoleMethod( ShapeBase, setHidden, void, 3, 3, "(bool show)")
{
   object->setHidden(dAtob(argv[2]));
}

ConsoleMethod( ShapeBase, isHidden, bool, 2, 2, "")
{
   return object->isHidden();
}

//----------------------------------------------------------------------------
ConsoleMethod( ShapeBase, playAudio, bool, 4, 4, "(int slot, SFXProfile profile)")
{
   U32 slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      SFXProfile* profile;
      if (Sim::findObject(argv[3],profile)) {
         object->playAudio(slot,profile);
         return true;
      }
   }
   return false;
}

ConsoleMethod( ShapeBase, stopAudio, bool, 3, 3, "(int slot)")
{
   U32 slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      object->stopAudio(slot);
      return true;
   }
   return false;
}


//----------------------------------------------------------------------------
ConsoleMethod( ShapeBase, playThread, bool, 3, 4, "(int slot, string sequenceName)")
{
   U32 slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (argc == 4) {
         if (object->getShape()) {
            S32 seq = object->getShape()->findSequence(argv[3]);
            if (seq != -1 && object->setThreadSequence(slot,seq))
               return true;
         }
      }
      else
         if (object->playThread(slot))
            return true;
   }
   return false;
}

ConsoleMethod( ShapeBase, setThreadDir, bool, 4, 4, "(int slot, bool isForward)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->setThreadDir(slot,dAtob(argv[3])))
         return true;
   }
   return false;
}

ConsoleMethod( ShapeBase, setThreadTimeScale, bool, 4, 4, "( int pSlot, float pTimeScale )" )
{
	return object->setThreadTimeScale( dAtoi( argv[2] ), dAtof( argv[3] ) );
}

ConsoleMethod( ShapeBase, stopThread, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->stopThread(slot))
         return true;
   }
   return false;
}

ConsoleMethod( ShapeBase, pauseThread, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxScriptThreads) {
      if (object->pauseThread(slot))
         return true;
   }
   return false;
}

//----------------------------------------------------------------------------
ConsoleMethod( ShapeBase, mountImage, bool, 4, 6, "(ShapeBaseImageData image, int slot, bool loaded=true, string skinTag=NULL)")
{
   ShapeBaseImageData* imageData;
   if (Sim::findObject(argv[2],imageData)) {
      U32 slot = dAtoi(argv[3]);
      bool loaded = (argc == 5)? dAtob(argv[4]): true;
      NetStringHandle team;
      if(argc == 6)
      {
         if(argv[5][0] == StringTagPrefixByte)
            team = NetStringHandle(U32(dAtoi(argv[5]+1)));
      }
      if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
         object->mountImage(imageData,slot,loaded,team);
   }
   return false;
}

ConsoleMethod( ShapeBase, unmountImage, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->unmountImage(slot);
   return false;
}

ConsoleMethod( ShapeBase, getMountedImage, S32, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      if (ShapeBaseImageData* data = object->getMountedImage(slot))
         return data->getId();
   return 0;
}

ConsoleMethod( ShapeBase, getPendingImage, S32, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      if (ShapeBaseImageData* data = object->getPendingImage(slot))
         return data->getId();
   return 0;
}

ConsoleMethod( ShapeBase, isImageFiring, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->isImageFiring(slot);
   return false;
}

ConsoleMethod( ShapeBase, isImageMounted, bool, 3, 3, "(ShapeBaseImageData db)")
{
   ShapeBaseImageData* imageData;
   if (Sim::findObject(argv[2],imageData))
      return object->isImageMounted(imageData);
   return false;
}

ConsoleMethod( ShapeBase, getMountSlot, S32, 3, 3, "(ShapeBaseImageData db)")
{
   ShapeBaseImageData* imageData;
   if (Sim::findObject(argv[2],imageData))
      return object->getMountSlot(imageData);
   return -1;
}

ConsoleMethod( ShapeBase, getImageSkinTag, S32, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageSkinTag(slot).getIndex();
   return -1;
}

ConsoleMethod( ShapeBase, getImageState, const char*, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageState(slot);
   return "Error";
}

ConsoleMethod( ShapeBase, getImageTrigger, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageTriggerState(slot);
   return false;
}

ConsoleMethod( ShapeBase, setImageTrigger, bool, 4, 4, "(int slot, bool isTriggered)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageTriggerState(slot,dAtob(argv[3]));
      return object->getImageTriggerState(slot);
   }
   return false;
}

ConsoleMethod( ShapeBase, getImageAltTrigger, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageAltTriggerState(slot);
   return false;
}

ConsoleMethod( ShapeBase, setImageAltTrigger, bool, 4, 4, "(int slot, bool isTriggered)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      object->setImageAltTriggerState(slot,dAtob(argv[3]));
      return object->getImageAltTriggerState(slot);
   }
   return false;
}

ConsoleMethod( ShapeBase, getImageAmmo, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageAmmoState(slot);
   return false;
}

ConsoleMethod( ShapeBase, setImageAmmo, bool, 4, 4, "(int slot, bool hasAmmo)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      bool ammo = dAtob(argv[3]);
      object->setImageAmmoState(slot,dAtob(argv[3]));
      return ammo;
   }
   return false;
}

ConsoleMethod( ShapeBase, getImageLoaded, bool, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      return object->getImageLoadedState(slot);
   return false;
}

ConsoleMethod( ShapeBase, setImageLoaded, bool, 4, 4, "(int slot, bool loaded)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      bool loaded = dAtob(argv[3]);
      object->setImageLoadedState(slot, dAtob(argv[3]));
      return loaded;
   }
   return false;
}

ConsoleMethod( ShapeBase, getMuzzleVector, const char*, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      VectorF v;
      object->getMuzzleVector(slot,&v);
      char* buff = Con::getReturnBuffer(100);
      dSprintf(buff,100,"%g %g %g",v.x,v.y,v.z);
      return buff;
   }
   return "0 1 0";
}

ConsoleMethod( ShapeBase, getMuzzlePoint, const char*, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages) {
      Point3F p;
      object->getMuzzlePoint(slot,&p);
      char* buff = Con::getReturnBuffer(100);
      dSprintf(buff,100,"%g %g %g",p.x,p.y,p.z);
      return buff;
   }
   return "0 0 0";
}

ConsoleMethod( ShapeBase, getSlotTransform, const char*, 3, 3, "(int slot)")
{
   int slot = dAtoi(argv[2]);
   MatrixF xf(true);
   if (slot >= 0 && slot < ShapeBase::MaxMountedImages)
      object->getMountTransform(slot,&xf);

   Point3F pos;
   xf.getColumn(3,&pos);
   AngAxisF aa(xf);
   char* buff = Con::getReturnBuffer(200);
   dSprintf(buff,200,"%g %g %g %g %g %g %g",
            pos.x,pos.y,pos.z,aa.axis.x,aa.axis.y,aa.axis.z,aa.angle);
   return buff;
}

ConsoleMethod( ShapeBase, getAIRepairPoint, const char*, 2, 2, "Get the position at which the AI should stand to repair things.")
{
    Point3F pos = object->getAIRepairPoint();
   char* buff = Con::getReturnBuffer(200);
   dSprintf(buff,200,"%g %g %g", pos.x,pos.y,pos.z);
   return buff;
}

ConsoleMethod( ShapeBase, getVelocity, const char *, 2, 2, "")
{
   const VectorF& vel = object->getVelocity();
   char* buff = Con::getReturnBuffer(100);
   dSprintf(buff,100,"%g %g %g",vel.x,vel.y,vel.z);
   return buff;
}

ConsoleMethod( ShapeBase, setVelocity, bool, 3, 3, "(Vector3F vel)")
{
   VectorF vel(0,0,0);
   dSscanf(argv[2],"%g %g %g",&vel.x,&vel.y,&vel.z);
   object->setVelocity(vel);
   return true;
}

ConsoleMethod( ShapeBase, applyImpulse, bool, 4, 4, "(Point3F Pos, VectorF vel)")
{
   Point3F pos(0,0,0);
   VectorF vel(0,0,0);
   dSscanf(argv[2],"%g %g %g",&pos.x,&pos.y,&pos.z);
   dSscanf(argv[3],"%g %g %g",&vel.x,&vel.y,&vel.z);
   object->applyImpulse(pos,vel);
   return true;
}

ConsoleMethod( ShapeBase, getEyeVector, const char*, 2, 2, "")
{
   MatrixF mat;
   object->getEyeTransform(&mat);
   VectorF v2;
   mat.getColumn(1,&v2);
   char* buff = Con::getReturnBuffer(100);
   dSprintf(buff, 100,"%g %g %g",v2.x,v2.y,v2.z);
   return buff;
}

ConsoleMethod( ShapeBase, getEyePoint, const char*, 2, 2, "")
{
   MatrixF mat;
   object->getEyeTransform(&mat);
   Point3F ep;
   mat.getColumn(3,&ep);
   char* buff = Con::getReturnBuffer(100);
   dSprintf(buff, 100,"%g %g %g",ep.x,ep.y,ep.z);
   return buff;
}

ConsoleMethod( ShapeBase, getEyeTransform, const char*, 2, 2, "")
{
   MatrixF mat;
   object->getEyeTransform(&mat);

   Point3F pos;
   mat.getColumn(3,&pos);
   AngAxisF aa(mat);
   char* buff = Con::getReturnBuffer(100);
   dSprintf(buff,100,"%g %g %g %g %g %g %g",
            pos.x,pos.y,pos.z,aa.axis.x,aa.axis.y,aa.axis.z,aa.angle);
   return buff;
}

ConsoleMethod( ShapeBase, getLookAtPoint, const char*, 2, 4,
   "( [float distance, bitset mask] )"
   " - Return look-at information as \"Object HitX HitY HitZ [Material]\" or empty string for no hit" )
{
   F32 distance = 2000.f;
   if( argc > 2 )
      distance = dAtof( argv[ 2 ] );
      
   S32 typeMask = 0xFFFFFFFF; // Default: hit anything.
   if( argc > 3 )
      typeMask = dAtoi( argv[ 3 ] );
      
   MatrixF mat;
   object->getEyeTransform( &mat );
   
   // Get eye vector.
   
   VectorF eyeVector;
   mat.getColumn( 1, &eyeVector );
   
   // Get eye position.
   
   VectorF eyePos;
   mat.getColumn( 3, &eyePos );
   
   // Make sure the eye vector covers the distance.
   
   eyeVector *= distance;
   
   // Do a container search.
   
   VectorF start = eyePos;
   VectorF end = eyePos + eyeVector;
   
   RayInfo ri;
   if( !gServerContainer.castRay( start, end, typeMask, &ri ) || !ri.object )
      return ""; // No hit.
   
   // Gather hit info.
      
   enum { BUFFER_SIZE = 256 };
   char* buffer = Con::getReturnBuffer( BUFFER_SIZE );
   if( ri.material )
      dSprintf( buffer, BUFFER_SIZE, "%u %f %f %f %u",
         ri.object->getId(),
         ri.point.x,
         ri.point.y,
         ri.point.z,
         ri.material->getMaterial()->getId() );
   else
      dSprintf( buffer, BUFFER_SIZE, "%u %f %f %f",
         ri.object->getId(),
         ri.point.x,
         ri.point.y,
         ri.point.z );

   return buffer;
}

ConsoleMethod( ShapeBase, setEnergyLevel, void, 3, 3, "(float level)")
{
   object->setEnergyLevel(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, getEnergyLevel, F32, 2, 2, "")
{
   return object->getEnergyLevel();
}

ConsoleMethod( ShapeBase, getEnergyPercent, F32, 2, 2, "")
{
   return object->getEnergyValue();
}

ConsoleMethod( ShapeBase, setDamageLevel, void, 3, 3, "(float level)")
{
   object->setDamageLevel(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, getDamageLevel, F32, 2, 2, "")
{
   return object->getDamageLevel();
}

ConsoleMethod( ShapeBase, getDamagePercent, F32, 2, 2, "")
{
   return object->getDamageValue();
}

ConsoleMethod( ShapeBase, setDamageState, bool, 3, 3, "(string state)")
{
   return object->setDamageState(argv[2]);
}

ConsoleMethod( ShapeBase, getDamageState, const char*, 2, 2, "")
{
   return object->getDamageStateName();
}

ConsoleMethod( ShapeBase, isDestroyed, bool, 2, 2, "")
{
   return object->isDestroyed();
}

ConsoleMethod( ShapeBase, isDisabled, bool, 2, 2, "True if the state is not Enabled.")
{
   return object->getDamageState() != ShapeBase::Enabled;
}

ConsoleMethod( ShapeBase, isEnabled, bool, 2, 2, "")
{
   return object->getDamageState() == ShapeBase::Enabled;
}

ConsoleMethod( ShapeBase, applyDamage, void, 3, 3, "(float amt)")
{
   object->applyDamage(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, applyRepair, void, 3, 3, "(float amt)")
{
   object->applyRepair(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, setRepairRate, void, 3, 3, "(float amt)")
{
   F32 rate = dAtof(argv[2]);
   if(rate < 0)
      rate = 0;
   object->setRepairRate(rate);
}

ConsoleMethod( ShapeBase, getRepairRate, F32, 2, 2, "")
{
   return object->getRepairRate();
}

ConsoleMethod( ShapeBase, setRechargeRate, void, 3, 3, "(float rate)")
{
   object->setRechargeRate(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, getRechargeRate, F32, 2, 2, "")
{
   return object->getRechargeRate();
}

ConsoleMethod( ShapeBase, getControllingClient, S32, 2, 2, "Returns a GameConnection.")
{
   if (GameConnection* con = object->getControllingClient())
      return con->getId();
   return 0;
}

ConsoleMethod( ShapeBase, getControllingObject, S32, 2, 2, "")
{
   if (ShapeBase* con = object->getControllingObject())
      return con->getId();
   return 0;
}

// return true if can cloak, otherwise the reason why object cannot cloak
ConsoleMethod( ShapeBase, canCloak, bool, 2, 2, "")
{
   return true;
}

ConsoleMethod( ShapeBase, setCloaked, void, 3, 3, "(bool isCloaked)")
{
   bool cloaked = dAtob(argv[2]);
   if (object->isServerObject())
      object->setCloakedState(cloaked);
}

ConsoleMethod( ShapeBase, isCloaked, bool, 2, 2, "")
{
   return object->getCloakedState();
}

ConsoleMethod( ShapeBase, setDamageFlash, void, 3, 3, "(float lvl)")
{
   F32 flash = dAtof(argv[2]);
   if (object->isServerObject())
      object->setDamageFlash(flash);
}

ConsoleMethod( ShapeBase, getDamageFlash, F32, 2, 2, "")
{
   return object->getDamageFlash();
}

ConsoleMethod( ShapeBase, setWhiteOut, void, 3, 3, "(float flashLevel)")
{
   F32 flash = dAtof(argv[2]);
   if (object->isServerObject())
      object->setWhiteOut(flash);
}

ConsoleMethod( ShapeBase, getWhiteOut, F32, 2, 2, "")
{
   return object->getWhiteOut();
}

ConsoleMethod( ShapeBase, getCameraFov, F32, 2, 2, "")
{
   if (object->isServerObject())
      return object->getCameraFov();
   return 0.0;
}

ConsoleMethod( ShapeBase, setCameraFov, void, 3, 3, "(float fov)")
{
   if (object->isServerObject())
      object->setCameraFov(dAtof(argv[2]));
}

ConsoleMethod( ShapeBase, setInvincibleMode, void, 4, 4, "(float time, float speed)")
{
   object->setupInvincibleEffect(dAtof(argv[2]), dAtof(argv[3]));
}

ConsoleMethod( ShapeBase, startFade, void, 5, 5, "( int fadeTimeMS, int fadeDelayMS, bool fadeOut )")
{
   U32   fadeTime;
   U32   fadeDelay;
   bool  fadeOut;

   dSscanf(argv[2], "%d", &fadeTime );
   dSscanf(argv[3], "%d", &fadeDelay );
   fadeOut = dAtob(argv[4]);

   object->startFade( fadeTime / 1000.0, fadeDelay / 1000.0, fadeOut );
}

ConsoleMethod( ShapeBase, setDamageVector, void, 3, 3, "(Vector3F origin)")
{
   VectorF normal;
   dSscanf(argv[2], "%g %g %g", &normal.x, &normal.y, &normal.z);
   normal.normalize();
   object->setDamageDir(VectorF(normal.x, normal.y, normal.z));
}

ConsoleMethod( ShapeBase, setShapeName, void, 3, 3, "(string tag)")
{
   object->setShapeName(argv[2]);
}


ConsoleMethod( ShapeBase, setSkinName, void, 3, 3, "(string tag)")
{
   object->setSkinName(argv[2]);
}

ConsoleMethod( ShapeBase, getShapeName, const char*, 2, 2, "")
{
   return object->getShapeName();
}


ConsoleMethod( ShapeBase, getSkinName, const char*, 2, 2, "")
{
   return object->getSkinName();
}

//----------------------------------------------------------------------------
void ShapeBase::consoleInit()
{
   Con::addVariable("SB::DFDec", TypeF32, &sDamageFlashDec);
   Con::addVariable("SB::WODec", TypeF32, &sWhiteoutDec);
   Con::addVariable("pref::environmentMaps", TypeBool, &gRenderEnvMaps);
}

void ShapeBase::_updateHiddenMeshes()
{
   if ( !mShapeInstance )
      return;

   // This may happen at some point in the future... lets
   // detect it so that it can be fixed at that time.
   AssertFatal( mMeshHidden.getSize() == mShapeInstance->mMeshObjects.size(),
      "ShapeBase::_updateMeshVisibility() - Mesh visibility size mismatch!" );

   for ( U32 i = 0; i < mMeshHidden.getSize(); i++ )
      setMeshHidden( i, mMeshHidden.test( i ) );
}

void ShapeBase::setMeshHidden( const char *meshName, bool forceHidden )
{
   setMeshHidden( mDataBlock->mShape->findObject( meshName ), forceHidden );
}

void ShapeBase::setMeshHidden( S32 meshIndex, bool forceHidden )
{
   if ( meshIndex == -1 || meshIndex >= mMeshHidden.getSize() )
      return;

   if ( forceHidden )
      mMeshHidden.set( meshIndex );   
   else
      mMeshHidden.clear( meshIndex );   

   if ( mShapeInstance )
      mShapeInstance->setMeshForceHidden( meshIndex, forceHidden );

   setMaskBits( MeshHiddenMask );
}

void ShapeBase::setAllMeshesHidden( bool forceHidden )
{
   if ( forceHidden )
      mMeshHidden.set();
   else
      mMeshHidden.clear();

   if ( mShapeInstance )
   {
      for ( U32 i = 0; i < mMeshHidden.getSize(); i++ )
         mShapeInstance->setMeshForceHidden( i, forceHidden );
   }

   setMaskBits( MeshHiddenMask );
}

ConsoleMethod( ShapeBase, setAllMeshesHidden, void, 3, 3, 
   "( bool forceHidden )\n"
   "Set the hidden state on all the shape meshes." )
{
   object->setAllMeshesHidden( dAtob( argv[2] ) );
}

ConsoleMethod( ShapeBase, setMeshHidden, void, 4, 4, 
   "( string meshName, bool forceHidden )\n"
   "Set the force hidden state on the named mesh." )
{
   object->setMeshHidden( argv[2], dAtob( argv[3] ) );
}

// Some development-handy functions
#ifndef TORQUE_SHIPPING

void ShapeBase::dumpMeshVisibility()
{
   if ( !mShapeInstance )
      return;

   const Vector<TSShapeInstance::MeshObjectInstance> &meshes = mShapeInstance->mMeshObjects;

   for ( U32 i = 0; i < meshes.size(); i++)
   {
      const TSShapeInstance::MeshObjectInstance &mesh = meshes[i];

      const String &meshName = mDataBlock->mShape->getMeshName( i );

      Con::printf( "%d - %s - forceHidden = %s, visibility = %f", 
         i,
         meshName.c_str(),
         mesh.forceHidden ? "true" : "false",
         mesh.visible );
   }
}

ConsoleMethod( ShapeBase, dumpMeshVisibility, void, 2, 2, 
   "Prints list of visible and hidden meshes to the console for debugging purposes." )
{
   object->dumpMeshVisibility();
}

#endif // #ifndef TORQUE_SHIPPING

//------------------------------------------------------------------------
//These functions are duplicated in tsStatic, shapeBase, and interiorInstance.
//They each function a little differently; but achieve the same purpose of gathering
//target names/counts without polluting simObject.

ConsoleMethod( ShapeBase, getTargetName, const char*, 3, 3, "")
{
	S32 idx = dAtoi(argv[2]);

	ShapeBase *obj = dynamic_cast< ShapeBase* > ( object );
	if(obj)
		return obj->getShape()->getTargetName(idx);

	return "";
}

ConsoleMethod( ShapeBase, getTargetCount, S32, 2, 2, "")
{
	ShapeBase *obj = dynamic_cast< ShapeBase* > ( object );
	if(obj)
		return obj->getShape()->getTargetCount();

	return -1;
}

// This method is able to change materials per map to with others. The material that is being replaced is being mapped to
// unmapped_mat as a part of this transition

// Warning, right now this only sort of works. It doesn't do a live update like it should.
ConsoleMethod( ShapeBase, changeMaterial, void, 5, 5, "(mapTo, fromMaterial, ToMaterial)")
{
	// initilize server/client versions
	ShapeBase *serverObj = object;
	ShapeBase *clientObj = dynamic_cast< ShapeBase* > ( object->getClientObject() );

	if(serverObj)
	{
		// Lets get ready to switch out materials
		Material *oldMat = dynamic_cast<Material*>(Sim::findObject(argv[3]));
		Material *newMat = dynamic_cast<Material*>(Sim::findObject(argv[4]));

		// if no valid new material, theres no reason for doing this
		if( !newMat )
			return;
		
		// Lets remap the old material off, so as to let room for our current material room to claim its spot
		if( oldMat )
			oldMat->mMapTo = String("unmapped_mat");

		newMat->mMapTo = argv[2];
		
		// Map the material in the in the matmgr
		MATMGR->mapMaterial( argv[2], argv[4] );
		
		U32 i = 0;
		// Replace instances with the new material being traded in. Lets make sure that we only
		// target the specific targets per inst. For shape base class we have to update the server/client objects
		// seperately so both represent our changes
		for (; i < serverObj->getShape()->materialList->getMaterialNameList().size(); i++)
		{
			if( String(argv[2]) == serverObj->getShape()->materialList->getMaterialName(i))
			{
				delete [] serverObj->getShape()->materialList->mMatInstList[i];
				serverObj->getShape()->materialList->mMatInstList[i] = newMat->createMatInstance();

				delete [] clientObj->getShapeInstance()->mMaterialList->mMatInstList[i];
				clientObj->getShapeInstance()->mMaterialList->mMatInstList[i] = newMat->createMatInstance();
				break;
			}
		}


		// Finish up preparing the material instances for rendering. Prep render for both
		// server and client objects
		const GFXVertexFormat *flags = getGFXVertexFormat<GFXVertexPNTTB>();
		FeatureSet features = MATMGR->getDefaultFeatures();
		serverObj->getShape()->materialList->getMaterialInst(i)->init( features, flags );
		clientObj->getShapeInstance()->mMaterialList->getMaterialInst(i)->init( features, flags );
	}
}

ConsoleMethod( ShapeBase, getModelFile, const char *, 2, 2, "getModelFile( String )")
{

	GameBaseData * datablock = object->getDataBlock();
	if( !datablock )
		return String::EmptyString;

	const char *fieldName = StringTable->insert( String("shapeFile") );
   return datablock->getDataField( fieldName, NULL );
}
// SphyxGames -> Melee
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
// SphyxGames <- Melee
