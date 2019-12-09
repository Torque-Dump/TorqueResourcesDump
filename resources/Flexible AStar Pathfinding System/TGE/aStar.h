#define AI_RENDER

#ifndef _NAVMESH_H_
#define _NAVMESH_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

#ifndef _SIMPATH_H_
#include "sim/simPath.h"
#endif

class AStar;
class NavMesh;

struct AStarPoint;

struct NMPoint
{
	U32 id;
	F32 interval;
   Point3F loc; //in world space to prevent confusion
   NMPoint *adjs[8];
   NMPoint *prev, *next; //for linear linked-list behavior
   AStarPoint *asp;
   NavMesh* mesh;
};

struct NMIDPoint
{
	Point3F loc;
	F32 interval;
	U32 adjs[8];
};

struct AStarPoint
{
   NMPoint *pt;
   AStarPoint *parent;
   F32 f, g; //we don't need h except to get f
   enum{
      none = 0,
      open = 1,
      closed = 2
   };
   U8 list;
};

class NavMesh : public SceneObject
{
    friend class AStar;
private:
	typedef SceneObject		Parent;

protected:

	enum {	NavMeshMask		= (1 << 0),
			NavMeshAnother	= (1 << 1) };

    Box3F oldBox;
    F32 interval;
    S32 xSize;
    S32 ySize;
    F32 height;

public:
	NavMesh();
	~NavMesh();

    NMPoint* first;
    NMPoint* last;

	// SimObject      
	bool onAdd();
	void onRemove();
	void onEditorEnable();
	void onEditorDisable();
	void inspectPostApply();

	// NetObject
	U32 packUpdate(NetConnection *, U32, BitStream *);
	void unpackUpdate(NetConnection *, BitStream *);

	// ConObject.
	static void initPersistFields();
    static void consoleInit();

	// Declare Console Object.
	DECLARE_CONOBJECT(NavMesh);
};


class AStar
{
    NMPoint* list;
    AStarPoint* openList;
    S32 points;

    Vector<NavMesh*> meshes;

    Vector<NMPoint*> sortVec;

    void processMesh (NavMesh *mesh);

	void reLabel();

    NMPoint* findPoint (Point3F loc);
    void sortPoints(Point3F pt);

public:
    AStar();
    ~AStar() {};

    S32 findBasicPath (Point3F Pstart, Point3F Pend, Vector<Point3F> &Vpoints);
    S32 findCover (Point3F Pstart, Point3F Penemy, Vector<Point3F> &Vpoints);
    S32 sneakUp (Point3F Pstart, Point3F Penemy, VectorF eLook, F32 okDist, Vector<Point3F> &Vpoints);
    S32 wander (Point3F Pstart, U32 length, Vector<Point3F> &Vpoints);

    void addMesh (NavMesh *mesh);
    void removeMesh (NavMesh *mesh);
    void linkMeshes ();
	void deleteAll();

	void write();
	bool read();

#ifdef AI_RENDER
    void render(SceneState* state);
    bool rendered;
#endif

    F32 clearence;

    bool autoBuild;
};

extern AStar gAStar;

inline int findDirec(Point2F pt)
{
    pt.normalize();
    return (S32)(10.5f-mAtan(pt.y, pt.x)/M_PI_F*4)%8;
}

inline int findDirec(Point3F pt)
{
    pt.normalize();
    return (S32)(10.5f-mAtan(pt.y, pt.x)/M_PI_F*4)%8;
}

//---------------------------------------------------------//
//                 Binary Heap Inlines                     //
//---------------------------------------------------------//

inline void BHSwap(AStarPoint* P1, AStarPoint* P2)
{
    AStarPoint tmp;
    P1->pt->asp = P2;
    P2->pt->asp = P1;
    tmp = *P1;
    *P1 = *P2;
    *P2 = tmp;
}

inline void BHAdd(AStarPoint* openList, S32 olSize)
{
    U32 tmp = olSize;
    while (tmp)
    {
        if ((openList[tmp].f > openList[(tmp-1)/2].f))
            return;
        BHSwap(&openList[tmp], &openList[(tmp-1)/2]);
        tmp = (tmp-1)/2;
    }
    return;
}

inline void BHPop(AStarPoint* openList, S32 olSize)
{
    if (!olSize)
        return;

    for (int i = 0; i < olSize; ++i)
        ++(openList[i].pt->asp);

    AStarPoint cpy = openList[olSize-1];
    dMemmove(openList + 1, openList, (olSize-1)*sizeof(AStarPoint)); //shove over one
    openList[0] = cpy;
    openList[0].pt->asp = openList;
    U32 tmp = 0;
    while (tmp*2+1 < olSize)//(openList[tmp*2+1].f)
    {
        if (tmp*2+2 >= olSize)
        {
            if (openList[tmp].f > openList[tmp*2+1].f)
                BHSwap(&openList[tmp], &openList[tmp*2+1]);
            return; //that was the last one
        }

        if (openList[tmp].f > openList[tmp*2+1].f)
        {
            if (openList[tmp*2+1].f > openList[tmp*2+2].f) {
                BHSwap(&openList[tmp], &openList[tmp*2+2]); tmp = tmp*2+2; }
            else {
                BHSwap(&openList[tmp], &openList[tmp*2+1]); tmp = tmp*2+1; }
        }
        else if (openList[tmp].f > openList[tmp*2+2].f)
        {
            if (openList[tmp*2+2].f > openList[tmp*2+1].f) {
                BHSwap(&openList[tmp], &openList[tmp*2+1]); tmp = tmp*2+1; }
            else {
                BHSwap(&openList[tmp], &openList[tmp*2+2]); tmp = tmp*2+2; }
        }
        else
            return; //we're done
    }
    return;
}

#endif