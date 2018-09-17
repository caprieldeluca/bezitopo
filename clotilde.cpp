/******************************************************/
/*                                                    */
/* clotilde.cpp - tables of approximations to spirals */
/*                                                    */
/******************************************************/
/* Copyright 2018 Pierre Abbat.
 * This file is part of Bezitopo.
 * 
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bezitopo. If not, see <http://www.gnu.org/licenses/>.
 */
/* This program is named Clotilde for similarity to "clotoide", the Spanish
 * word for the Euler spiral.
 */
#include <iostream>
#include "manyarc.h"
#include "vball.h"
#include "cmdopt.h"
#include "config.h"
using namespace std;

int verbosity=1;
bool helporversion=false,commandError=false;
double arcLength=NAN,chordLength=NAN;
vector<double> curvature;
vector<int> lengthUnits,angleUnits;

vector<option> options(
  {
    {'h',"help","","Help using the program"},
    {'\0',"version","","Output version number"},
    {'l',"length","length","Arc length"},
    {'C',"chordlength","length","Chord length"},
    {'c',"curvature","cur cur","Start and end curvatures"},
    {'r',"radius","length length","Start and end radii"},
    {'u',"unit","m/ft/deg/dms","Length or angle unit"}
  });

vector<token> cmdline;

void outhelp()
{
  int i,j;
  cout<<"Clotilde outputs approximations to spiralarcs. Example:\n"
    <<"clotilde -u m -l 200 -r inf 900\n"
    <<"approximates a 200-meter-long spiral starting straight and ending on 900 m radius.\n"
    <<"clotilde -u ft -l 500 -c 0 7\n"
    <<"approximates a 500-foot-long spiral starting straight and ending on a 7° curve.\n"
    <<"clotilde -u usft -l 500 -c 0 7 -u m\n"
    <<"approximates a 500-USfoot-long spiral, outputting the arcs in meters.\n"
    <<"When using feet, curvature is expressed as angle of 100 ft arc,\n"
    <<"and clothance is expressed as change in 100 ft of angle of 100 ft arc.\n";
  for (i=0;i<options.size();i++)
  {
    cout<<(options[i].shopt?options[i].shopt:' ')<<' ';
    cout<<options[i].lopt;
    for (j=options[i].lopt.length();j<14;j++)
      cout<<' ';
    cout<<options[i].args;
    for (j=options[i].args.length();j<20;j++)
      cout<<' ';
    cout<<options[i].desc<<endl;
  }
}

void startHtml(spiralarc s,Measure ms)
{
  cout<<"<html><head><title>Approximation ";
  cout<<ms.formatMeasurementUnit(s.length(),LENGTH,0,0.522)<<"</title></head><body>\n";
}

void endHtml()
{
  cout<<"</body></html>\n";
}

void outSpiral(spiralarc s,Measure ms)
{
  double startCur=s.curvature(0),endCur=s.curvature(s.length());
  if (fabs(startCur)<1/EARTHRAD)
    startCur=0;
  if (fabs(endCur)<1/EARTHRAD)
    endCur=0;
  cout<<"<table border><tr><td><h1>Arc length: "<<ms.formatMeasurementUnit(s.length(),LENGTH)<<"</h1></td>\n";
  cout<<"<td><h1>Chord length: "<<ms.formatMeasurementUnit(s.chordlength(),LENGTH)<<"</h1></td></tr>\n";
  cout<<"<tr><td><h1>Start curvature: "<<formatCurvature(startCur,ms)<<"</h1></td>\n";
  cout<<"<td><h1>End curvature: "<<formatCurvature(endCur,ms)<<"</h1></td></tr>\n";
  cout<<"<tr><td><h1>Start radius: "<<ms.formatMeasurementUnit(1/startCur,LENGTH)<<"</h1></td>\n";
  cout<<"<td><h1>End radius: "<<ms.formatMeasurementUnit(1/endCur,LENGTH)<<"</h1></td></tr>\n";
  cout<<"<tr><td><h1>Clothance: "<<formatClothance(s.clothance(),ms)<<"</h1></td>\n";
  cout<<"<td><h1>Delta: "<<ms.formatMeasurementUnit(s.getdelta(),ANGLE_B)<<"</h1></td></tr></table>\n";
}

void outArc(arc oneArc,Measure ms)
{
  double relprec=abs(oneArc.getdelta());
  if (relprec==0)
    relprec=1;
  cout<<"<tr><td colspan=4>"<<ms.formatMeasurementUnit(oneArc.length(),LENGTH)<<"</td>";
  cout<<"<td colspan=4>"<<ms.formatMeasurementUnit(oneArc.chordlength(),LENGTH)<<"</td>";
  cout<<"<td colspan=4>"<<ms.formatMeasurementUnit(oneArc.getdelta(),ANGLE_B)<<"</td>";
  cout<<"<td colspan=4>"<<formatCurvature(oneArc.curvature(0),ms,oneArc.curvature(0)/relprec)<<"</td>";
  cout<<"<td colspan=4>"<<ms.formatMeasurementUnit(oneArc.radius(0),LENGTH,0,oneArc.radius(0)/relprec)<<"</td></tr>\n";
}

void outPoint(xy pnt,spiralarc s,Measure ms)
{
  int sb=s.startbearing(),eb=s.endbearing();
  xy sp=s.getstart(),ep=s.getend();
  cout<<"<tr><td colspan=5>"<<ms.formatMeasurementUnit((sp==pnt)?0:(dir(sp,pnt)-sb),ANGLE_B)<<"</td>";
  cout<<"<td colspan=5>"<<ms.formatMeasurementUnit(dist(sp,pnt),LENGTH)<<"</td>";
  cout<<"<td colspan=5>"<<ms.formatMeasurementUnit((pnt==ep)?0:(dir(pnt,ep)-eb),ANGLE_B)<<"</td>";
  cout<<"<td colspan=5>"<<ms.formatMeasurementUnit(dist(pnt,ep),LENGTH)<<"</td></tr>\n";
}

