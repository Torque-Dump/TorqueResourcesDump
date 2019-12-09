#include "game/aStar.h"

#include "dgl/dgl.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mathIO.h"
#include "game/gameConnection.h"
#include "console/simBase.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sgUtil.h"

extern bool gEditingMission;

inline F32 disSqr(Point3F a, Point3F b)
{
    return (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z);
}

#define LOOK_DIS_SQR 225

AStar gAStar;

AStar::AStar()
{
    list = 0;
    points = 0;
    openList = 0;
    clearence = 0.4f;
#ifdef AI_RENDER
    rendered = true;
#endif
}

//---------------------------------------------------------//
//                   Misc NavMesh Stuff                    //
//---------------------------------------------------------//

IMPLEMENT_CO_NETOBJECT_V1(NavMesh);

NavMesh::NavMesh()
{
   mTypeMask |= StaticObjectType;
   mNetFlags.set(Ghostable);
   interval = 3;
   xSize = 2;
   ySize = 2;
   height = 8;
   first = last = 0;
}

NavMesh::~NavMesh()
{
}

void NavMesh::consoleInit()
{
#ifdef AI_RENDER
    Con::addVariable("$pref::AStar::Render", TypeBool, &gAStar.rendered);
#endif
    Con::addVariable("$pref::AStar::Clearence", TypeF32, &gAStar.clearence);
    Con::addVariable("$pref::AStar::AutoBuild", TypeBool, &gAStar.autoBuild);
}

void NavMesh::initPersistFields()
{
	Parent::initPersistFields();
    addField( "Interval",		TypeF32,		Offset( interval,			NavMesh ) );
    addField( "XSize",	    	TypeS32,		Offset( xSize,  			NavMesh ) );
    addField( "YSize",	    	TypeS32,		Offset( ySize,  			NavMesh ) );
    addField( "Height",	    	TypeF32,		Offset( height, 			NavMesh ) );
}

bool NavMesh::onAdd()
{
	if(!Parent::onAdd()) return(false);
    F32 x = (interval*((F32)xSize-1))/2;
    F32 y = (interval*((F32)ySize-1))/2;
	mObjBox.min.set( -x, -y, -(height/2));
	mObjBox.max.set(  x, y, height/2);
	resetWorldBox();
	setRenderTransform(mObjToWorld);
	addToScene();
    oldBox = mWorldBox;
    if (isServerObject())
        gAStar.addMesh(this);
	return(true);
}

void NavMesh::onRemove()
{
	removeFromScene();
	Parent::onRemove();
    if (isServerObject() && gEditingMission)
        gAStar.removeMesh(this);
}

void NavMesh::inspectPostApply()
{
	Parent::inspectPostApply();
	setMaskBits(NavMeshMask);
    if (oldBox.min != mWorldBox.min || oldBox.max != mWorldBox.max)
    {
        oldBox = mWorldBox;
        gAStar.addMesh(this);

        if (gAStar.autoBuild)
		   gAStar.linkMeshes();
    }
}

void NavMesh::onEditorEnable()
{
}

void NavMesh::onEditorDisable()
{
}

U32 NavMesh::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
	U32 retMask = Parent::packUpdate(con, mask, stream);
	if (stream->writeFlag(mask & NavMeshMask))
	{
		stream->writeAffineTransform(mObjToWorld);
		stream->write(interval);
        stream->write(xSize);
        stream->write(ySize);
        stream->write(height);

        F32 x = (interval*((F32)xSize-1))/2;
        F32 y = (interval*((F32)ySize-1))/2;
	    mObjBox.min.set( -x, -y, -(height/2));
	    mObjBox.max.set(  x, y, height/2);
	    resetWorldBox();
	    setRenderTransform(mObjToWorld);
	}
	return(retMask);
}

void NavMesh::unpackUpdate(NetConnection * con, BitStream * stream)
{
	Parent::unpackUpdate(con, stream);

	if(stream->readFlag())
	{
		MatrixF		ObjectMatrix;
		stream->readAffineTransform(&ObjectMatrix);
		stream->read(&interval);
		stream->read(&xSize);
		stream->read(&ySize);
        stream->read(&height);
		setTransform(ObjectMatrix);

        F32 x = (interval*((F32)xSize-1))/2;
        F32 y = (interval*((F32)ySize-1))/2;
	    mObjBox.min.set( -x, -y, -(height/2));
	    mObjBox.max.set(  x, y, height/2);
	    resetWorldBox();
	    setRenderTransform(mObjToWorld);
	}
}

