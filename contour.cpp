/******************************************************/
/*                                                    */
/* contour.cpp - generates contours                   */
/*                                                    */
/******************************************************/

/* After finding extrema, generate contours. The method for generating contours
 * as polylines is as follows:
 * 1. Between two corners (i.e. points in the TIN, corners of triangles) in the TIN
 *    that includes the extrema, find a point on the edge that has the given elevation.
 *    Join these points with line segments.
 * 2. Change the line segments to spiralarcs to form a smooth curve.
 * 3. Add points to each spiralarc, staying within the triangle, until the elevation
 *    all along each spiralarc is within the tolerance or the spiralarc is shorter
 *    than the tolerance.
 */
#include <iostream>
#include <cassert>
#include "contour.h"
#include "pointlist.h"
#include "relprime.h"
#include "ps.h"
using namespace std;

float splittab[65]=
{
  0.2113,0.2123,0.2134,0.2145,0.2156,0.2167,0.2179,0.2191,0.2204,0.2216,0.2229,0.2244,0.2257,
  0.2272,0.2288,0.2303,0.2319,0.2337,0.2354,0.2372,0.2390,0.2410,0.2430,0.2451,0.2472,0.2495,
  0.2519,0.2544,0.2570,0.2597,0.2625,0.2654,0.2684,0.2716,0.2750,0.2786,0.2823,0.2861,0.2902,
  0.2945,0.2990,0.3038,0.3088,0.3141,0.3198,0.3258,0.3320,0.3386,0.3454,0.3527,0.3605,0.3687,
  0.3773,0.3862,0.3955,0.4053,0.4153,0.4256,0.4362,0.4469,0.4577,0.4684,0.4792,0.4897,0.5000
};

float splitpoint(double leftclamp,double rightclamp,double tolerance)
/* If the values at the clamp points indicate that the curve may be out of tolerance,
 * returns the point to split it at, as a fraction of the length. If not, returns 0.
 * tolerance must be positive.
 */
{
  bool whichbig;
  double ratio;
  float sp;
  if (std::isnan(leftclamp))
    sp=CCHALONG;
  else if (std::isnan(rightclamp))
    sp=1-CCHALONG;
  else if (fabs(leftclamp)*27>tolerance*23 || fabs(rightclamp)*27>tolerance*23)
  {
    whichbig=fabs(rightclamp)>fabs(leftclamp);
    ratio=whichbig?(leftclamp/rightclamp):(rightclamp/leftclamp);
    sp=splittab[(int)rint((ratio+1)*32)];
    if (whichbig)
      sp=1-sp;
  }
  else
    sp=0;
  return sp;
}

vector<uintptr_t> contstarts(pointlist &pts,double elev)
{
  vector<uintptr_t> ret;
  uintptr_t ep;
  int sd,io;
  triangle *tri;
  int i,j;
  //cout<<"Exterior edges:";
  for (io=0;io<2;io++)
    for (i=0;i<pts.edges.size();i++)
      if (io==pts.edges[i].isinterior())
      {
	tri=pts.edges[i].tria;
	if (!tri)
	  tri=pts.edges[i].trib;
	assert(tri);
	//cout<<' '<<i;
	for (j=0;j<3;j++)
	{
	  ep=j+(uintptr_t)&pts.edges[i];
	  sd=tri->subdir(ep);
	  if (tri->crosses(sd,elev) && (io || tri->upleft(sd)))
	  {
	    //cout<<(char)(j+'a');
	    ret.push_back(ep);
	  }
	}
      }
  //cout<<endl;
  return ret;
}

void mark(uintptr_t ep)
{
  ((edge *)(ep&-4))->mark(ep&3);
}

bool ismarked(uintptr_t ep)
{
  return ((edge *)(ep&-4))->ismarked(ep&3);
}

