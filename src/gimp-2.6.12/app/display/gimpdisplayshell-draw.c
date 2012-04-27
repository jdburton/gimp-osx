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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "base/tile-manager.h"

#include "core/gimpcontext.h"
#include "core/gimpgrid.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimplist.h"
#include "core/gimpprojection.h"
#include "core/gimpsamplepoint.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"


/*  local function prototypes  */

static GdkGC * gimp_display_shell_get_grid_gc (GimpDisplayShell *shell,
                                               GimpGrid         *grid);
static GdkGC * gimp_display_shell_get_pen_gc  (GimpDisplayShell *shell,
                                               GimpContext      *context,
                                               GimpActiveColor   active);


/*  public functions  */

/**
 * gimp_display_shell_get_scaled_image_size:
 * @shell:
 * @w:
 * @h:
 *
 * Gets the size of the rendered image after it has been scaled.
 *
 **/
void
gimp_display_shell_draw_get_scaled_image_size (const GimpDisplayShell *shell,
                                               gint                   *w,
                                               gint                   *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_draw_get_scaled_image_size_for_scale (shell,
                                                           gimp_zoom_model_get_factor (shell->zoom),
                                                           w,
                                                           h);
}

/**
 * gimp_display_shell_draw_get_scaled_image_size_for_scale:
 * @shell:
 * @scale:
 * @w:
 * @h:
 *
 **/
void
gimp_display_shell_draw_get_scaled_image_size_for_scale (const GimpDisplayShell *shell,
                                                         gdouble                 scale,
                                                         gint                   *w,
                                                         gint                   *h)
{
  GimpProjection *proj;
  TileManager    *tiles;
  gdouble         scale_x;
  gdouble         scale_y;
  gint            level;
  gint            level_width;
  gint            level_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_IMAGE (shell->display->image));

  proj = gimp_image_get_projection (shell->display->image);

  gimp_display_shell_calculate_scale_x_and_y (shell, scale, &scale_x, &scale_y);

  level = gimp_projection_get_level (proj, scale_x, scale_y);

  tiles = gimp_projection_get_tiles_at_level (proj, level, NULL);

  level_width  = tile_manager_width (tiles);
  level_height = tile_manager_height (tiles);

  if (w) *w = PROJ_ROUND (level_width  * (scale_x * (1 << level)));
  if (h) *h = PROJ_ROUND (level_height * (scale_y * (1 << level)));
}

void
gimp_display_shell_draw_guide (const GimpDisplayShell *shell,
                               GimpGuide              *guide,
                               gboolean                active)
{
  gint  position;
  gint  x1, y1, x2, y2;
  gint  x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  position = gimp_guide_get_position (guide);

  if (position < 0)
    return;

  x1 = 0;
  y1 = 0;

  gdk_drawable_get_size (shell->canvas->window, &x2, &y2);

  switch (gimp_guide_get_orientation (guide))
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_transform_xy (shell, 0, position, &x, &y, FALSE);
      y1 = y2 = y;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_transform_xy (shell, position, 0, &x, &y, FALSE);
      x1 = x2 = x;
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      return;
    }

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                         (active ?
                          GIMP_CANVAS_STYLE_GUIDE_ACTIVE :
                          GIMP_CANVAS_STYLE_GUIDE_NORMAL), x1, y1, x2, y2);
}

void
gimp_display_shell_draw_guides (const GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->display->image &&
      gimp_display_shell_get_show_guides (shell))
    {
      GList *list;

      for (list = gimp_image_get_guides (shell->display->image);
           list;
           list = g_list_next (list))
        {
          gimp_display_shell_draw_guide (shell, list->data, FALSE);
        }
    }
}