//---------------------------------------------------------//
//                  Mesh/Point Management                  //
//---------------------------------------------------------//

void AStar::removeMesh (NavMesh *mesh)
{
    if (meshes.empty())
        return; //wtflol?

    if (!mesh->first) //sanity check
        return;

    S32 i;
    for (i = 0; i < meshes.size(); ++i)
    {
        if (meshes[i] == mesh)
        {
            meshes.erase(i);
            break;
        }
    }
            
    if (mesh->first == list && !mesh->last->next) //only in list
        list = 0;
    else if (mesh->first == list) //first in list
        list = mesh->last->next;
    else //also works for last b/c mesh->last->next == 0
        mesh->first->prev->next = mesh->last->next; //lol

    if (mesh->last->next)
        mesh->last->next->prev = mesh->first->prev;

    NMPoint * delpt = mesh->first;
    NMPoint * delnext;

    while (1)
    {
        delnext = delpt->next;
        delete delpt;
        --points;
        if (delpt == mesh->last)
            break;
        delpt = delnext;
    }
    mesh->first = mesh->last = 0;

    delete [] openList;
    openList = new AStarPoint[points];
    dMemset(openList, 0, sizeof(AStarPoint)*points);

	 reLabel();
    linkMeshes();
}

void AStar::addMesh (NavMesh *mesh)
{
    processMesh(mesh);
	 reLabel();
    //linkMeshes();
}

void AStar::processMesh (NavMesh *mesh)
{
    NavMesh* itr;

    //cleanup
    if (!meshes.empty()) {
    for (S32 i = 0; i < meshes.size(); ++i)
    {
        itr = meshes[i];
        if (itr == mesh)
        {
            meshes.erase(i);

            if (!mesh->first) //sanity check
                continue;
            
            if (mesh->first == list && !mesh->last->next) //only in list
                list = 0;
            else if (mesh->first == list) //first in list
                list = mesh->last->next;
            else //also works for last b/c mesh->last->next == 0
                mesh->first->prev->next = mesh->last->next; //lol

            if (mesh->last->next)
                mesh->last->next->prev = mesh->first->prev;

            NMPoint * delpt = mesh->first;
            NMPoint * delnext;
            while (1)
            {
                delnext = delpt->next;
                delete delpt;
                --points;
                if (delpt == mesh->last)
                    break;
                delpt = delnext;
            }
            mesh->first = mesh->last = 0;
        }
    }
    }
    meshes.push_front(mesh);

    //add points
    Point3F wpt;
    NMPoint * newPt = 0;
    RayInfo rInfo;

    bool first = true;

    for (S32 j = 0; j < mesh->ySize; j++)
    {
        for (S32 i = 0; i < mesh->xSize; i++)
        {
            wpt.set(((F32)i - ((F32)mesh->xSize-1)/2)*mesh->interval, ((F32)j - ((F32)mesh->ySize-1)/2)*mesh->interval, mesh->height/2);
            mesh->mObjToWorld.mulP(wpt);
            if (!gServerContainer.castRay(wpt, Point3F(wpt.x, wpt.y, wpt.z - mesh->height), STATIC_COLLISION_MASK, &rInfo))
                continue;
            if (gServerContainer.castRay(rInfo.point + Point3F(0, 0, 2), rInfo.point + Point3F(0, 0, 0.01f), STATIC_COLLISION_MASK, &rInfo))
                continue;
            ++points;
            wpt = rInfo.point;
            wpt += Point3F(0, 0, 1); //move it off the ground a bit
            newPt = new NMPoint();
            dMemset(newPt, 0, sizeof(*newPt)); //friggin contructor doesnt work right
            newPt->loc = wpt;
            newPt->mesh = mesh;
			newPt->interval = mesh->interval;
            if (list)
                list->prev = newPt;
            if (first) {
                mesh->last = newPt; first = false; } //b/c they're added in reverse order
            newPt->next = list;
            list = newPt;
        }
    }
    mesh->first = newPt;

    delete [] openList;
    openList = new AStarPoint[points];
    dMemset(openList, 0, sizeof(AStarPoint)*points);
}

ConsoleFunction(BuildPaths, void, 1, 1, "()")
{
    gAStar.linkMeshes();
}

