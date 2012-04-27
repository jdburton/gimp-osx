/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __TOOLS_UTILS_H__
#define __TOOLS_UTILS_H__


/*
 * Common values for the n_snap_lines parameter of
 * gimp_tool_motion_constrain.
 */
#define GIMP_TOOL_CONSTRAIN_90_DEGREES 2
#define GIMP_TOOL_CONSTRAIN_45_DEGREES 4
#define GIMP_TOOL_CONSTRAIN_15_DEGREES 12


void  gimp_tool_motion_constrain (gdouble   start_x,
                                  gdouble   start_y,
                                  gdouble  *end_x,
                                  gdouble  *end_y,
                                  gint      n_snap_lines);


#endif  /*  __TOOLS_UTILS_H__  */
