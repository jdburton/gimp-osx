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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


static cairo_surface_t * print_cairo_surface_from_drawable (gint32 drawable_ID);

static inline void
convert_from_rgb (const guchar *src,
                  guchar       *dest,
                  gint          pixels)
{
  while (pixels--)
    {
      GIMP_CAIRO_RGB24_SET_PIXEL (dest,
                                  src[0], src[1], src[2]);

      src  += 3;
      dest += 4;
    }
}

static inline void
convert_from_rgba (const guchar *src,
                   guchar       *dest,
                   gint          pixels)
{
  while (pixels--)
    {
      GIMP_CAIRO_ARGB32_SET_PIXEL (dest,
                                   src[0], src[1], src[2], src[3]);

      src  += 4;
      dest += 4;
    }
}

static inline void
convert_from_gray (const guchar *src,
                   guchar       *dest,
                   gint          pixels)
{
  while (pixels--)
    {
      GIMP_CAIRO_RGB24_SET_PIXEL (dest,
                                  src[0], src[0], src[0]);

      src  += 1;
      dest += 4;
    }
}

static inline void
convert_from_graya (const guchar *src,
                    guchar       *dest,
                    gint          pixels)
{
  while (pixels--)
    {
      GIMP_CAIRO_ARGB32_SET_PIXEL (dest,
                                   src[0], src[0], src[0], src[1]);

      src  += 2;
      dest += 4;
    }
}

static inline void
convert_from_indexed (const guchar *src,
                      guchar       *dest,
                      gint          pixels,
                      const guchar *cmap)
{
  while (pixels--)
    {
      const gint i = 3 * src[0];

      GIMP_CAIRO_RGB24_SET_PIXEL (dest,
                                  cmap[i], cmap[i + 1], cmap[i + 2]);

      src  += 1;
      dest += 4;
    }
}

static inline void
convert_from_indexeda (const guchar *src,
                       guchar       *dest,
                       gint          pixels,
                       const guchar *cmap)
{
  while (pixels--)
    {
      const gint i = 3 * src[0];

      GIMP_CAIRO_ARGB32_SET_PIXEL (dest,
                                   cmap[i], cmap[i + 1], cmap[i + 2], src[1]);

      src  += 2;
      dest += 4;
    }
}

gboolean
print_draw_page (GtkPrintContext *context,
                 PrintData       *data)
{
  cairo_t         *cr;
  cairo_surface_t *surface;

  cr = gtk_print_context_get_cairo_context (context);

  surface = print_cairo_surface_from_drawable (data->drawable_id);

  cairo_translate (cr, data->offset_x, data->offset_y);
  cairo_scale (cr,
	       gtk_print_context_get_dpi_x (context) / data->xres,
	       gtk_print_context_get_dpi_y (context) / data->yres);

  cairo_rectangle (cr,
                   0, 0,
                   cairo_image_surface_get_width (surface),
                   cairo_image_surface_get_height (surface));
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_fill (cr);
  cairo_surface_destroy (surface);

  return TRUE;
}

static cairo_surface_t *
print_cairo_surface_from_drawable (gint32 drawable_ID)
{
  GimpDrawable    *drawable      = gimp_drawable_get (drawable_ID);
  GimpPixelRgn     region;
  GimpImageType    image_type    = gimp_drawable_type (drawable_ID);
  cairo_surface_t *surface;
  const gint       width         = drawable->width;
  const gint       height        = drawable->height;
  guchar           cmap[3 * 256] = { 0, };
  guchar          *pixels;
  gint             stride;
  guint            count         = 0;
  guint            done          = 0;
  gpointer         pr;

  if (gimp_drawable_is_indexed (drawable_ID))
    {
      guchar *colors;
      gint    num_colors;

      colors = gimp_image_get_colormap (gimp_drawable_get_image (drawable_ID),
                                        &num_colors);
      memcpy (cmap, colors, 3 * num_colors);
      g_free (colors);
    }

  surface = cairo_image_surface_create (gimp_drawable_has_alpha (drawable_ID) ?
                                        CAIRO_FORMAT_ARGB32 :
                                        CAIRO_FORMAT_RGB24,
                                        width, height);

  pixels = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &region);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      const guchar *src  = region.data;
      guchar       *dest = pixels + region.y * stride + region.x * 4;
      gint          y;

      for (y = 0; y < region.h; y++)
        {
          switch (image_type)
            {
            case GIMP_RGB_IMAGE:
              convert_from_rgb (src, dest, region.w);
              break;

            case GIMP_RGBA_IMAGE:
              convert_from_rgba (src, dest, region.w);
              break;

            case GIMP_GRAY_IMAGE:
              convert_from_gray (src, dest, region.w);
              break;

            case GIMP_GRAYA_IMAGE:
              convert_from_graya (src, dest, region.w);
              break;

            case GIMP_INDEXED_IMAGE:
              convert_from_indexed (src, dest, region.w, cmap);
              break;

            case GIMP_INDEXEDA_IMAGE:
              convert_from_indexeda (src, dest, region.w, cmap);
              break;
            }

          src  += region.rowstride;
          dest += stride;
        }

      done += region.h * region.w;

      if (count++ % 16 == 0)
        gimp_progress_update ((gdouble) done / (width * height));
    }

  gimp_drawable_detach (drawable);

  cairo_surface_mark_dirty (surface);

  return surface;
}