void AStar::linkMeshes ()
{
    NMPoint* cur;
    NMPoint* tmp;
    Point2F tmpVec;
    int direc;
    RayInfo rInfo;
    Point3F yVec;
    Point3F offset;

    int nodes = 0;
    
    sortVec.clear();

    const S32 time = Platform::getRealMilliseconds();

    for (cur = list; cur; cur = cur->next)
        cur->adjs[0] = cur->adjs[1] = cur->adjs[2] = cur->adjs[3] = cur->adjs[4] = cur->adjs[5] = cur->adjs[6] = cur->adjs[7] = 0;

    for (cur = list; cur; cur = cur->next)
    {
        nodes++;

        sortVec.push_back(cur); //might as well do it here

        for (tmp = list; tmp; tmp = tmp->next) //iterate thru list
        {
            if (tmp == cur)
                continue;

            tmpVec.set(cur->loc.x-tmp->loc.x, cur->loc.y-tmp->loc.y);
				if (cur->mesh)
					cur->mesh->mObjToWorld.getColumn(1, &yVec);
				else
					yVec.set(0, 1, 0);
            direc = ( findDirec(tmpVec) + (findDirec(yVec)&1) ) % 8;

            if (cur->adjs[direc])
                continue;

            if (tmp->loc.z - cur->loc.z < tmp->interval && tmpVec.len() < cur->interval*(direc&1 ? 1.5f : 1.1f))
            {
                offset.set(cur->loc.y - tmp->loc.y, tmp->loc.x - cur->loc.x, 0);
                offset.normalize(clearence);
                if (!gServerContainer.castRay(cur->loc + offset, tmp->loc + offset, STATIC_COLLISION_MASK, &rInfo) &&
                    !gServerContainer.castRay(cur->loc - offset, tmp->loc - offset, STATIC_COLLISION_MASK, &rInfo))
                    cur->adjs[direc] = tmp;
            }

            if (cur->adjs[0] && cur->adjs[1] && cur->adjs[2] && cur->adjs[3] &&
                cur->adjs[4] && cur->adjs[5] && cur->adjs[6] && cur->adjs[7])
                break;
        }
    }

    Con::printf("Built paths in %d ms; %d nodes.", Platform::getRealMilliseconds()-time, nodes);
}

void AStar::reLabel()
{
	int i = 0;
	for(NMPoint* pt = list; pt; pt = pt->next)
	{
		pt->id = i;
		i++;
	}
}

ConsoleFunction(deletePaths, void, 1, 1, "delete navmesh")
{
	gAStar.deleteAll();
}

void AStar::deleteAll()
{
	if (!list)
		return;

	while(meshes.size())
	{
		meshes[0]->deleteObject();
		meshes.pop_front();
	}

	NMPoint* pt;
	for (pt = list; pt->next;)
	{
		pt = pt->next;
		delete pt->prev;
	}
	delete pt;

	list = 0;
	points = 0;
}

//seems like a silly way to do this...
Point3F qsLoc;
int qsLocCmp(const void* a, const void* b)
{
   F32 ad = disSqr((*(NMPoint**)a)->loc, qsLoc); //jeebus!
   F32 bd = disSqr((*(NMPoint**)b)->loc, qsLoc);
   return ad < bd ? -1 : (ad == bd ? 0 : 1);
}

void AStar::sortPoints(Point3F pt)
{
   if ((qsLoc - pt).len() < clearence)
      return; //no need to sort
   qsLoc = pt;
   dQsort((void*)&sortVec[0], sortVec.size(), sizeof(NMPoint*), qsLocCmp);
}

NMPoint* AStar::findPoint (Point3F pt)
{
   RayInfo rInfo;
   bool found = false;
   NMPoint* nmp = list;
   for (NMPoint* tmp = list; tmp; tmp = tmp->next) //iterate thru list
   {
      if(tmp->loc.z - pt.z < 3 && disSqr(tmp->loc, pt) < LOOK_DIS_SQR &&
         disSqr(tmp->loc, pt) < disSqr(nmp->loc, pt) &&
         !gServerContainer.castRay(tmp->loc, pt, ( InteriorObjectType | StaticTSObjectType | StaticShapeObjectType  ), &rInfo)) //find closest one
      {
         nmp = tmp;
         found = true;
      }
   }
   if (found)
      return nmp;

   //didn't find anything, will do prioritized brute force now

   sortPoints(pt);

   for (int i=0; i<sortVec.size(); i++)
      if (disSqr(sortVec[i]->loc, pt) > LOOK_DIS_SQR && !gServerContainer.castRay(sortVec[i]->loc, pt, ( InteriorObjectType | StaticTSObjectType | StaticShapeObjectType  ), &rInfo))
         return sortVec[i];

   return 0; //give up
}

