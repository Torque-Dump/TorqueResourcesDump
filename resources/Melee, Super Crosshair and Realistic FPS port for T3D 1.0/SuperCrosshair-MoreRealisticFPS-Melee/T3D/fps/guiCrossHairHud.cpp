#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "platform/platform.h"
#include "gui/core/guiControl.h"
#include "gui/controls/guiBitmapCtrl.h"
#include "gui/3d/guiTSControl.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "T3D/gameConnection.h"
#include "T3D/shapeBase.h"
#include "terrain/terrData.h"
#include "interior/interior.h"
#include "gfx/gfxDrawUtil.h"
#define SMOOTH_FACTOR 10

class GuiCrossHairHud : public GuiBitmapCtrl
{
typedef GuiBitmapCtrl Parent;

ColorF mDamageFillColor;
ColorF mDamageFrameColor;
Point2I mDamageRectSize;
Point2I mDamageOffset;

Point2I mBitmapSize;
ColorF mFriendlyTargetColor;
ColorF mEnemyTargetColor;
ColorF mNeutralTargetColor;

ColorF mActiveColor;

Point2I mSmooth[SMOOTH_FACTOR];


// RectI origBounds;

protected:
void drawDamage(Point2I offset, F32 damage, F32 opacity);
void shuntSmoothArray();
Point2I avgSmoothArray();
void drawName( Point2I offset, const char *buf, F32 opacity);

public:
GuiCrossHairHud();

void onRender( Point2I, const RectI &);
static void initPersistFields();
DECLARE_CONOBJECT( GuiCrossHairHud );
   DECLARE_CATEGORY( "Gui Game" );
};

/// Valid object types for which the cross hair will render, this
/// should really all be script controlled.
static const U32 ObjectMask = PlayerObjectType | VehicleObjectType;


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT( GuiCrossHairHud );

GuiCrossHairHud::GuiCrossHairHud()
{
U32 i;
   mDamageFillColor.set( 0.0f, 1.0f, 0.0f, 1.0f );
mDamageFrameColor.set( 0.0f, 1.0f, 0.0f, 1.0f );
   mDamageRectSize.set(50, 4);
   mDamageOffset.set(0,32);


mBitmapSize.set(32,32);
mFriendlyTargetColor.set(0.0f,1.0f,0.0f,0.0f);
mEnemyTargetColor.set(1.0f,0.0f,0.0f,1.0f);
mNeutralTargetColor.set(1.0f,1.0f,1.0f,1.0f);

	for(i=0;i<SMOOTH_FACTOR;i++)
	{
		mSmooth[i] = Point2I(0,0);
	}
}

