/-----------------------------------------------------------------------------
// Torque Game Engine
//
// Written by Markus Nuebel
//-----------------------------------------------------------------------------

#include "math/mCatmullRomPatch.h"


//******************************************************************************
// Quadratic spline patch
//******************************************************************************
CatmullRomPatch::CatmullRomPatch()
{
  setNumReqControlPoints(4);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CatmullRomPatch::calcTangentVectors()
{
    SplCtrlPts* points = (SplCtrlPts*) getControlPoints();
    if(points->getNumPoints() > 3)
    {
        tangent0 = *points->getPoint(2) - *points->getPoint(0);
        tangent1 = *points->getPoint(3) - *points->getPoint(1);
    }
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CatmullRomPatch::submitControlPoints( SplCtrlPts &points )
{
  Parent::submitControlPoints( points );
  calcTangentVectors();
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CatmullRomPatch::setControlPoint( Point3F &point, int index )
{
  ( (SplCtrlPts*) getControlPoints() )->setPoint( point, index );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CatmullRomPatch::calc( F32 t, Point3F &result )
{
    SplCtrlPts* points = (SplCtrlPts*) getControlPoints();

    F32 t2 = t*t;
  F32 t3 = t*t*t;

  F32 h0 = 2*t3-3*t2+1;
  F32 h1 = -2*t3+3*t2;
  F32 h2 = t3-2*t2+t;
  F32 h3 = t3-t2;

  result = h0*(*points->getPoint(1)) + h1*(*points->getPoint(2)) + h2*tangent0 + h3*tangent1;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CatmullRomPatch::calc( Point3F *points, F32 t, Point3F &result )
{
}

