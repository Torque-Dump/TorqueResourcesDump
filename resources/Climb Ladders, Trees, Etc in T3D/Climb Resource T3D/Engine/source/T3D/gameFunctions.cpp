//-----------------------------------------------------------------------------
// Torque 3D
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/gameFunctions.h"
#include "T3D/gameConnection.h"
#include "T3D/camera.h"
#include "console/consoleTypes.h"
#include "gfx/sim/debugDraw.h"
#include "gui/3d/guiTSControl.h"
#include "sceneGraph/sceneRoot.h"
#include "sceneGraph/decalManager.h"
#include "sceneGraph/pathManager.h"
#include "terrain/terrRender.h"
#include "ts/tsShapeInstance.h"
#include "core/util/journal/process.h"
#include "materials/materialManager.h"


extern void ShowInit();

// Register the initialization and shutdown routines
static bool Init3dSubsystems();
static bool Shutdown3dSubsystems();

static ProcessRegisterInit sgInit(Init3dSubsystems);
static ProcessRegisterShutdown sgShutdown(Shutdown3dSubsystems);

//------------------------------------------------------------------------------
/// Camera and FOV info
namespace {

   const  U32 MaxZoomSpeed             = 2000;     ///< max number of ms to reach target FOV

   static F32 sConsoleCameraFov        = 90.f;     ///< updated to camera FOV each frame
   static F32 sDefaultFov              = 90.f;     ///< normal FOV
   static F32 sCameraFov               = 90.f;     ///< current camera FOV
   static F32 sTargetFov               = 90.f;     ///< the desired FOV
   static F32 sLastCameraUpdateTime    = 0;        ///< last time camera was updated
   static S32 sZoomSpeed               = 500;      ///< ms per 90deg fov change

   /// A scale to apply to the normal visible distance
   /// typically used for tuning performance.
   static F32 sVisDistanceScale = 1.0f;

} // namespace {}

// query
static SimpleQueryList sgServerQueryList;
static U32 sgServerQueryIndex = 0;

//SERVER FUNCTIONS ONLY
ConsoleFunctionGroupBegin( Containers, "Spatial query functions. <b>Server side only!</b>");

ConsoleFunction(containerFindFirst, const char*, 6, 6, "(bitset type, Point3F point, float x, float y, float z)"
                "Find objects matching the bitmask type within a box centered at point, with extents x, y, z.\n\n"
                "Returns the first object found; thereafter, you can get more results using containerFindNext().")
{
   //find out what we're looking for
   U32 typeMask = U32(dAtoi(argv[1]));

   //find the center of the container volume
   Point3F origin(0.0f, 0.0f, 0.0f);
   dSscanf(argv[2], "%g %g %g", &origin.x, &origin.y, &origin.z);

   //find the box dimensions
   Point3F size(0.0f, 0.0f, 0.0f);
   size.x = mFabs(dAtof(argv[3]));
   size.y = mFabs(dAtof(argv[4]));
   size.z = mFabs(dAtof(argv[5]));

   //build the container volume
   Box3F queryBox;
   queryBox.minExtents = origin;
   queryBox.maxExtents = origin;
   queryBox.minExtents -= size;
   queryBox.maxExtents += size;

   //initialize the list, and do the query
   sgServerQueryList.mList.clear();
   gServerContainer.findObjects(queryBox, typeMask, SimpleQueryList::insertionCallback, &sgServerQueryList);

   //return the first element
   sgServerQueryIndex = 0;
   char *buff = Con::getReturnBuffer(100);
   if (sgServerQueryList.mList.size())
      dSprintf(buff, 100, "%d", sgServerQueryList.mList[sgServerQueryIndex++]->getId());
   else
      buff[0] = '\0';

   return buff;
}

ConsoleFunction( containerFindNext, const char*, 1, 1, "Get more results from a previous call to containerFindFirst().")
{
   //return the next element
   char *buff = Con::getReturnBuffer(100);
   if (sgServerQueryIndex < sgServerQueryList.mList.size())
      dSprintf(buff, 100, "%d", sgServerQueryList.mList[sgServerQueryIndex++]->getId());
   else
      buff[0] = '\0';

   return buff;
}

