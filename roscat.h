/******************************************************/
/*                                                    */
/* roscat.h - rotate, scale, translate                */
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
#ifndef ROSCAT_H
#define ROSCAT_H
#include "xyz.h"

struct RoscatStruct
/* This does not roscat anything. It is only a struct, so that absorient
 * can return it. To roscat, see the roscat methods of drawobj and other classes.
 */
{
  xy tfrom;
  int ro;
  double sca;
  xy tto;
};
#endif
