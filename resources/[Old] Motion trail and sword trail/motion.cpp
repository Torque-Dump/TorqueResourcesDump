//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

#include "game/fx/motion.h"
#include "math/mMath.h"
#include "math/mSplinePatch.h"
#include "math/mCatmullRomPatch.h"
#include "sim/netConnection.h"
#include "dgl/dgl.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"

//--------------------------------------------------------------------------
//-------------------------------------- Datablock implementation

IMPLEMENT_CO_DATABLOCK_V1(MotionTrailData);

// Blend mode map
int MotionTrailData::sBlendMap[] = 
{
    GL_ZERO,    // 0
    GL_ONE,        // 1
    GL_SRC_COLOR,    // 2
    GL_ONE_MINUS_SRC_COLOR,    //3
    GL_SRC_ALPHA,    //4
    GL_ONE_MINUS_SRC_ALPHA, //5
    GL_DST_ALPHA,    //6
    GL_ONE_MINUS_DST_ALPHA,    //7
    GL_DST_COLOR, // 8
    GL_ONE_MINUS_DST_COLOR,    //9
    GL_SRC_ALPHA_SATURATE    //10
};

// Default constructor
MotionTrailData::MotionTrailData()
{
    // Init members
    srcBlend = 4;
    dstBlend = 5;
    lifetime = 0.4;
    numQuads = 100;
  showAlways = false;
    color.set( 1.0, 1.0, 0.0, 0.20 );
    curveDetail = 0.05f;
    trailTextureName = 0;
}
static MotionTrailData gDefaultMotionData;

IMPLEMENT_GETDATATYPE(MotionTrailData)
IMPLEMENT_SETDATATYPE(MotionTrailData)

// Register console variables
void MotionTrailData::initPersistFields()
{
    // Call to base class
  Parent::initPersistFields();

  Con::registerType("MotionTrailDataPtr", TypeMotionTrailDataPtr, sizeof(MotionTrailData*),
                    REF_GETDATATYPE(MotionTrailData),
                    REF_SETDATATYPE(MotionTrailData));

  addField("srcBlend",                TypeS32,    Offset(srcBlend,           MotionTrailData));
  addField("dstBlend",                TypeS32,    Offset(dstBlend,           MotionTrailData));
  addField("numberOfQuads",        TypeS32,    Offset(numQuads,           MotionTrailData));
  addField("showAlways",            TypeBool,   Offset(showAlways,          MotionTrailData));
  addField("lifetime",                TypeF32,    Offset(lifetime,           MotionTrailData));
  addField("color",                TypeColorF, Offset(color,              MotionTrailData));
  addField("curveDetail",          TypeF32,    Offset(curveDetail,        MotionTrailData));
  addField("trailTexture",         TypeString, Offset(trailTextureName,   MotionTrailData));
}

// Pack instance data for network transfer
void MotionTrailData::packData(BitStream* stream)
{
    // Call to base class
  Parent::packData(stream);

  stream->writeInt(srcBlend, 4);
  stream->writeInt(dstBlend, 4);
  stream->writeInt(numQuads, 9);
  stream->writeFloat(lifetime/5, 10);
  stream->writeFlag(showAlways);
  stream->write(color.alpha);
  stream->write(color.red);
  stream->write(color.green);
  stream->write(color.blue);
  stream->writeFloat(curveDetail, 10);
  stream->writeString(trailTextureName);
}

// Unpack instance data from network stream
void MotionTrailData::unpackData(BitStream* stream)
{
    // Call to base class
  Parent::unpackData(stream);

  srcBlend = stream->readInt(4); 
  dstBlend = stream->readInt(4); 
  numQuads = stream->readInt(9);
  lifetime = stream->readFloat(10)*5.0f;
  showAlways = stream->readFlag();
  stream->read(&color.alpha );
  stream->read(&color.red);
  stream->read(&color.green);
  stream->read(&color.blue);
  curveDetail = stream->readFloat(10);
  trailTextureName = stream->readSTString();
}