//---------------------------------------------------------//
//                          I/O                            //
//---------------------------------------------------------//
ConsoleFunction(writePaths, void, 1, 1, "writes navmesh to file")
{
	gAStar.write();
	gAStar.read();
}

ConsoleFunction(readPaths, bool, 1, 1, "reads navmesh from file")
{
	return gAStar.read();
}

void AStar::write()
{
	if (!list)
		return;

	char misName[256];
	dSprintf(misName, sizeof(misName), "%s", Con::getVariable("$Server::MissionFile"));
	char * dot = dStrstr((const char*)misName, ".mis");
	if(dot)
		dSprintf(dot, dot-misName, "%s", ".as");

	FileStream fs;
   if (!fs.open(misName, FileStream::Write))
		return;

	NMIDPoint * data = new NMIDPoint[points];

	dMemset(data, 0, sizeof(NMIDPoint)*points);

	int i = 0;
	for(NMPoint* pt = list; pt; pt = pt->next)
	{
		data[i].loc = pt->loc;
		data[i].interval = pt->interval;
		for (int j=0; j<8; j++)
			data[i].adjs[j] = pt->adjs[j] ? pt->adjs[j]->id + 1: 0;
		++i;
	}

	fs.write(points);

	fs.write(sizeof(NMIDPoint)*points, data);

	fs.close();

	delete [] data;

	Con::printf("Save the mission now.");
}

bool AStar::read()
{
	char misName[256];
	dSprintf(misName, sizeof(misName), "%s", Con::getVariable("$Server::MissionFile"));
	char * dot = dStrstr((const char*)misName, ".mis");
	if(dot)
		dSprintf(dot, dot-misName, "%s", ".as");

	FileStream fs;
	if (!fs.open(misName, FileStream::Read))
		return false;

	deleteAll();

	fs.read(&points);
	NMIDPoint * data = new NMIDPoint[points];

	fs.read(sizeof(NMIDPoint)*points, data);

	fs.close();

	if (!points)
		return false;

    //allocate in a block so it's faster
	NMPoint* tmpList = new NMPoint[points];

	list = tmpList;

    sortVec.clear();

	for (int i=0; i<points; i++)
	{
	    dMemset(&tmpList[i], 0, sizeof(NMPoint));

        sortVec.push_back(&tmpList[i]);

		tmpList[i].loc = data[i].loc;
		tmpList[i].id = i;
		tmpList[i].mesh = 0;
		tmpList[i].interval = data[i].interval;

		tmpList[i].next = &tmpList[i+1];
		tmpList[i].prev = &tmpList[i-1];
	}
    tmpList[0].prev = 0;
    tmpList[points-1].next = 0;
	
	for (int i=0; i<points; i++)
		for (int j=0; j<8; j++)
			tmpList[i].adjs[j] = data[i].adjs[j] ? &tmpList[data[i].adjs[j]-1] : 0;

   delete [] data;

   delete [] openList;
   openList = new AStarPoint[points];
   dMemset(openList, 0, sizeof(AStarPoint)*points);

	return true;
}

//---------------------------------------------------------//
//                      Pathfinding                        //
//---------------------------------------------------------//