void GuiCrossHairHud::initPersistFields()
{
   addGroup("Damage");		
addField( "damageFillColor", TypeColorF, Offset( mDamageFillColor, GuiCrossHairHud ) );
addField( "damageFrameColor", TypeColorF, Offset( mDamageFrameColor, GuiCrossHairHud ) );
addField( "damageRect", TypePoint2I, Offset( mDamageRectSize, GuiCrossHairHud ) );
addField( "damageOffset", TypePoint2I, Offset( mDamageOffset, GuiCrossHairHud ) );

addField( "bitmapSize", TypePoint2I, Offset( mBitmapSize, GuiCrossHairHud ) );
addField( "friendlyTargetColor", TypeColorF, Offset( mFriendlyTargetColor, GuiCrossHairHud ) );
addField( "enemyTargetColor", TypeColorF, Offset( mEnemyTargetColor, GuiCrossHairHud ) );
addField( "neutralTargetColor", TypeColorF, Offset( mNeutralTargetColor, GuiCrossHairHud ) );
   endGroup("Damage");
   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void GuiCrossHairHud::onRender(Point2I offset, const RectI &updateRect)
{
// Must have a connection and player control object
GameConnection* conn = GameConnection::getConnectionToServer();
if (!conn)
return;
   ShapeBase* control = dynamic_cast<ShapeBase*>(conn->getControlObject());
if (!control || !(control->getType() & ObjectMask))
	return;

// MUZZLE RAYCAST
MatrixF gun;
Point3F gunPos,gunVec;
//conn->getControlObject()->getMuzzleTransform(0,&gun);
//control->getControlObject()->getMuzzleTransform(0,&gun);
control->getMuzzleTransform(0,&gun);
gun.getColumn(3, &gunPos);

gun.getColumn(1, &gunVec);
Point3F gunEndPos = gunVec;
gunEndPos *= gClientSceneGraph->getVisibleDistance();
gunEndPos += gunPos;

static U32 losMask = TerrainObjectType | InteriorObjectType | ShapeBaseObjectType;
control->disableCollision(); // disable collisions with control object

RayInfo gunRay;
gunRay.object = 0;
gClientContainer.castRay(gunPos, gunEndPos, losMask, &gunRay);

control->enableCollision(); // we are done with the raycasting, so we can do this now

Point3F targetPoint = gunPos;
bool hasObject = false;
F32 dmgLevel = 1;
char* szShapeName = new char[24]; //StringTableEntry



if(gunRay.object == 0)
targetPoint = gunPos + (gunVec * (gClientSceneGraph->getVisibleDistance() / 2));
else
{
if (ShapeBase* obj = dynamic_cast<ShapeBase*>(gunRay.object))
{
if(obj->getShapeName())
{
hasObject = true;
dmgLevel = obj->getDamageValue();

dStrcpy(szShapeName, obj->getShapeName());

mActiveColor = mEnemyTargetColor;
}
}
targetPoint = gunRay.point;
}
Point3F projPnt(320.0f,320.0f,0.0f);

// setup TSCCtrl as parent, for doing the ->project
GuiTSCtrl *parent = dynamic_cast<GuiTSCtrl*>(getParent());
if (!parent) return;

if (!parent->project(targetPoint, &projPnt))
{
return;
}
if(bool(mTextureObject))
{
 GFXStateBlockDesc desc;  
   desc.ffLighting = false;  
   desc.setCullMode( GFXCullNone );  
   desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);  
   desc.samplers[0].textureColorOp = GFXTOPModulate;  
   desc.samplers[1].textureColorOp = GFXTOPDisable;  
  
   GFXStateBlockRef myState = GFX->createStateBlock(desc);  
  
   // Render time code  
   GFX->setStateBlock(myState);  
  
	GFX->setTexture(0, mTextureObject );


//TextureObject* texture = (TextureObject*)mTextureHandle;
GFXTextureObject* texture = (GFXTextureObject*)mTextureObject;


shuntSmoothArray();
mSmooth[SMOOTH_FACTOR-1] = Point2I( projPnt.x - (mBitmapSize.x / 2) , projPnt.y - (mBitmapSize.y / 2));

//Point2I aimOffset = Point2I( projPnt.x - (mBitmapSize.x / 2) , projPnt.y - (mBitmapSize.y / 2) );

Point2I aimOffset = avgSmoothArray();

RectI rect(aimOffset,Point2I(mBitmapSize.x * 2,mBitmapSize.y * 2));

//glBegin(GL_TRIANGLE_FAN);
PrimBuild::begin(GFXTriangleFan,4);

//glTexCoord2f(0, 1);
PrimBuild::texCoord2f(0, 1);
//glVertex2f(rect.point.x, rect.point.y + rect.extent.y);
PrimBuild::vertex2f(rect.point.x, rect.point.y + rect.extent.y);


//glTexCoord2f(1, 1);
PrimBuild::texCoord2f(1, 1);
//glVertex2f(rect.point.x + rect.extent.x, rect.point.y + rect.extent.y);
PrimBuild::vertex2f(rect.point.x + rect.extent.x, rect.point.y + rect.extent.y);
//glTexCoord2f(1, 0);
PrimBuild::texCoord2f(1, 0);
//glVertex2f(rect.point.x + rect.extent.x, rect.point.y);
PrimBuild::vertex2f(rect.point.x + rect.extent.x, rect.point.y);
//glTexCoord2f(0, 0);
PrimBuild::texCoord2f(0, 0);
//glVertex2f(rect.point.x, rect.point.y);
PrimBuild::vertex2f(rect.point.x, rect.point.y);
//glEnd();
PrimBuild::end();
//glDisable(GL_BLEND);
//glDisable(GL_TEXTURE_2D);
}
else
{
Point2I aimOffset = Point2I( projPnt.x - 2,
projPnt.y - 2 );

RectI rect(aimOffset, Point2I(4,4));

//dglDrawRectFill(rect, ColorF(1.0f,1.0f,1.0f,1.0f));
 GFX->getDrawUtil()->drawRect(rect, ColorF(1.0f,1.0f,1.0f,1.0f));
}

//renderChildControls(offset, updateRect);

// if we have an object, render the hud part
if (hasObject)
{
drawDamage(Point2I(projPnt.x,projPnt.y + mBitmapSize.y) + mDamageOffset,dmgLevel , 1);
drawName(Point2I(projPnt.x,projPnt.y + mBitmapSize.y) + mDamageOffset,szShapeName,1);
}
}


