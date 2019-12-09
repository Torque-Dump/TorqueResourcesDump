//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Written by Markus Nuebel
//-----------------------------------------------------------------------------

#ifndef _CATMULROMPATCH_H_
#define _CATMULROMPATCH_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MSPLINEPATCH_H_
#include "math/mSplinePatch.h"
#endif


//------------------------------------------------------------------------------
// Catmull-Rom spline patch
//------------------------------------------------------------------------------
class CatmullRomPatch : public SplinePatch
{
  typedef SplinePatch Parent;

private:
    Point3F    tangent0;
    Point3F    tangent1;

    void calcTangentVectors();

public:

  CatmullRomPatch();

  virtual void   calc( F32 t, Point3F &result );
  virtual void   calc( Point3F *points, F32 t, Point3F &result );
  virtual void   setControlPoint( Point3F &point, int index );
  virtual void   submitControlPoints( SplCtrlPts &points );
  
};



#endif