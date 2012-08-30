/******************************************************/
/*                                                    */
/* segment.cpp - 3d line segment                      */
/* base class of arc and spiral                       */
/*                                                    */
/******************************************************/

#include "segment.h"
#include "vcurve.h"
#include <cmath>

segment::segment()
{
  start=end=xyz(0,0,0);
  control1=control2=0;
}

segment::segment(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
  control1=(2*start.elev()+end.elev())/3;
  control2=(start.elev()+2*end.elev())/3;
}

double segment::length()
{
  return dist(xy(start),xy(end));
}

void segment::setslope(int which,double s)
{
  switch(which)
  {
    case START:
      control1=(2*start.elev()+end.elev()+s*length())/3;
      break;
    case END:
      control2=(start.elev()+2*end.elev()-s*length())/3;
      break;
  }
}

double segment::elev(double along)
{
  return vcurve(start.elev(),control1,control2,end.elev(),along/length());
}

double segment::slope(double along)
{
  return vslope(start.elev(),control1,control2,end.elev(),along/length())/length();
}

xyz segment::station(double along)
{
  double gnola,len;
  len=length();
  gnola=len-along;
  return xyz((start.east()*gnola+end.east()*along)/len,(start.north()*gnola+end.north()*along)/len,
	     elev(along));
}

xyz segment::midpoint()
{
  return station(length()/2);
}

xy segment::center()
{
  return xy(nan(""),nan(""));
}