S32 AStar::findBasicPath (Point3F Pstart, Point3F Pend, Vector<Point3F> &Vpoints)
{
    if (!list)
        return -1;

    if (Pstart == Pend) //may seem stupid, but probably good to do
        return 1;

    NMPoint *start, *end, *tmp;
    start = end = list;
    RayInfo rInfo;
    
    if (!(end = findPoint(Pend)) || !(start = findPoint(Pstart)))
       return -1;

    if (start == end)
    {
        return 1;
    }

    //ok, we've got start and end NMPoints, time for some a*

    S32 olSize = 1;
    AStarPoint *curpt = openList;
    curpt->pt = start;
    start->asp = curpt;

    curpt->f = (start->loc - end->loc).len();

    int i;

    do {
        BHPop(curpt+1, olSize-1);
        curpt->list = AStarPoint::closed;
        for (i=0;i<8;++i)
        {
            if (!curpt->pt->adjs[i]) //no point
                continue;
            if (!curpt->pt->adjs[i]->asp) //a new point
            {
                if (curpt->pt->adjs[i] == end) //woohoo!
                    goto found;
                curpt[olSize].pt = curpt->pt->adjs[i];
                curpt[olSize].pt->asp = &curpt[olSize];
                curpt[olSize].parent = curpt;
                curpt[olSize].list = AStarPoint::open;
                curpt[olSize].g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt[olSize].f = curpt[olSize].g + (curpt[olSize].pt->loc - end->loc).len();
                BHAdd(curpt+1, olSize-1); //b/c curpt is the one *before* the first element
                olSize++;
            }
            else if ((curpt->pt->adjs[i]->asp->list == AStarPoint::open) &&
                     ((curpt->g + curpt->pt->interval*((i&1)?1.4f:1)) < curpt->pt->adjs[i]->asp->g))  //this is a better path
            {
                curpt->pt->adjs[i]->asp->parent = curpt;
                curpt->pt->adjs[i]->asp->g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt->pt->adjs[i]->asp->f = curpt->pt->adjs[i]->asp->g +
                                             (curpt->pt->adjs[i]->loc - end->loc).len();
                BHAdd(curpt+1, curpt->pt->adjs[i]->asp - curpt - 1);
            }
        }
        ++curpt;
        --olSize;
     } while (olSize);

     for (AStarPoint *del = openList; del->pt; del++)
     {
        del->pt->asp = 0;
     }

     dMemset(openList, 0, sizeof(AStarPoint)*points);

     return -1;

found:

    Vpoints.clear();

    Vpoints.push_front(end->loc);


    while (curpt->pt != start)
    {
        Vpoints.push_front(curpt->pt->loc);
        curpt = curpt->parent;
    } 

    if (mAbs(findDirec(Vpoints[0] - start->loc) - findDirec(Pstart - start->loc)) > 2)
        Vpoints.push_front(start->loc);

	Vpoints.push_front(Pstart);

    for (AStarPoint *del = openList; del->pt; del++)
    {
       del->pt->asp = 0;
    }

    dMemset(openList, 0, sizeof(AStarPoint)*points);

    return 0;
}