ConsoleFunctionGroupEnd( Containers );

//------------------------------------------------------------------------------

bool GameGetCameraTransform(MatrixF *mat, Point3F *velocity)
{
   // Return the position and velocity of the control object
   GameConnection* connection = GameConnection::getConnectionToServer();
   return connection && connection->getControlCameraTransform(0, mat) &&
      connection->getControlCameraVelocity(velocity);
}

//------------------------------------------------------------------------------
ConsoleFunctionGroupBegin( CameraFunctions, "Functions controlling the global camera properties defined in main.cc.");

ConsoleFunction(setDefaultFov, void, 2,2, "(defaultFov) - Set the default FOV for a camera.")
{
   argc;
   sDefaultFov = mClampF(dAtof(argv[1]), MinCameraFov, MaxCameraFov);
   if(sCameraFov == sTargetFov)
      sTargetFov = sDefaultFov;
}

ConsoleFunction(setZoomSpeed, void, 2,2, "(speed) - Set the zoom speed of the camera, in ms per 90deg FOV change.")
{
   argc;
   sZoomSpeed = mClamp(dAtoi(argv[1]), 0, MaxZoomSpeed);
}

ConsoleFunction(setFov, void, 2, 2, "(fov) - Set the FOV of the camera.")
{
   argc;
   sTargetFov = mClampF(dAtof(argv[1]), MinCameraFov, MaxCameraFov);
}

ConsoleFunctionGroupEnd( CameraFunctions );

F32 GameGetCameraFov()
{
   return(sCameraFov);
}

void GameSetCameraFov(F32 fov)
{
   sTargetFov = sCameraFov = fov;
}

void GameSetCameraTargetFov(F32 fov)
{
   sTargetFov = fov;
}

void GameUpdateCameraFov()
{
   F32 time = F32(Platform::getVirtualMilliseconds());

   // need to update fov?
   if(sTargetFov != sCameraFov)
   {
      F32 delta = time - sLastCameraUpdateTime;

      // snap zoom?
      if((sZoomSpeed == 0) || (delta <= 0.f))
         sCameraFov = sTargetFov;
      else
      {
         // gZoomSpeed is time in ms to zoom 90deg
         F32 step = 90.f * (delta / F32(sZoomSpeed));

         if(sCameraFov > sTargetFov)
         {
            sCameraFov -= step;
            if(sCameraFov < sTargetFov)
               sCameraFov = sTargetFov;
         }
         else
         {
            sCameraFov += step;
            if(sCameraFov > sTargetFov)
               sCameraFov = sTargetFov;
         }
      }
   }

   // the game connection controls the vertical and the horizontal
   GameConnection * connection = GameConnection::getConnectionToServer();
   if(connection)
   {
      // check if fov is valid on control object
      if(connection->isValidControlCameraFov(sCameraFov))
         connection->setControlCameraFov(sCameraFov);
      else
      {
         // will set to the closest fov (fails only on invalid control object)
         if(connection->setControlCameraFov(sCameraFov))
         {
            F32 setFov = sCameraFov;
            connection->getControlCameraFov(&setFov);
            sTargetFov = sCameraFov = setFov;
         }
      }
   }

   // update the console variable
   sConsoleCameraFov = sCameraFov;
   sLastCameraUpdateTime = time;
}
//--------------------------------------------------------------------------

#ifdef TORQUE_DEBUG
// ConsoleFunction(dumpTSShapes, void, 1, 1, "dumpTSShapes();")
// {
//    argc, argv;

//    FindMatch match("*.dts", 4096);
//    gResourceManager->findMatches(&match);

//    for (U32 i = 0; i < match.numMatches(); i++)
//    {
//       U32 j;
//       Resource<TSShape> shape = ResourceManager::get().load(match.matchList[i]);
//       if (bool(shape) == false)
//          Con::errorf(" aaa Couldn't load: %s", match.matchList[i]);