// Called, when the data block is added to the simulation
bool MotionTrailData::onAdd()
{
    // Call to base class
  if (Parent::onAdd() == false)
     return false;

  // Validate the parameters...

  if (lifetime < 0)
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) lifetime < 1 second", getName());
     lifetime = 0;
  }
  if (!color.isValidColor())
  {
       Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) Invalid color: (%.2f/%.2f/%.2f - %.2f)",
                getName(), color.red, color.green, color.blue, color.alpha);
       color = gDefaultMotionData.color;
  }

  if (curveDetail < 0.0f)
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) curveDetail < 0.0f", getName());
      curveDetail = gDefaultMotionData.curveDetail;
  }

  if ((srcBlend < 0) || (srcBlend > 10))
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) srcDest must be between 0 and 10", getName());
      srcBlend = gDefaultMotionData.srcBlend;
  }
  if ((dstBlend < 0) || (dstBlend > 10))
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) dstDest must be between 0 and 10", getName());
      dstBlend = gDefaultMotionData.dstBlend;
  }
  if ((numQuads < 0) || (numQuads> 511))
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) numQuads must be between 0 and 511", getName());
      numQuads = gDefaultMotionData.numQuads;
  }

  int strlen = dStrlen(trailTextureName);
  if (strlen < 0)
  {
     Con::warnf(ConsoleLogEntry::General, "MotionTrailData(%s) trailTextureName not set.", getName());
      trailTextureName = gDefaultMotionData.trailTextureName;
  }

  return true;
}

//----------------------------------------------------------------------------
//-------------------------------------- MotionTrail implementation

// Default constructor
MotionTrail::MotionTrail()
{
    // Init members
  mTextureHandle = NULL;
  mSrcBlend = GL_SRC_ALPHA;
  mDstBlend = GL_ONE_MINUS_SRC_ALPHA;
  mNumQuads = 100;

  // Preallocate vector
  quads.reserve( mNumQuads );

  // Give this item a name
  assignName("MotionTrail");
}

// Default constructor
MotionTrail::~MotionTrail()
{

}

// Called, when the trail is added to the simulation
bool MotionTrail::onAdd()
{
    // Call to base class
  if(!Parent::onAdd())
     return false;

     // Reset the World Box.
    resetWorldBox();

    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

  // Add to client scene graph
  gClientContainer.addObject(this);
  gClientSceneGraph->addObjectToScene(this);

  // Add to client process list
  removeFromProcessList();
  gClientProcessList.addObject(this);

  // Setup blending
  mSrcBlend = MotionTrailData::sBlendMap[mDataBlock->srcBlend];
  mDstBlend = MotionTrailData::sBlendMap[mDataBlock->dstBlend];

  // Setup quad array
  mNumQuads = mDataBlock->numQuads;
  quads.reserve(mNumQuads);

  return true;
}

// Called when the trail is removed from the simulation
void MotionTrail::onRemove()
{
    // Cleanup stuff goes here
  removeFromScene();

    // Call to base class
    Parent::onRemove();
}

// Called from gamebase to set a new datablock on this object.
bool MotionTrail::onNewDataBlock(GameBaseData* dptr)
{
  mDataBlock = dynamic_cast<MotionTrailData*>(dptr);
  if (!mDataBlock || !Parent::onNewDataBlock(dptr))
     return false;

  if(isClientObject())
        // Load the texture
        setTexture();

  // Inform script
  scriptOnNewDataBlock();

  return true;
}

// Called during scene traversal to prepare a render image
bool MotionTrail::prepRenderImage(SceneState* state, const U32 stateKey,
                                      const U32 /*startZone*/, const bool /*modifyBaseState*/)
{

    // Has something changes?
  if (isLastState(state, stateKey))
     return false;

  // Update the last state
  setLastState(state, stateKey);

  // This should be sufficient for most objects that don't manage zones, and
  //  don't need to return a specialized RenderImage...
  if (state->isObjectRendered(this))
  {
     SceneRenderImage* image = new SceneRenderImage;
     image->obj = this;
     image->isTranslucent = true;
     image->sortType = SceneRenderImage::Point;
     state->setImageRefPoint(this, image);

     state->insertRenderImage(image);
  }

  return false;
}

// Called to render the trail
void MotionTrail::renderObject(SceneState* state, SceneRenderImage*)
{
  if (numQuads() <= 1) return;
  S32 i;

  prerender();

  for( i=0; i<(quads.size())-1; i++ )
  {
     if( quads[i].ElapsedTime < mDataBlock->lifetime )
        renderTrail( i, quads.size());
  }

  postrender();

}

void MotionTrail::processTick (const Move* move)
{
    // Call to base class
    Parent::processTick(move);

}

