//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_MotionTrail
#define _H_MotionTrail

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _COLOR_H_
#include "Core/color.h"
#endif
#ifndef _TVECTOR_H_
#include "Core/tVector.h"
#endif

class Point3F;

struct MotionTrailQuad
{
   Point3F  start;
   Point3F  end;
   ColorF   color;
   F32      ElapsedTime;
   F32      startAlpha;

   MotionTrailQuad()
   {
      start.set( 0.0, 0.0, 0.0 );
      end.set( 0.0, 0.0, 0.0 );
      color.set( 0.0, 0.0, 0.0 );
      startAlpha = 1.0;
      ElapsedTime = 0.0;
   }
};

//--------------------------------------
// DataBlock for MotionTrail
class MotionTrailData : public GameBaseData
{
public:
   typedef GameBaseData Parent;
   static int sBlendMap[];

   MotionTrailData();

   static void initPersistFields();
   void packData(BitStream* stream);
   void unpackData(BitStream* stream);

   bool onAdd();

S32srcBlend;
S32dstBlend;
S32numQuads;
F32     lifetime;
F32curveDetail;
boolshowAlways;
ColorF  color;
    StringTableEntry trailTextureName;

   DECLARE_CONOBJECT(MotionTrailData);
};
//--------------------------------------


class MotionTrail : public GameBase
{
public:

typedef GameBase Parent;

private:

MotionTrailData* mDataBlock;

Vector < MotionTrailQuad > quads;

TextureHandle mTextureHandle;// Trail texture handle
int mSrcBlend;
int mDstBlend;
int mNumQuads;

void prerender();
void postrender();

// Helper function, to do a linear interpolation of two floats
inline F32 interpolate(const F32& _rFrom, const F32& _to, const F32 _factor)
{
AssertFatal(_factor >= 0.0f && _factor <= 1.0f, "Out of bound interpolation
factor");
F32 ret;
ret = (_rFrom * (1.0f - _factor)) + (_to * _factor);

return ret;
};

public:
   MotionTrail();
   virtual    ~MotionTrail();

   // Base class overwrites
MotionTrailData *getDataBlock(){ return mDataBlock; }
bool onNewDataBlock(GameBaseData* dptr);

   void  updateTrail(Point3F &start,Point3F &end);
   void  addQuad( const Point3F &start, const Point3F &end );
   void  calcLineAlpha();
   U32   numQuads(){ return quads.size(); }
   void  renderTrail(const int nCurrent,  const int nCount);

void setTexture();

protected:

Point3F mLastAddedPos;

bool onAdd();
void onRemove();
bool prepRenderImage(SceneState* state, const U32 stateKey,
                                       const U32 /*startZone*/, const bool /*modifyBaseState*/);
void renderObject(SceneState* state, SceneRenderImage*);
void advanceTime(F32 dt);
void update(F32 dt);
virtual void  processTick (const Move* move);
};

#endif // _H_MotionTrail