//       U32 numMeshes = 0, numSkins = 0;
//       for (j = 0; j < shape->meshes.size(); j++)
//          if (shape->meshes[j])
//             numMeshes++;
//       for (j = 0; j < shape->skins.size(); j++)
//          if (shape->skins[j])
//             numSkins++;

//      Con::printf(" aaa Shape: %s (%d meshes, %d skins)", match.matchList[i], numMeshes, numSkins);
//       Con::printf(" aaa   Meshes");
//       for (j = 0; j < shape->meshes.size(); j++)
//       {
//          if (shape->meshes[j])
//             Con::printf(" aaa     %d -> nf: %d, nmf: %d, nvpf: %d (%d, %d, %d, %d, %d)",
//                         shape->meshes[j]->meshType & TSMesh::TypeMask,
//                         shape->meshes[j]->numFrames,
//                         shape->meshes[j]->numMatFrames,
//                         shape->meshes[j]->vertsPerFrame,
//                         shape->meshes[j]->verts.size(),
//                         shape->meshes[j]->norms.size(),
//                         shape->meshes[j]->tverts.size(),
//                         shape->meshes[j]->primitives.size(),
//                         shape->meshes[j]->indices.size());
//       }
//       Con::printf(" aaa   Skins");
//       for (j = 0; j < shape->skins.size(); j++)
//       {
//          if (shape->skins[j])
//             Con::printf(" aaa     %d -> nf: %d, nmf: %d, nvpf: %d (%d, %d, %d, %d, %d)",
//                         shape->skins[j]->meshType & TSMesh::TypeMask,
//                         shape->skins[j]->numFrames,
//                         shape->skins[j]->numMatFrames,
//                         shape->skins[j]->vertsPerFrame,
//                         shape->skins[j]->verts.size(),
//                         shape->skins[j]->norms.size(),
//                         shape->skins[j]->tverts.size(),
//                         shape->skins[j]->primitives.size(),
//                         shape->skins[j]->indices.size());
//       }
//    }
// }
#endif

ConsoleFunction( getControlObjectAltitude, const char*, 1, 1, "Get distance from bottom of controlled object to terrain.")
{
   GameConnection* connection = GameConnection::getConnectionToServer();
   if (connection) {
      ShapeBase* pSB = dynamic_cast<ShapeBase*>(connection->getControlObject());
      if (pSB != NULL && pSB->isClientObject())
      {
         Point3F pos(0.f, 0.f, 0.f);

         // if this object is mounted, then get the bottom position of the mount's bbox
         if(pSB->getObjectMount())
         {
            static Point3F BoxPnts[] = {
               Point3F(0.0f,0.0f,0.0f),
                  Point3F(0.0f,0.0f,1.0f),
                  Point3F(0.0f,1.0f,0.0f),
                  Point3F(0.0f,1.0f,1.0f),
                  Point3F(1.0f,0.0f,0.0f),
                  Point3F(1.0f,0.0f,1.0f),
                  Point3F(1.0f,1.0f,0.0f),
                  Point3F(1.0f,1.0f,1.0f)
            };

            ShapeBase * mount = pSB->getObjectMount();
            Box3F box = mount->getObjBox();
            MatrixF mat = mount->getTransform();
            VectorF scale = mount->getScale();

            Point3F projPnts[8];
            F32 minZ = 1e30f;

            for(U32 i = 0; i < 8; i++)
            {
               Point3F pnt(BoxPnts[i].x ? box.maxExtents.x : box.minExtents.x,
                  BoxPnts[i].y ? box.maxExtents.y : box.minExtents.y,
                  BoxPnts[i].z ? box.maxExtents.z : box.minExtents.z);

               pnt.convolve(scale);
               mat.mulP(pnt, &projPnts[i]);

               if(projPnts[i].z < minZ)
                  pos = projPnts[i];
            }
         }
         else
            pSB->getTransform().getColumn(3, &pos);

         TerrainBlock* pBlock = gClientSceneGraph->getCurrentTerrain();
         if (pBlock != NULL) {
            Point3F terrPos = pos;
            pBlock->getWorldTransform().mulP(terrPos);
            terrPos.convolveInverse(pBlock->getScale());

            F32 height;
            if (pBlock->getHeight(Point2F(terrPos.x, terrPos.y), &height) == true) {
               terrPos.z = height;
               terrPos.convolve(pBlock->getScale());
               pBlock->getTransform().mulP(terrPos);

               pos.z -= terrPos.z;
            }
         }

         char* retBuf = Con::getReturnBuffer(128);
         dSprintf(retBuf, 128, "%g", mFloor(getMax(pos.z, 0.f)));
         return retBuf;
      }
   }

   return "0";
}