S32 AStar::findCover (Point3F Pstart, Point3F Penemy, Vector<Point3F> &Vpoints)
{
    Point3F offset;
    offset.set(Pstart.y - Penemy.y, Penemy.x - Pstart.x, 0);
    offset.normalize(clearence);
    offset.set(offset.z, offset.y, 0.5f);
    Penemy += Point3F(0, 0, 0.5f);
    RayInfo rInfo;
    if (gServerContainer.castRay(Pstart + offset, Penemy, STATIC_COLLISION_MASK, &rInfo) &&
        findDirec(rInfo.normal) == findDirec(Pstart - Penemy) &&
        gServerContainer.castRay(Pstart - offset, Penemy, STATIC_COLLISION_MASK, &rInfo) &&
        findDirec(rInfo.normal) == findDirec(Pstart - Penemy)) //home free
        return 1;

    if (!list)
        return -1;

    NMPoint *start, *end, *tmp;
    start = list;
    
    if (!(start = findPoint(Pstart)))
       return -1;

    S32 olSize = 1;
    AStarPoint *curpt = openList;
    curpt->pt = start;
    start->asp = curpt;

    int i;

    do {
        BHPop(curpt+1, olSize-1);
        curpt->list = AStarPoint::closed;
        for (i=0;i<8;++i)
        {
            if (!curpt->pt->adjs[i]) //no point
                continue;
            if (!curpt->pt->adjs[i]->asp) //a new point
            {
                offset.set(curpt->pt->adjs[i]->loc.y - Penemy.y, Penemy.x - curpt->pt->adjs[i]->loc.x, 0);
                offset.normalize(clearence);
                offset.set(offset.z, offset.y, 0.5f);
                if (gServerContainer.castRay(curpt->pt->adjs[i]->loc + offset, Penemy, STATIC_COLLISION_MASK, &rInfo) &&
                    findDirec(rInfo.normal) == findDirec(curpt->pt->adjs[i]->loc - Penemy) &&
                    gServerContainer.castRay(curpt->pt->adjs[i]->loc - offset, Penemy, STATIC_COLLISION_MASK, &rInfo) &&
                    findDirec(rInfo.normal) == findDirec(curpt->pt->adjs[i]->loc - Penemy)) //woohoo!
                {
                    end = curpt->pt->adjs[i];
                    goto found;
                }
                curpt[olSize].pt = curpt->pt->adjs[i];
                curpt[olSize].pt->asp = &curpt[olSize];
                curpt[olSize].parent = curpt;
                curpt[olSize].list = AStarPoint::open;
                curpt[olSize].g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt[olSize].f = curpt[olSize].g - (curpt[olSize].pt->loc - Penemy).len();
                BHAdd(curpt+1, olSize-1); //b/c curpt is the one *before* the first element
                olSize++;
            }
            else if ((curpt->pt->adjs[i]->asp->list == AStarPoint::open) &&
                     ((curpt->g + curpt->pt->interval*((i&1)?1.4f:1)) < curpt->pt->adjs[i]->asp->g))  //this is a better path
            {
                curpt->pt->adjs[i]->asp->parent = curpt;
                curpt->pt->adjs[i]->asp->g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt->pt->adjs[i]->asp->f = curpt->pt->adjs[i]->asp->g - (curpt->pt->adjs[i]->loc - Penemy).len();
                BHAdd(curpt+1, curpt->pt->adjs[i]->asp - curpt - 1);
            }
        }
        ++curpt;
        --olSize;
     } while (olSize);

     for (AStarPoint *del = openList; del->pt; del++)
     {
        del->pt->asp = 0;
     }

     dMemset(openList, 0, sizeof(AStarPoint)*points);

     return -1;

found:

    Vpoints.clear();

    Vpoints.push_front(end->loc);

    while (curpt->pt != start)
    {
        Vpoints.push_front(curpt->pt->loc);
        curpt = curpt->parent;
    } 

    if (mAbs(findDirec(Vpoints[0] - start->loc) - findDirec(Pstart - start->loc)) > 2)
        Vpoints.push_front(start->loc);

	Vpoints.push_front(Pstart);

    for (AStarPoint *del = openList; del->pt; del++)
    {
       del->pt->asp = 0;
    }

    dMemset(openList, 0, sizeof(AStarPoint)*points);

    return 0;
}