polyline intrace(triangle *tri,double elev)
/* Returns the contour that is inside the triangle, if any. The contour is an elliptic curve.
 * If a contour is wholly inside a triangle, there is at most one contour partly in it.
 * If there is no contour wholly inside the triangle, there can be three partly inside it.
 * Start at a subsegment and trace the contour. One of three things will happen:
 * • You get a segment number greater than the number of subsegments (i.e. 65535). You've exited the triangle.
 * • You get a segment number less than the one you started with. You're retracing a contour you traced already.
 * • You get the segment number you started with. You've found a contour inside the triangle.
 */
{
  polyline ret(elev);
  int i,j=-610,start=-987;
  vector<int> sube;
  xy cept;
  for (i=0;i<tri->subdiv.size();i++)
    if (tri->crosses(i,elev))
    {
      start=i;
      if (!tri->upleft(start))
	start+=65536;
      sube.clear();
      for (j=start;sube.size()==0 || (j&65535)>(start&65535) && (j&65535)<tri->subdiv.size() && sube.size()<256;j=tri->proceed(j,elev))
	sube.push_back(j);
      if (j==start)
	break;
    }
  if (j==start)
    for (i=0;i<sube.size();i++)
    {
      cept=tri->contourcept(sube[i],elev);
      if (cept.isfinite())
	ret.insert(cept);
    }
  return ret;
}

polyline trace(uintptr_t edgep,double elev)
{
  polyline ret(elev);
  int subedge,subnext,i;
  uintptr_t prevedgep;
  bool wasmarked;
  xy lastcept,thiscept,firstcept;
  triangle *tri,*ntri;
  tri=((edge *)(edgep&-4))->tria;
  ntri=((edge *)(edgep&-4))->trib;
  if (tri==nullptr || !tri->upleft(tri->subdir(edgep)))
    tri=ntri;
  mark(edgep);
  //cout<<"Start edgep "<<edgep<<endl;
  firstcept=lastcept=tri->contourcept(tri->subdir(edgep),elev);
  if (firstcept.isnan())
  {
    cerr<<"Tracing STARTS on Nan"<<endl;
    return ret;
  }
  ret.insert(firstcept);
  do
  {
    prevedgep=edgep;
    subedge=tri->subdir(edgep);
    //cout<<"before loop "<<subedge<<' '<<subnext<<endl;
    i=0;
    do
    {
      subnext=tri->proceed(subedge,elev);
      if (subnext>=0)
      {
	if (subnext==subedge)
	  cerr<<"proceed failed! "<<ret.size()<<endl;
	subedge=subnext;
	thiscept=tri->contourcept(subedge,elev);
	if (thiscept.isfinite())
	{
	  if (thiscept!=lastcept)
	    ret.insert(thiscept);
	  else
	    cerr<<"Repeated contourcept: "<<edgep<<' '<<ret.size()<<endl;
	  lastcept=thiscept;
	}
	//else
	  //cerr<<"NaN contourcept"<<endl;
      }
    } while (subnext>=0 && ++i<256);
    //cout<<"after loop "<<subedge<<' '<<subnext<<endl;
    edgep=tri->edgepart(subedge);
    //cout<<"Next edgep "<<edgep<<endl;
    if (edgep==prevedgep)
    {
      cout<<"Edge didn't change"<<endl;
      subedge=tri->subdir(edgep);
      subnext=tri->proceed(subedge,elev);
      if (subnext>=0)
      {
	if (subnext==subedge)
	  cout<<"proceed failed!"<<endl;
	subedge=subnext;
	//ret.insert(tri->contourcept(subedge,elev));
      }
    }
    if (edgep==0)
    {
      // This happens in Independence Park triangle (697 681 564) at elevation 205.6.
      ntri=nullptr;
      cout<<"Tracing stopped in middle of a triangle "<<ret.size()<<endl;
      subedge=tri->subdir(prevedgep);
      subnext=tri->proceed(subedge,elev);
    }
    else
    {
      wasmarked=ismarked(edgep);
      if (!wasmarked)
      {
	thiscept=tri->contourcept(tri->subdir(edgep),elev);
	if (thiscept!=lastcept && thiscept!=firstcept && thiscept.isfinite())
	  ret.insert(thiscept);
	lastcept=thiscept;
      }
      mark(edgep);
      ntri=((edge *)(edgep&-4))->othertri(tri);
    }
    if (ntri)
      tri=ntri;
  } while (ntri && !wasmarked);
  if (!ntri)
    ret.open();
  return ret;
}