// Called once during a frame to update the object
void MotionTrail::advanceTime(F32 dt)
{
    // Call to base class
    Parent::advanceTime(dt);

    // Update the trail appearance
   update(dt);
}


// Called by ShapeBase to update the trail with new quads
void MotionTrail::updateTrail(Point3F &start,Point3F &end)
{
   // check the trail "emit frequency", if we are due to emit a trail segment, then add one!
   if (numQuads() < 100)
   {
       addQuad(start,end);
   }
}

// Loads the configured texture
void MotionTrail::setTexture()
{
    if (!mTextureHandle)
        mTextureHandle = TextureHandle(mDataBlock->trailTextureName, BitmapTexture );
}


//--------------------------------------------------------------------------
// Add segment
//--------------------------------------------------------------------------
void MotionTrail::addQuad( const Point3F &start, const Point3F &end )
{
    // Check, if the new pos is somewhat different from the last one.
    if((mLastAddedPos-start).len() < 0.1f)
        return;
    // Update last saved position
    mLastAddedPos = start;

  MotionTrailQuad q;
  q.start = start;
  q.end = end;
  q.ElapsedTime = 0.0;
  q.color = mDataBlock->color;

  quads.push_back( q );
  
  int nCount = quads.size();
  
  if(1 == nCount)
  {
     // Reset object box
     mObjBox.min = VectorF(0,0,0);
     mObjBox.max = VectorF(0,0,0);
     resetWorldBox();
       // Update the world space position
      setPosition((start+end)/2.0f);
  }
  else
  {
     // Calculate new object box
     VectorF vecDist1 = start - getPosition();
     VectorF vecDist2 = end - getPosition();

     // Has vecDist1 new min values?
     if(mObjBox.min.x > vecDist1.x)
        mObjBox.min.x = vecDist1.x;
     if(mObjBox.min.y > vecDist1.y)
        mObjBox.min.y = vecDist1.y;
     if(mObjBox.min.z > vecDist1.z)
        mObjBox.min.z = vecDist1.z;
     // Has start new max values?
     if(mObjBox.max.x < vecDist1.x)
        mObjBox.max.x = vecDist1.x;
     if(mObjBox.max.y < vecDist1.y)
        mObjBox.max.y = vecDist1.y;
     if(mObjBox.max.z < vecDist1.z)
        mObjBox.max.z = vecDist1.z;
     // Has end new min values?
     if(mObjBox.min.x > vecDist2.x)
        mObjBox.min.x = vecDist2.x;
     if(mObjBox.min.y > vecDist2.y)
        mObjBox.min.y = vecDist2.y;
     if(mObjBox.min.z > vecDist2.z)
        mObjBox.min.z = vecDist2.z;
     // Has end new max values?
     if(mObjBox.max.x < vecDist2.x)
        mObjBox.max.x = vecDist2.x;
     if(mObjBox.max.y < vecDist2.y)
        mObjBox.max.y = vecDist2.y;
     if(mObjBox.max.z < vecDist2.z)
        mObjBox.max.z = vecDist2.z;

     // Update the world box
     resetWorldBox();
  }
}