void
gimp_display_shell_draw_grid (GimpDisplayShell   *shell,
                              const GdkRectangle *area)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (area != NULL);

  if (shell->display->image &&
      gimp_display_shell_get_show_grid (shell))
    {
      GimpGrid   *grid;
      GimpCanvas *canvas;
      gdouble     x, y;
      gint        x0, x1, x2, x3;
      gint        y0, y1, y2, y3;
      gint        x_real, y_real;
      gdouble     x_offset, y_offset;
      gint        width, height;

#define CROSSHAIR 2

      grid = GIMP_GRID (shell->display->image->grid);
      if (! grid)
        return;

      g_return_if_fail (grid->xspacing > 0 && grid->yspacing > 0);

      x1 = area->x;
      y1 = area->y;
      x2 = area->x + area->width;
      y2 = area->y + area->height;

      width  = gimp_image_get_width  (shell->display->image);
      height = gimp_image_get_height (shell->display->image);

      x_offset = grid->xoffset;
      while (x_offset > 0)
        x_offset -= grid->xspacing;

      y_offset = grid->yoffset;
      while (y_offset > 0)
        y_offset -= grid->yspacing;

      canvas = GIMP_CANVAS (shell->canvas);

      gimp_canvas_set_custom_gc (canvas,
                                 gimp_display_shell_get_grid_gc (shell, grid));

      switch (grid->style)
        {
        case GIMP_GRID_DOTS:
          for (x = x_offset; x <= width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real < x1 || x_real >= x2)
                continue;

              for (y = y_offset; y <= height; y += grid->yspacing)
                {
                  if (y < 0)
                    continue;

                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (y_real >= y1 && y_real < y2)
                    gimp_canvas_draw_point (GIMP_CANVAS (shell->canvas),
                                            GIMP_CANVAS_STYLE_CUSTOM,
                                            x_real, y_real);
                }
            }
          break;

        case GIMP_GRID_INTERSECTIONS:
          for (x = x_offset; x <= width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real + CROSSHAIR < x1 || x_real - CROSSHAIR >= x2)
                continue;

              for (y = y_offset; y <= height; y += grid->yspacing)
                {
                  if (y < 0)
                    continue;

                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (y_real + CROSSHAIR < y1 || y_real - CROSSHAIR >= y2)
                    continue;

                  if (x_real >= x1 && x_real < x2)
                    gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                           x_real,
                                           CLAMP (y_real - CROSSHAIR,
                                                  y1, y2 - 1),
                                           x_real,
                                           CLAMP (y_real + CROSSHAIR,
                                                  y1, y2 - 1));
                  if (y_real >= y1 && y_real < y2)
                    gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                           CLAMP (x_real - CROSSHAIR,
                                                  x1, x2 - 1),
                                           y_real,
                                           CLAMP (x_real + CROSSHAIR,
                                                  x1, x2 - 1),
                                           y_real);
                }
            }
          break;

        case GIMP_GRID_ON_OFF_DASH:
        case GIMP_GRID_DOUBLE_DASH:
        case GIMP_GRID_SOLID:
          gimp_display_shell_transform_xy (shell,
                                           0, 0, &x0, &y0,
                                           FALSE);
          gimp_display_shell_transform_xy (shell,
                                           width, height, &x3, &y3,
                                           FALSE);

          for (x = x_offset; x < width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real >= x1 && x_real < x2)
                gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                       x_real, y0, x_real, y3 - 1);
            }

          for (y = y_offset; y < height; y += grid->yspacing)
            {
              if (y < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               0, y, &x_real, &y_real,
                                               FALSE);

              if (y_real >= y1 && y_real < y2)
                gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                       x0, y_real, x3 - 1, y_real);
            }
          break;
        }

      gimp_canvas_set_custom_gc (canvas, NULL);
    }
}

void
gimp_display_shell_draw_pen (GimpDisplayShell  *shell,
                             const GimpVector2 *points,
                             gint               num_points,
                             GimpContext       *context,
                             GimpActiveColor    color,
                             gint               width)
{
  GimpCanvas  *canvas;
  GdkGC       *gc;
  GdkGCValues  values;
  GdkPoint    *coords;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (num_points == 0 || points != NULL);

  canvas = GIMP_CANVAS (shell->canvas);

  coords = g_new (GdkPoint, MAX (2, num_points));

  gimp_display_shell_transform_points (shell,
                                       points, coords, num_points, FALSE);

  if (num_points == 1)
    {
      coords[1] = coords[0];
      num_points = 2;
    }

  gc = gimp_display_shell_get_pen_gc (shell, context, color);

  values.line_width = MAX (1, width);
  gdk_gc_set_values (gc, &values, GDK_GC_LINE_WIDTH);

  gimp_canvas_set_custom_gc (canvas, gc);

  gimp_canvas_draw_lines (canvas, GIMP_CANVAS_STYLE_CUSTOM, coords, num_points);

  gimp_canvas_set_custom_gc (canvas, NULL);

  g_free (coords);
}

