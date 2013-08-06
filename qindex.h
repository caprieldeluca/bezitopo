/******************************************************/
/*                                                    */
/* qindex.h - quad index to tin                       */
/*                                                    */
/******************************************************/

#include <vector>
#include "bezier.h"
#include "point.h"
class qindex
{
public:
  double x,y,side;
  union
  {
    qindex *sub[4]; // Either all four subs are set,
    triangle *tri;  // or tri alone is set, or they're all NULL.
  };
  triangle *findt(xy pnt);
  xy middle();
  void sizefit(std::vector<xy> pnts);
  void clear();
  qindex();
  ~qindex();
};