//--------------------------------------------------------------------------
// PreRender: Setup OGL state
//--------------------------------------------------------------------------
void MotionTrail::prerender()
{
    if (mTextureHandle)
    {
        glBindTexture( GL_TEXTURE_2D, mTextureHandle.getGLName() );
        glEnable( GL_TEXTURE_2D );
    }

    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(mSrcBlend, mDstBlend);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

//--------------------------------------------------------------------------
// PostRender: Cleanup OGL state
//--------------------------------------------------------------------------
void MotionTrail::postrender()
{
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//--------------------------------------------------------------------------
// Render Segment
//--------------------------------------------------------------------------
void MotionTrail::renderTrail(const int nCurrent,  const int nCount)
{
    if(nCount < 2)
        return;
    // Prepare spline patch for interpolation of start points
    CatmullRomPatch    mPatchStart;
    SplCtrlPts ctrlPointsStart;
    if(nCurrent < 1)
        ctrlPointsStart.addPoint(quads[0].start);
    else
        ctrlPointsStart.addPoint(quads[nCurrent-1].start);
    ctrlPointsStart.addPoint(quads[nCurrent].start);
    ctrlPointsStart.addPoint(quads[nCurrent+1].start);
    if(nCurrent+2 > nCount-1)
        ctrlPointsStart.addPoint(quads[nCount-1].start);
    else
        ctrlPointsStart.addPoint(quads[nCurrent+2].start);
    mPatchStart.submitControlPoints(ctrlPointsStart);

    // Prepare spline patch for interpolation of end points
    CatmullRomPatch    mPatchEnd;
    SplCtrlPts ctrlPointsEnd;
    if(nCurrent < 1)
        ctrlPointsEnd.addPoint(quads[0].end);
    else
        ctrlPointsEnd.addPoint(quads[nCurrent-1].end);
    ctrlPointsEnd.addPoint(quads[nCurrent].end);
    ctrlPointsEnd.addPoint(quads[nCurrent+1].end);
    if(nCurrent+2 > nCount-1)
        ctrlPointsEnd.addPoint(quads[nCount-1].end);
    else
        ctrlPointsEnd.addPoint(quads[nCurrent+2].end);
    mPatchEnd.submitControlPoints(ctrlPointsEnd);

    MotionTrailQuad quad1 = quads[nCurrent];
    MotionTrailQuad quad2 = quads[nCurrent+1];
    float    fPercentage1 = (float)nCurrent/(float)(nCount-1);
    float    fPercentage2 = (float)(nCurrent+1)/(float)(nCount-1);

    Point3F start, end;
    F32 texCoordX;

    F32 fDist = (quad1.start - quad2.start).len();
    F32 fInc = mDataBlock->curveDetail/fDist;

  glBegin(GL_QUADS);

  for(F32 f=0.0; f<1.0; f+=fInc)
  {
       // Clamp to 1.0f due to floating point errors
       F32    nextf = getMin(f+fInc, 1.0f);

        // Compute alpha for first side of quad
        quad1.color.alpha = interpolate(quad1.startAlpha, quad2.startAlpha, f);
        glColor4fv(quad1.color);

        // Compute texture coordinate for first side of quad
        texCoordX = interpolate(fPercentage1, fPercentage2, f);

        // Compute interpolated coordinate of first side (top)
        mPatchStart.calc(f, start);

        // First quad point
        glTexCoord2f(texCoordX, 0);
        glVertex3fv(start);

        // Compute interpolated coordinate of first side (bottom)
        mPatchEnd.calc(f, end);

        // Second quad point
        glTexCoord2f(texCoordX, 1);
        glVertex3fv(end);

        // Compute alpha for second side of quad
        quad2.color.alpha = interpolate(quad1.startAlpha, quad2.startAlpha, nextf);

        glColor4fv(quad2.color);

        // Compute texture coordinate for second side of quad
        texCoordX = interpolate(fPercentage1, fPercentage2, nextf);

        // Compute interpolated coordinate of second side (bottom)
        mPatchEnd.calc(nextf, end);

        // Third quad point
        glTexCoord2f(texCoordX, 1);
        glVertex3fv(end);

        // Compute interpolated coordinate of first side (top)
        mPatchStart.calc(nextf, start);

        // Forth quad point
        glTexCoord2f(texCoordX, 0);
        glVertex3fv(start);

  }

  glEnd();
}

//--------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------
void MotionTrail::update( F32 dt )
{

   // stop the segment array from growing indefinitely.
    U8 bQuit = 0;

   while((quads.size() > 0) && !bQuit)
   {
       if(quads.front().ElapsedTime > mDataBlock->lifetime )
       {
           // line segment is too old.. lets erase it.
           quads.erase(quads.begin());
       }
        else
            bQuit = 1;

   }

  U32 i;
  for( i=0; i<quads.size(); i++ )
  {
     quads[i].ElapsedTime += dt;
  }

  calcLineAlpha();
}

//--------------------------------------------------------------------------
// Calc alpha at each vertex
//--------------------------------------------------------------------------
void MotionTrail::calcLineAlpha()
{

  U32 i;
  for( i=0; i<quads.size(); i++ )
  {
     F32 percentDone;

     // make the very end clear so you can't see the poly edge
     if( i == 0 )
     {
        quads[i].startAlpha = 0.0;
     }
     else
     {
        percentDone = quads[i].ElapsedTime / mDataBlock->lifetime;
        quads[i].startAlpha = 1.0 - percentDone;
     }

  }

}