//-----------------------------------------------------------------------------
/**
Display a damage bar ubove the shape.
This is a support funtion, called by onRender.
*/
void GuiCrossHairHud::drawDamage(Point2I offset, F32 damage, F32 opacity)
{
mActiveColor.alpha = mDamageFrameColor.alpha = opacity;

// Damage should be 0->1 (0 being no damage,or healthy), but
// we'll just make sure here as we flip it.
damage = mClampF(1 - damage, 0, 1);

// Center the bar
RectI rect(offset, mDamageRectSize);
rect.point.x -= mDamageRectSize.x / 2;

// Draw the border
//dglDrawRect(rect, mDamageFrameColor);
 GFX->getDrawUtil()->drawRect(rect, mDamageFrameColor);

// Draw the damage % fill
rect.point += Point2I(1, 1);
rect.extent -= Point2I(1, 1);
rect.extent.x = (S32)(rect.extent.x * damage);
if (rect.extent.x == 1)
rect.extent.x = 2;
if (rect.extent.x > 0)
//dglDrawRectFill(rect, mActiveColor);
GFX->getDrawUtil()->drawRectFill(rect, mActiveColor);
}

//------------------------------------------------------------------------------
void GuiCrossHairHud::shuntSmoothArray()
{
U8 i;

for (i=1;i<SMOOTH_FACTOR;i++)
{
mSmooth[i-1] = mSmooth[i];
}
}


//------------------------------------------------------------------------------
Point2I GuiCrossHairHud::avgSmoothArray()
{
U8 i;
Point2I average(0,0);

for (i=0;i<SMOOTH_FACTOR;i++)
{
average.x += mSmooth[i].x;
average.y += mSmooth[i].y;
}
average.x /= SMOOTH_FACTOR;
average.y /= SMOOTH_FACTOR;
return average;

}

//----------------------------------------------------------------------------
/// Render object names.
///
/// Helper function for GuiShapeNameHud::onRender
///
/// @param offset Screen coordinates to render name label. (Text is centered
/// horizontally about this location, with bottom of text at
/// specified y position.)
/// @param name String name to display.
/// @param opacity Opacity of name (a fraction).
void GuiCrossHairHud::drawName(Point2I offset, const char *name, F32 opacity)
{
// Center the name
offset.x -= mProfile->mFont->getStrWidth(name) / 2;
offset.y -= mProfile->mFont->getHeight();

// Deal with opacity and draw.
mActiveColor.alpha = opacity;
//dglSetBitmapModulation(mActiveColor);
GFX->getDrawUtil()->setBitmapModulation(mActiveColor);
//dglDrawText(mProfile->mFont, offset, name);
GFX->getDrawUtil()->drawText(mProfile->mFont, offset, name);
//dglClearBitmapModulation();
GFX->getDrawUtil()->clearBitmapModulation();
}