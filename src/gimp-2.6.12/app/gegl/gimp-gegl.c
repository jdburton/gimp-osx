/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include "config.h"

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "base/tile.h"

#include "gimp-gegl.h"
#include "gimpoperationcolorbalance.h"
#include "gimpoperationcolorize.h"
#include "gimpoperationcurves.h"
#include "gimpoperationdesaturate.h"
#include "gimpoperationhuesaturation.h"
#include "gimpoperationlevels.h"
#include "gimpoperationposterize.h"
#include "gimpoperationthreshold.h"
#include "gimpoperationtilesink.h"
#include "gimpoperationtilesource.h"


void
gimp_gegl_init (void)
{
  g_object_set (gegl_config (),
                "tile-width",  TILE_WIDTH,
                "tile-height", TILE_HEIGHT,
                NULL);

  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_BALANCE);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLORIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_CURVES);
  g_type_class_ref (GIMP_TYPE_OPERATION_DESATURATE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HUE_SATURATION);
  g_type_class_ref (GIMP_TYPE_OPERATION_LEVELS);
  g_type_class_ref (GIMP_TYPE_OPERATION_POSTERIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_THRESHOLD);
  g_type_class_ref (GIMP_TYPE_OPERATION_TILE_SINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_TILE_SOURCE);
}