S32 AStar::sneakUp (Point3F Pstart, Point3F Penemy, VectorF Vlook, F32 okDist, Vector<Point3F> &Vpoints)
{
    if (!list)
        return -1;

    Penemy += Point3F(0,0,.5);
    Pstart += Point3F(0,0,.5);

    if (Pstart == Penemy) //may seem stupid, but probably good to do
        return 1;

    NMPoint *start, *end, *tmp;
    start = end = list;
    RayInfo rInfo;
    
    VectorF vToPt(0,0,0);

    bool found = false;
    for (NMPoint* tmp = list; tmp; tmp = tmp->next) //iterate thru list
    {
       vToPt = (tmp->loc - Penemy);
       vToPt.normalize(); //vv because we can't go up
       if(tmp->loc.z - Penemy.z < 3 && mDot(Vlook, vToPt) < .8 && disSqr(tmp->loc, Penemy) < LOOK_DIS_SQR &&
          disSqr(tmp->loc, Penemy) < disSqr(end->loc, Penemy) &&
          !gServerContainer.castRay(tmp->loc, Penemy, ( InteriorObjectType | StaticTSObjectType | StaticShapeObjectType  ), &rInfo)) //find closest one
       {
          end = tmp;
          found = true;
       }
    }

    if (!found)
    {
        //didn't find anything, will do prioritized brute force now
        sortPoints(Penemy);

        for (int i=0; i<sortVec.size(); i++)
        {
            vToPt = (sortVec[i]->loc - Penemy);
            vToPt.normalize();
            if (sortVec[i]->loc.z - Penemy.z < 3 && mDot(Vlook, vToPt) < .8 &&
                !gServerContainer.castRay(sortVec[i]->loc, Penemy, ( InteriorObjectType | StaticTSObjectType | StaticShapeObjectType  ), &rInfo))
            {
            end = sortVec[i];
            break;
            }
        }
    }

    if (!(start = findPoint(Pstart)))
        return -1;

    vToPt = (start->loc - Penemy);
    vToPt.normalize();
    if (mDot(Vlook, vToPt) > .8 && !gServerContainer.castRay(start->loc, Penemy, STATIC_COLLISION_MASK, &rInfo))
        return -1;

    if (start == end)
        return 1;

    //ok, we've got start and end NMPoints, time for some a*

    S32 olSize = 1;
    AStarPoint *curpt = openList;
    curpt->pt = start;
    start->asp = curpt;

    curpt->f = (start->loc - end->loc).len();

    int i;

    do {
        BHPop(curpt+1, olSize-1);
        curpt->list = AStarPoint::closed;
        for (i=0;i<8;++i)
        {
            if (!curpt->pt->adjs[i]) //no point
                continue;
            if (curpt->pt->adjs[i] == end) //woohoo!
                goto found;
            vToPt = (curpt->pt->adjs[i]->loc - Penemy);
            vToPt.normalize();
            if (!curpt->pt->adjs[i]->asp && ((okDist ? (Penemy - curpt->pt->adjs[i]->loc).len() < okDist : 0) ||
                    (mDot(Vlook, vToPt) < .8 ) ||
                    gServerContainer.castRay(curpt->pt->adjs[i]->loc, Penemy, STATIC_COLLISION_MASK, &rInfo)))//a new point
            {
                curpt[olSize].pt = curpt->pt->adjs[i];
                curpt[olSize].pt->asp = &curpt[olSize];
                curpt[olSize].parent = curpt;
                curpt[olSize].list = AStarPoint::open;
                curpt[olSize].g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt[olSize].f = curpt[olSize].g + (curpt[olSize].pt->loc - end->loc).len();
                BHAdd(curpt+1, olSize-1); //b/c curpt is the one *before* the first element
                olSize++;
            }
            else if (curpt->pt->adjs[i]->asp && (curpt->pt->adjs[i]->asp->list == AStarPoint::open) &&
                     ((curpt->g + curpt->pt->interval*((i&1)?1.4f:1)) < curpt->pt->adjs[i]->asp->g))  //this is a better path
            {
                curpt->pt->adjs[i]->asp->parent = curpt;
                curpt->pt->adjs[i]->asp->g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
                curpt->pt->adjs[i]->asp->f = curpt->pt->adjs[i]->asp->g +
                                             (curpt->pt->adjs[i]->loc - end->loc).len();
                BHAdd(curpt+1, curpt->pt->adjs[i]->asp - curpt - 1);
            }
        }
        ++curpt;
        --olSize;
     } while (olSize);

     for (AStarPoint *del = openList; del->pt; del++)
     {
        del->pt->asp = 0;
     }

     dMemset(openList, 0, sizeof(AStarPoint)*points);

     return -1;

found:
    
    Vpoints.clear();

    Vpoints.push_front(end->loc);

    while (curpt->pt != start)
    {
        Vpoints.push_front(curpt->pt->loc);
        curpt = curpt->parent;
    } 

    if (mAbs(findDirec(Vpoints[0] - start->loc) - findDirec(Pstart - start->loc)) > 2)
        Vpoints.push_front(start->loc);

	Vpoints.push_front(Pstart);

    for (AStarPoint *del = openList; del->pt; del++)
    {
       del->pt->asp = 0;
    }

    dMemset(openList, 0, sizeof(AStarPoint)*points);

    return 0;
}