ConsoleFunction( getControlObjectSpeed, const char*, 1, 1, "Get speed (but not velocity) of controlled object.")
{
   GameConnection* connection = GameConnection::getConnectionToServer();
   if (connection)
   {
      ShapeBase* pSB = dynamic_cast<ShapeBase*>(connection->getControlObject());
      if (pSB != NULL && pSB->isClientObject()) {
         Point3F vel = pSB->getVelocity();
         F32 speed = vel.len();

         // We're going to force the formating to be what we want...
         F32 intPart = mFloor(speed);
         speed -= intPart;
         speed *= 10;
         speed  = mFloor(speed);

         char* retBuf = Con::getReturnBuffer(128);
         dSprintf(retBuf, 128, "%g.%g", intPart, speed);
         return retBuf;
      }
   }

   return "0";
}

bool GameProcessCameraQuery(CameraQuery *query)
{
   GameConnection* connection = GameConnection::getConnectionToServer();

   if (connection && connection->getControlCameraTransform(0.032f, &query->cameraMatrix))
   {
      query->object = dynamic_cast<ShapeBase*>(connection->getControlObject());
      query->nearPlane = Con::getFloatVariable( "$pref::Video::nearPlane", 0.1f);

      // Scale the normal visible distance by the performance 
      // tuning scale which we never let over 1.
      sVisDistanceScale = mClampF( sVisDistanceScale, 0.01f, 1.0f );
      query->farPlane = gClientSceneGraph->getVisibleDistance() * sVisDistanceScale;

      F32 cameraFov;
      if(!connection->getControlCameraFov(&cameraFov))
         return false;

      query->fov = mDegToRad(cameraFov);
      return true;
   }
   return false;
}

void GameRenderFilters(const CameraQuery& camq)
{
   // Stubbed out currently - check version history for old code.
}

void GameRenderWorld()
{
   PROFILE_START(GameRenderWorld);
   FrameAllocator::setWaterMark(0);

   gClientSceneGraph->renderScene( SPT_Diffuse );

   AssertFatal(FrameAllocator::getWaterMark() == 0,
      "Error, someone didn't reset the water mark on the frame allocator!");
   FrameAllocator::setWaterMark(0);
   PROFILE_END();
}


static void Process3D()
{
   MATMGR->updateTime();
}