void
gimp_display_shell_draw_sample_point (const GimpDisplayShell *shell,
                                      GimpSamplePoint        *sample_point,
                                      gboolean                active)
{
  GimpCanvasStyle style;
  gdouble         x, y;
  gint            x1, x2;
  gint            y1, y2;
  gint            w, h;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (sample_point != NULL);

  if (sample_point->x < 0)
    return;

  gimp_display_shell_transform_xy_f (shell,
                                     sample_point->x + 0.5,
                                     sample_point->y + 0.5,
                                     &x, &y, FALSE);

  x1 = floor (x - GIMP_SAMPLE_POINT_DRAW_SIZE);
  x2 = ceil  (x + GIMP_SAMPLE_POINT_DRAW_SIZE);
  y1 = floor (y - GIMP_SAMPLE_POINT_DRAW_SIZE);
  y2 = ceil  (y + GIMP_SAMPLE_POINT_DRAW_SIZE);

  gdk_drawable_get_size (shell->canvas->window, &w, &h);

  if (x < - GIMP_SAMPLE_POINT_DRAW_SIZE   ||
      y < - GIMP_SAMPLE_POINT_DRAW_SIZE   ||
      x > w + GIMP_SAMPLE_POINT_DRAW_SIZE ||
      y > h + GIMP_SAMPLE_POINT_DRAW_SIZE)
    return;

  if (active)
    style = GIMP_CANVAS_STYLE_SAMPLE_POINT_ACTIVE;
  else
    style = GIMP_CANVAS_STYLE_SAMPLE_POINT_NORMAL;

#define HALF_SIZE (GIMP_SAMPLE_POINT_DRAW_SIZE / 2)

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), style,
                         x, y1, x, y1 + HALF_SIZE);
  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), style,
                         x, y2 - HALF_SIZE, x, y2);

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), style,
                         x1, y, x1 + HALF_SIZE, y);
  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), style,
                         x2 - HALF_SIZE, y, x2, y);

  gimp_canvas_draw_arc (GIMP_CANVAS (shell->canvas), style,
                        FALSE,
                        x - HALF_SIZE, y - HALF_SIZE,
                        GIMP_SAMPLE_POINT_DRAW_SIZE,
                        GIMP_SAMPLE_POINT_DRAW_SIZE,
                        0, 64 * 270);

  gimp_canvas_draw_text (GIMP_CANVAS (shell->canvas), style,
                         x + 2, y + 2,
                         "%d",
                         g_list_index (gimp_image_get_sample_points (shell->display->image),
                                       sample_point) + 1);
}

void
gimp_display_shell_draw_sample_points (const GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->display->image &&
      gimp_display_shell_get_show_sample_points (shell))
    {
      GList *list;

      for (list = gimp_image_get_sample_points (shell->display->image);
           list;
           list = g_list_next (list))
        {
          gimp_display_shell_draw_sample_point (shell, list->data, FALSE);
        }
    }
}

void
gimp_display_shell_draw_vector (const GimpDisplayShell *shell,
                                GimpVectors            *vectors)
{
  GimpStroke *stroke = NULL;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
    {
      GArray   *coords;
      gboolean  closed;

      coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

      if (coords && coords->len > 0)
        {
          GdkPoint *gdk_coords = g_new (GdkPoint, coords->len);

          gimp_display_shell_transform_coords (shell,
                                               &g_array_index (coords,
                                                               GimpCoords, 0),
                                               gdk_coords,
                                               coords->len,
                                               FALSE);

          gimp_canvas_draw_lines (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR,
                                  gdk_coords, coords->len);

          g_free (gdk_coords);
        }

      if (coords)
        g_array_free (coords, TRUE);
    }
}

void
gimp_display_shell_draw_vectors (const GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->display->image &&
      TRUE /* gimp_display_shell_get_show_vectors (shell) */)
    {
      GList *list;

      for (list = GIMP_LIST (shell->display->image->vectors)->list;
           list;
           list = list->next)
        {
          GimpVectors *vectors = list->data;

          if (gimp_item_get_visible (GIMP_ITEM (vectors)))
            gimp_display_shell_draw_vector (shell, vectors);
        }
    }
}

void
gimp_display_shell_draw_cursor (const GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->have_cursor)
    gimp_canvas_draw_cursor (GIMP_CANVAS (shell->canvas),
                             shell->cursor_x, shell->cursor_y);
}