S32 AStar::wander (Point3F Pstart, U32 length, Vector<Point3F> &Vpoints)
{
    if (!list)
        return -1;

    if (length == 0)
        return 1;

    NMPoint *start;
    RayInfo rInfo;
    
    if (!(start = findPoint(Pstart)))
       return -1;

    //ok, we've got start and end NMPoints, time for some a*

    S32 olSize = 1;
    AStarPoint *curpt = openList;
    curpt->pt = start;
    start->asp = curpt;

    curpt->f = length;

    int i;

    do {
       BHPop(curpt+1, olSize-1);
       curpt->list = AStarPoint::closed;
       if (curpt->g - length > 0)
           break;
       for (i=0;i<8;++i)
       {
           if (!curpt->pt->adjs[i]) //no point
               continue;
           if (!curpt->pt->adjs[i]->asp) //a new point
           {
               curpt[olSize].pt = curpt->pt->adjs[i];
               curpt[olSize].pt->asp = &curpt[olSize];
               curpt[olSize].parent = curpt;
               curpt[olSize].list = AStarPoint::open;
               curpt[olSize].g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
               curpt[olSize].f = curpt[olSize].g + gRandGen.randF(0, length);
               BHAdd(curpt+1, olSize-1); //b/c curpt is the one *before* the first element
               olSize++;
           }
           else if ((curpt->pt->adjs[i]->asp->list == AStarPoint::open) &&
                    ((curpt->g + curpt->pt->interval*((i&1)?1.4f:1)) < curpt->pt->adjs[i]->asp->g))  //this is a better path
           {
               curpt->pt->adjs[i]->asp->parent = curpt;
               curpt->pt->adjs[i]->asp->g = curpt->g + curpt->pt->interval*((i&1)?1.4f:1);
               curpt->pt->adjs[i]->asp->f = curpt->pt->adjs[i]->asp->g +
                                            gRandGen.randF(0, length);
               BHAdd(curpt+1, curpt->pt->adjs[i]->asp - curpt - 1);
           }
       }
       ++curpt;
       --olSize;
    } while (olSize);

    Vpoints.clear();

    if (!olSize) //if we run out of points, just start with the last one added
        curpt--;

    while (curpt->pt != start)
    {
        Vpoints.push_front(curpt->pt->loc);
        curpt = curpt->parent;
    }

    if (mAbs(findDirec(Vpoints[0] - start->loc) - findDirec(Pstart - start->loc)) > 2)
        Vpoints.push_front(start->loc);

	Vpoints.push_front(Pstart);

    for (AStarPoint *del = openList; del->pt; del++)
    {
       del->pt->asp = 0;
    }

    dMemset(openList, 0, sizeof(AStarPoint)*points);
    
    return 0;
}

//this causes problems with the editor and is inefficient
/*
ConsoleFunction(FindPath, S32, 3, 4, "FindPath(start, end, smooth = true)")
{
   if (gEditingMission)
      return -1; //creating a path while the editor is running causes problems

   Point3F a( 0.0f, 0.0f, 0.0f );
   dSscanf( argv[1], "%g %g %g", &a.x, &a.y, &a.z );

   Point3F b( 0.0f, 0.0f, 0.0f );
   dSscanf( argv[2], "%g %g %g", &b.x, &b.y, &b.z );

   Vector<Point3F> pts;

   S32 tmp;

   if (tmp = gAStar.findBasicPath(a, b, pts))
      return tmp;

   Path* path = new Path;

   if (!path->registerObject())
   {
      Con::errorf(ConsoleLogEntry::General, "AStar: couldn't register path");
      delete path;
      return -1;
   }

   path->Looping(false);

   MatrixF xform(true);
   Marker * marker;
   for (Point3F* pt = pts.begin(); pt != pts.end(); pt++)
   {
      marker = new Marker;
      xform.setPosition(*pt);
      marker->setTransform(xform);
      if (!marker->registerObject())
      {
         delete marker;
         continue;
      }
      path->addObject(marker);
   }

   return path->getId();
};
*/

//---------------------------------------------------------//
//                          Viz                            //
//---------------------------------------------------------//

#ifdef AI_RENDER
void AStar::render(SceneState* state) //, SceneRenderImage*)
{
    if (!Con::getBoolVariable("$pref::AStar::Render", false))
        return;

    RayInfo rInfo;

	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");
	RectI viewport;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	dglGetViewport(&viewport);
	state->setupBaseProjection();
	//glPushMatrix();
	//dglMultMatrix(&getTransform());
    glDisable(GL_CULL_FACE);

    int i;
    glBegin(GL_LINES);
    for (NMPoint* tmp = list; tmp; tmp = tmp->next) //iterate thru list
    {
        glColor3f(0,1,0);
        glVertex3f(tmp->loc.x, tmp->loc.y, tmp->loc.z);
        glVertex3f(tmp->loc.x, tmp->loc.y, tmp->loc.z-1);
        for (i=0; i<8; i++)
		{
            if (!tmp->adjs[i])
                continue;
            glColor3f(0,0,1);
            glVertex3f(tmp->loc.x, tmp->loc.y, tmp->loc.z);
            glVertex3f(tmp->adjs[i]->loc.x, tmp->adjs[i]->loc.y, tmp->adjs[i]->loc.z-1);
        }
    }
    glEnd();

	// Restore our canonical matrix state.
	//glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	dglSetViewport(viewport);
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}
#endif