void outApprox(polyarc approx,spiralarc s,Measure ms)
{
  int i;
  arc oneArc;
  double err=maxError(approx,s);
  cout<<"<table border><tr><th colspan=20>"<<approx.size()<<" arcs, error "
    <<ms.formatMeasurementUnit(err,LENGTH,0,err/32)<<"</th></tr>\n";
  for (i=0;i<approx.size();i++)
  {
    oneArc=approx.getarc(i);
    outPoint(oneArc.getstart(),s,ms);
    outArc(oneArc,ms);
  }
  outPoint(oneArc.getend(),s,ms);
  cout<<"</table>\n";
}

void argpass2()
/* Pass 2 does not parse lengths or curvatures, since the units may be
 * specified after the lengths and curvatures, or both input and output
 * units may be specified before the lengths.
 */
{
  int i,j;
  for (i=0;i<cmdline.size();i++)
    switch (cmdline[i].optnum)
    {
      case 0:
	helporversion=true;
	outhelp();
	break;
      case 1:
	helporversion=true;
	cout<<"Clotilde, part of Bezitopo version "<<VERSION<<" © "<<COPY_YEAR<<" Pierre Abbat\n"
	<<"Distributed under GPL v3 or later. This is free software with no warranty."<<endl;
	break;
      case 2: // arc length
        if (i+1<cmdline.size() && cmdline[i+1].optnum<0)
	{
	  i++;
	}
	break;
      case 3: // chord length
        if (i+1<cmdline.size() && cmdline[i+1].optnum<0)
	{
	  i++;
	}
	break;
      case 4: // curvature
        if (i+1<cmdline.size() && cmdline[i+1].optnum<0)
	{
	  i++;
	}
	break;
      case 5: // radius
        if (i+1<cmdline.size() && cmdline[i+1].optnum<0)
	{
	  i++;
	}
	break;
      case 6:
        if (i+1<cmdline.size() && cmdline[i+1].optnum<0)
	{
	  i++;
	  if (cmdline[i].nonopt=="m")
	    lengthUnits.push_back(255);
	  else if (cmdline[i].nonopt=="ft")
	    lengthUnits.push_back(INTERNATIONAL);
	  else if (cmdline[i].nonopt=="usft")
	    lengthUnits.push_back(USSURVEY);
	  else if (cmdline[i].nonopt=="inft")
	    lengthUnits.push_back(INSURVEY);
	  else if (cmdline[i].nonopt=="deg")
	    angleUnits.push_back(9000);
	  else if (cmdline[i].nonopt=="dms")
	    angleUnits.push_back(5400);
	  else if (cmdline[i].nonopt=="gon")
	    angleUnits.push_back(10000);
	  else
	  {
	    commandError=true;
	    cerr<<"Unrecognized unit "<<cmdline[i].nonopt<<"; should be m, ft, usft, inft, deg, dms, or gon.\n";
	  }
	}
	else
	{
	  cerr<<"--unit requires an argument, one of m, ft, usft, inft, deg, dms, and gon.\n";
	  commandError=true;
	}
	break;
      default:
	;
    }
}

/* Ways to specify the spiralarc to be approximated:
 * • Start radius, end radius, arc length
 * • Start curvature, end curvature, arc length
 * • Start radius, end radius, chord length
 * • Start curvature, end curvature, chord length
 * Curvature may be specified in diopters or degrees; if in degrees, the length
 * is assumed to be 100 unless otherwise specified.
 */
int main(int argc, char *argv[])
{
  int i;
  spiralarc s;
  spiralarc trans(xyz(0,0,0),0,0.003,xyz(500,0,0));
  polyarc approx;
  Measure ms;
  ms.setMetric();
  ms.setDefaultUnit(LENGTH,0.552);
  ms.setDefaultPrecision(LENGTH,2e-6);
  ms.setDefaultUnit(CURVATURE,0.001);
  ms.setDefaultPrecision(CURVATURE,2e-9);
  ms.setDefaultUnit(CLOTHANCE,1e-6);
  ms.setDefaultPrecision(CLOTHANCE,2e-12);
  ms.setDefaultPrecision(ANGLE_B,1);
  ms.setDefaultPrecision(ANGLE,bintorad(1));
  argpass1(argc,argv);
  argpass2();
  if (angleUnits.size()>2 || lengthUnits.size()>2)
    commandError=true;
  if (lengthUnits.size())
    if (lengthUnits[0]==255)
      ms.setMetric();
    else
    {
      ms.setCustomary();
      ms.setFoot(lengthUnits[0]);
    }
  if (angleUnits.size())
    switch (angleUnits[0])
    {
      case 10000:
	ms.addUnit(GON);
	ms.addUnit(GON_B);
	break;
      case 9000:
	ms.addUnit(DEGREE);
	ms.addUnit(DEGREE_B);
	break;
      case 5400:
	ms.addUnit(ARCSECOND+DECIMAL+FIXLARGER);
	ms.addUnit(ARCSECOND_B+DECIMAL+FIXLARGER);
	break;
    }
  else
  {
    ms.addUnit(ARCSECOND+DECIMAL+FIXLARGER);
    ms.addUnit(ARCSECOND_B+DECIMAL+FIXLARGER);
  }
  if (!commandError)
  {
    startHtml(trans,ms);
    outSpiral(trans,ms);
    i=2;
    do
    {
      approx=manyArc(trans,i);
      outApprox(approx,trans,ms);
      i++;
    } while (maxError(approx,trans)>0.01);
    endHtml();
  }
  return 0;
}