void roughcontours(pointlist &pl,double conterval)
/* Draws contours consisting of line segments.
 * The perimeter must be present in the triangles.
 * Do not attempt to draw contours in the Mariana Trench with conterval
 * less than 5 µm or of Chomolungma with conterval less than 4 µm. It will fail.
 */
{
  vector<double> tinlohi;
  vector<uintptr_t> cstarts;
  polyline ctour;
  int i,j;
  pl.contours.clear();
  tinlohi=pl.lohi(); // FIXME produces garbage for Independence Park
  for (i=floor(tinlohi[0]/conterval);i<=ceil(tinlohi[1]/conterval);i++)
  {
    cstarts=contstarts(pl,i*conterval);
    pl.clearmarks();
    for (j=0;j<cstarts.size();j++)
      if (!ismarked(cstarts[j]))
      {
	ctour=trace(cstarts[j],i*conterval);
	ctour.dedup();
	//for (j=0;j<ctour.size();j++)
	//cout<<"Contour length: "<<ctour.length()<<endl;
	pl.contours.push_back(ctour);
      }
    for (j=0;j<pl.triangles.size();j++)
    {
      ctour=intrace(&pl.triangles[j],i*conterval);
      if (ctour.size())
      {
	pl.contours.push_back(ctour);
	//cout<<"Contour length: "<<ctour.length()<<endl;
      }
    }
  }
}

void smoothcontours(pointlist &pl,double conterval,bool log)
{
  int i,j,k,n=0,sz,origsz;
  double sp,wide;
  double we,ea,so,no;
  xyz lpt,rpt,newpt;
  xy spt;
  segment splitseg;
  spiralarc sarc;
  triangle *midptri;
  ofstream logfile;
  we=pl.dirbound(0);
  so=pl.dirbound(DEG90);
  ea=-pl.dirbound(DEG180);
  no=-pl.dirbound(DEG270);
  if (log)
  {
    psopen("smoothcontours.ps");
    psprolog();
  }
  for (i=0;i<pl.contours.size();i++)
  {
    cout<<"smoothcontours "<<i<<'/'<<pl.contours.size()<<" elev "<<pl.contours[i].getElevation()<<" \r";
    cout.flush();
    if (fabs(pl.contours[i].getElevation()-203.2)<1e-9 && pl.contours[i].size()==11)
      cout<<"203.2 11"<<endl;
    /* Smooth the contours in two passes. The first works with straight lines
     * and uses 1/2 the conterval for tolerance. The second works with spiral
     * curves and uses 1/10 the conterval for tolerance.
     */
    for (k=0;k<2;k++)
    {
      if (k)
	pl.contours[i].smooth();
      origsz=sz=pl.contours[i].size();
      for (j=0;j<sz;j++)
      {
	n=(n+relprime(sz))%sz;
	wide=((sz>2*origsz)?(sz/(double)origsz):1)*(k?0.5:0.1);
	sarc=pl.contours[i].getspiralarc(n);
	lpt=sarc.station(sarc.length()*CCHALONG);
	rpt=sarc.station(sarc.length()*(1-CCHALONG));
	if (lpt.isfinite() && rpt.isfinite())
	{
	  if (sarc.getdelta()==883276262)
	    cout<<"883276262"<<endl;
	  midptri=pl.qinx.findt((sarc.getstart()+sarc.getend())/2);
	  if (midptri)
	    if (midptri->in(sarc.getstart()) && midptri->in(sarc.getend()) &&
	      !(midptri->in(lpt) && midptri->in(rpt)))
	      sp=splitpoint(lpt.elev()-pl.elevation(lpt),rpt.elev()-pl.elevation(rpt),0);
	    else
	      sp=splitpoint(lpt.elev()-midptri->elevation(lpt),rpt.elev()-midptri->elevation(rpt),conterval*wide);
	  else
	  {
	    sp=0.5;
	    cerr<<"can't happen: midptri is null"<<endl;
	  }
	  if (sp && sarc.length()>conterval)
	  {
	    //cout<<"segment "<<n<<" of "<<sz<<" of contour "<<i<<" needs splitting at "<<sp<<endl;
	    spt=sarc.getstart()+sp*(sarc.getend()-sarc.getstart());
	    splitseg=pl.qinx.findt(spt)->dirclip(spt,dir(xy(sarc.getend()),xy(sarc.getstart()))+DEG90);
	    newpt=splitseg.station(splitseg.contourcept(pl.contours[i].getElevation()));
	    if (newpt.isfinite())
	    {
	      pl.contours[i].insert(newpt,n+1);
	      sz++;
	      j=0;
	    }
	    if (log)
	    {
	      startpage();
	      setscale(we,so,ea,no,0);
	      setcolor(0,0,0);
	      spline(pl.contours[i].approx3d(0.1));
	      setcolor(0,0,1);
	      spline(splitseg.approx3d(0.1));
	      endpage();
	    }
	  }
	}
      }
    }
  }
  if (log)
  {
    pstrailer();
    psclose();
  }
}