static void RegisterGameFunctions()
{
   Con::addVariable( "$pref::visibleDistanceMod", TypeF32, &sVisDistanceScale );
   Con::addVariable( "$cameraFov", TypeF32, &sConsoleCameraFov );

   // Stuff game types into the console
   Con::setIntVariable("$TypeMasks::StaticObjectType",         StaticObjectType);
   Con::setIntVariable("$TypeMasks::EnvironmentObjectType",    EnvironmentObjectType);
   Con::setIntVariable("$TypeMasks::TerrainObjectType",        TerrainObjectType);
   Con::setIntVariable("$TypeMasks::InteriorObjectType",       InteriorObjectType);
   Con::setIntVariable("$TypeMasks::WaterObjectType",          WaterObjectType);
   Con::setIntVariable("$TypeMasks::TriggerObjectType",        TriggerObjectType);
   Con::setIntVariable("$TypeMasks::MarkerObjectType",         MarkerObjectType);
   Con::setIntVariable("$TypeMasks::GameBaseObjectType",       GameBaseObjectType);
   Con::setIntVariable("$TypeMasks::ShapeBaseObjectType",      ShapeBaseObjectType);
   Con::setIntVariable("$TypeMasks::CameraObjectType",         CameraObjectType);
   Con::setIntVariable("$TypeMasks::StaticShapeObjectType",    StaticShapeObjectType);
   Con::setIntVariable("$TypeMasks::PlayerObjectType",         PlayerObjectType);
   Con::setIntVariable("$TypeMasks::ItemObjectType",           ItemObjectType);
   Con::setIntVariable("$TypeMasks::VehicleObjectType",        VehicleObjectType);
   Con::setIntVariable("$TypeMasks::VehicleBlockerObjectType", VehicleBlockerObjectType);
   Con::setIntVariable("$TypeMasks::ProjectileObjectType",     ProjectileObjectType);
   Con::setIntVariable("$TypeMasks::ExplosionObjectType",      ExplosionObjectType);
   Con::setIntVariable("$TypeMasks::CorpseObjectType",         CorpseObjectType);
   Con::setIntVariable("$TypeMasks::DebrisObjectType",         DebrisObjectType);
   Con::setIntVariable("$TypeMasks::PhysicalZoneObjectType",   PhysicalZoneObjectType);
   Con::setIntVariable("$TypeMasks::StaticTSObjectType",       StaticTSObjectType);
   Con::setIntVariable("$TypeMasks::StaticRenderedObjectType", StaticRenderedObjectType);
   Con::setIntVariable("$TypeMasks::DamagableItemObjectType",  DamagableItemObjectType);   
   Con::setIntVariable("$TypeMasks::LightObjectType",          LightObjectType);
   Con::setIntVariable("$TypeMasks::ClimableItemObjectType",   ClimableItemObjectType); //Climb Resource

}

static bool Init3dSubsystems()
{
   // Misc other static subsystems.
   MaterialManager::createSingleton();

   // Client scenegraph & root zone.
   gClientSceneGraph = new SceneGraph(true);
   gClientSceneRoot  = new SceneRoot;
   gClientSceneGraph->addObjectToScene(gClientSceneRoot);

   // Server scenegraph & root zone.
   gServerSceneGraph = new SceneGraph(false);
   gServerSceneRoot  = new SceneRoot;
   gServerSceneGraph->addObjectToScene(gServerSceneRoot);

   // Decal manager (just for clients).
   gDecalManager = new DecalManager;
   gClientContainer.addObject(gDecalManager);
   gClientSceneGraph->addObjectToScene(gDecalManager);

   // Misc other subsystems.
   TSShapeInstance::init();
   DebugDrawer::init();
   PathManager::init();
   MoveManager::init();

   Process::notify(Process3D, PROCESS_TIME_ORDER);

   GameConnection::smFovUpdate.notify(GameSetCameraFov);

   RegisterGameFunctions();

   return true;
}

static bool Shutdown3dSubsystems()
{
   GameConnection::smFovUpdate.remove(GameSetCameraFov);

   Process::remove(Process3D);

   PathManager::destroy();
   TSShapeInstance::destroy();

   gClientSceneGraph->removeObjectFromScene(gDecalManager);
   gClientContainer.removeObject(gDecalManager);
   gClientSceneGraph->removeObjectFromScene(gClientSceneRoot);
   gServerSceneGraph->removeObjectFromScene(gServerSceneRoot);

   SAFE_DELETE(gClientSceneRoot);
   SAFE_DELETE(gServerSceneRoot);
   SAFE_DELETE(gClientSceneGraph);
   SAFE_DELETE(gServerSceneGraph);
   SAFE_DELETE(gDecalManager);

   MaterialManager::deleteSingleton();

   return true;
}