void
gimp_display_shell_draw_area (const GimpDisplayShell *shell,
                              gint                    x,
                              gint                    y,
                              gint                    w,
                              gint                    h)
{
  gint sx, sy;
  gint sw, sh;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display->image)
    return;

  sx = - shell->offset_x;
  sy = - shell->offset_y;

  gimp_display_shell_draw_get_scaled_image_size (shell, &sw, &sh);

  /*  check if the passed in area intersects with
   *  both the display and the image
   */
  if (gimp_rectangle_intersect (x, y, w, h,
                                0, 0, shell->disp_width,  shell->disp_height,
                                &x, &y, &w, &h)
      &&
      gimp_rectangle_intersect (x, y, w, h,
                                sx, sy, sw, sh,
                                &x, &y, &w, &h))
    {
      GdkRectangle  rect;
      gint          x2, y2;
      gint          i, j;

      x2 = x + w;
      y2 = y + h;

      if (shell->highlight)
        {
          rect.x      = ceil  (shell->scale_x * shell->highlight->x);
          rect.y      = ceil  (shell->scale_y * shell->highlight->y);
          rect.width  = floor (shell->scale_x * shell->highlight->width);
          rect.height = floor (shell->scale_y * shell->highlight->height);
        }

      /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT
       *  sized chunks
       */
      for (i = y; i < y2; i += GIMP_DISPLAY_RENDER_BUF_HEIGHT)
        {
          for (j = x; j < x2; j += GIMP_DISPLAY_RENDER_BUF_WIDTH)
            {
              gint disp_xoffset, disp_yoffset;
              gint dx, dy;

              dx = MIN (x2 - j, GIMP_DISPLAY_RENDER_BUF_WIDTH);
              dy = MIN (y2 - i, GIMP_DISPLAY_RENDER_BUF_HEIGHT);

              gimp_display_shell_scroll_get_disp_offset (shell,
                                                         &disp_xoffset,
                                                         &disp_yoffset);

              gimp_display_shell_render (shell,
                                         j - disp_xoffset,
                                         i - disp_yoffset,
                                         dx, dy,
                                         shell->highlight ? &rect : NULL);
            }
        }
    }
}


/*  private functions  */

static GdkGC *
gimp_display_shell_get_grid_gc (GimpDisplayShell *shell,
                                GimpGrid         *grid)
{
  GdkGCValues  values;
  GdkColor     fg, bg;

  if (shell->grid_gc)
    return shell->grid_gc;

  switch (grid->style)
    {
    case GIMP_GRID_ON_OFF_DASH:
      values.line_style = GDK_LINE_ON_OFF_DASH;
      break;

    case GIMP_GRID_DOUBLE_DASH:
      values.line_style = GDK_LINE_DOUBLE_DASH;
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_INTERSECTIONS:
    case GIMP_GRID_SOLID:
      values.line_style = GDK_LINE_SOLID;
      break;
    }

  values.join_style = GDK_JOIN_MITER;

  shell->grid_gc = gdk_gc_new_with_values (shell->canvas->window,
                                           &values, (GDK_GC_LINE_STYLE |
                                                     GDK_GC_JOIN_STYLE));

  gimp_rgb_get_gdk_color (&grid->fgcolor, &fg);
  gdk_gc_set_rgb_fg_color (shell->grid_gc, &fg);

  gimp_rgb_get_gdk_color (&grid->bgcolor, &bg);
  gdk_gc_set_rgb_bg_color (shell->grid_gc, &bg);

  return shell->grid_gc;
}

static GdkGC *
gimp_display_shell_get_pen_gc (GimpDisplayShell *shell,
                               GimpContext      *context,
                               GimpActiveColor   active)
{
  GdkGCValues  values;
  GimpRGB      rgb;
  GdkColor     color;

  if (shell->pen_gc)
    return shell->pen_gc;

  values.line_style = GDK_LINE_SOLID;
  values.cap_style  = GDK_CAP_ROUND;
  values.join_style = GDK_JOIN_ROUND;

  shell->pen_gc = gdk_gc_new_with_values (shell->canvas->window,
                                          &values, (GDK_GC_LINE_STYLE |
                                                    GDK_GC_CAP_STYLE  |
                                                    GDK_GC_JOIN_STYLE));

  switch (active)
    {
    case GIMP_ACTIVE_COLOR_FOREGROUND:
      gimp_context_get_foreground (context, &rgb);
      break;

    case GIMP_ACTIVE_COLOR_BACKGROUND:
      gimp_context_get_background (context, &rgb);
      break;
    }

  gimp_rgb_get_gdk_color (&rgb, &color);
  gdk_gc_set_rgb_fg_color (shell->pen_gc, &color);

  g_object_add_weak_pointer (G_OBJECT (shell->pen_gc),
                             (gpointer) &shell->pen_gc);

  g_signal_connect_object (context, "notify",
                           G_CALLBACK (g_object_unref),
                           shell->pen_gc, G_CONNECT_SWAPPED);

  return shell->pen_gc;
}
