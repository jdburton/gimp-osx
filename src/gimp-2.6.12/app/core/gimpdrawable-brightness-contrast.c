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

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimpbrightnesscontrastconfig.h"

/* temporary */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpdrawable.h"
#include "gimpdrawable-brightness-contrast.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-process.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_brightness_contrast (GimpDrawable *drawable,
                                   GimpProgress *progress,
                                   gint          brightness,
                                   gint          contrast)
{
  GimpBrightnessContrastConfig *config;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  config = g_object_new (GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                         "brightness", brightness / 127.0,
                         "contrast",   contrast   / 127.0,
                         NULL);

  if (gimp_use_gegl (GIMP_ITEM (drawable)->image->gimp))
    {
      GeglNode    *node;
      const gchar *name;

      name = (gimp_gegl_check_version (0, 0, 21) ?
              "gegl:brightness-contrast" : "brightness-contrast");

      node = g_object_new (GEGL_TYPE_NODE,
                           "operation", name,
                           NULL);
      gimp_brightness_contrast_config_set_node (config, node);

      gimp_drawable_apply_operation (drawable, progress,
                                     _("Brightness_Contrast"), node, TRUE);
      g_object_unref (node);
    }
  else
    {
      GimpLut *lut;

      lut = brightness_contrast_lut_new (config->brightness / 2.0,
                                         config->contrast,
                                         gimp_drawable_bytes (drawable));

      gimp_drawable_process_lut (drawable, progress, _("Brightness-Contrast"),
                                 lut);
      gimp_lut_free (lut);
    }

  g_object_unref (config);
}
