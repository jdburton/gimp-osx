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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"
#include "core/gimpmarshal.h"

#include "gimpcurveview.h"


enum
{
  PROP_0,
  PROP_BASE_LINE,
  PROP_GRID_ROWS,
  PROP_GRID_COLUMNS
};


static void       gimp_curve_view_finalize       (GObject          *object);
static void       gimp_curve_view_dispose        (GObject          *object);
static void       gimp_curve_view_set_property   (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       gimp_curve_view_get_property   (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static void       gimp_curve_view_style_set      (GtkWidget        *widget,
                                                  GtkStyle         *prev_style);
static gboolean   gimp_curve_view_expose         (GtkWidget        *widget,
                                                  GdkEventExpose   *event);
static gboolean   gimp_curve_view_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean   gimp_curve_view_button_release (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean   gimp_curve_view_motion_notify  (GtkWidget        *widget,
                                                  GdkEventMotion   *bevent);
static gboolean   gimp_curve_view_leave_notify   (GtkWidget        *widget,
                                                  GdkEventCrossing *cevent);
static gboolean   gimp_curve_view_key_press      (GtkWidget        *widget,
                                                  GdkEventKey      *kevent);

static void       gimp_curve_view_set_cursor     (GimpCurveView    *view,
                                                  gdouble           x,
                                                  gdouble           y);
static void       gimp_curve_view_unset_cursor   (GimpCurveView *view);


G_DEFINE_TYPE (GimpCurveView, gimp_curve_view,
               GIMP_TYPE_HISTOGRAM_VIEW)

#define parent_class gimp_curve_view_parent_class


static void
gimp_curve_view_class_init (GimpCurveViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize             = gimp_curve_view_finalize;
  object_class->dispose              = gimp_curve_view_dispose;
  object_class->set_property         = gimp_curve_view_set_property;
  object_class->get_property         = gimp_curve_view_get_property;

  widget_class->style_set            = gimp_curve_view_style_set;
  widget_class->expose_event         = gimp_curve_view_expose;
  widget_class->button_press_event   = gimp_curve_view_button_press;
  widget_class->button_release_event = gimp_curve_view_button_release;
  widget_class->motion_notify_event  = gimp_curve_view_motion_notify;
  widget_class->leave_notify_event   = gimp_curve_view_leave_notify;
  widget_class->key_press_event      = gimp_curve_view_key_press;

  g_object_class_install_property (object_class, PROP_BASE_LINE,
                                   g_param_spec_boolean ("base-line",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_GRID_ROWS,
                                   g_param_spec_int ("grid-rows", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_GRID_COLUMNS,
                                   g_param_spec_int ("grid-columns", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_curve_view_init (GimpCurveView *view)
{
  view->curve       = NULL;
  view->selected    = 0;
  view->last_x      = 0.0;
  view->last_y      = 0.0;
  view->cursor_type = -1;
  view->xpos        = -1.0;
  view->cursor_x    = -1.0;
  view->cursor_y    = -1.0;

  GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);

  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK      |
                         GDK_LEAVE_NOTIFY_MASK);
}

static void
gimp_curve_view_finalize (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  if (view->xpos_layout)
    {
      g_object_unref (view->xpos_layout);
      view->xpos_layout = NULL;
    }

  if (view->cursor_layout)
    {
      g_object_unref (view->cursor_layout);
      view->cursor_layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curve_view_dispose (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  gimp_curve_view_set_curve (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_curve_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GRID_ROWS:
      view->grid_rows = g_value_get_int (value);
      break;
    case PROP_GRID_COLUMNS:
      view->grid_columns = g_value_get_int (value);
      break;
    case PROP_BASE_LINE:
      view->draw_base_line = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GRID_ROWS:
      g_value_set_int (value, view->grid_rows);
      break;
    case PROP_GRID_COLUMNS:
      g_value_set_int (value, view->grid_columns);
      break;
    case PROP_BASE_LINE:
      g_value_set_boolean (value, view->draw_base_line);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (view->xpos_layout)
    {
      g_object_unref (view->xpos_layout);
      view->xpos_layout = NULL;
    }

  if (view->cursor_layout)
    {
      g_object_unref (view->cursor_layout);
      view->cursor_layout = NULL;
    }
}

static void
gimp_curve_view_draw_grid (GimpCurveView *view,
                           GtkStyle      *style,
                           cairo_t       *cr,
                           gint           width,
                           gint           height,
                           gint           border)
{
  gint i;

  gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);

  for (i = 1; i < view->grid_rows; i++)
    {
      gint y = i * height / view->grid_rows;

      if ((view->grid_rows % 2) == 0 && (i == view->grid_rows / 2))
        continue;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  for (i = 1; i < view->grid_columns; i++)
    {
      gint x = i * width / view->grid_columns;

      if ((view->grid_columns % 2) == 0 && (i == view->grid_columns / 2))
        continue;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  if (view->draw_base_line)
    {
      cairo_move_to (cr, border, border + height - 1);
      cairo_line_to (cr, border + width - 1, border);
    }

  cairo_set_line_width (cr, 0.6);
  cairo_stroke (cr);

  if ((view->grid_rows % 2) == 0)
    {
      gint y = height / 2;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  if ((view->grid_columns % 2) == 0)
    {
      gint x = width / 2;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);
}

static void
gimp_curve_view_draw_point (GimpCurveView *view,
                            cairo_t       *cr,
                            gint           i,
                            gint           width,
                            gint           height,
                            gint           border)
{
  gdouble x, y;

  gimp_curve_get_point (view->curve, i, &x, &y);

  if (x < 0.0)
    return;

  y = 1.0 - y;

#define RADIUS 3

  cairo_move_to (cr,
                 border + (gdouble) width  * x + RADIUS,
                 border + (gdouble) height * y);
  cairo_arc (cr,
             border + (gdouble) width  * x,
             border + (gdouble) height * y,
             RADIUS,
             0, 2 * G_PI);
}

static gboolean
gimp_curve_view_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GtkStyle      *style = gtk_widget_get_style (widget);
  cairo_t       *cr;
  gint           border;
  gint           width;
  gint           height;
  gdouble        x, y;
  gint           i;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  if (! view->curve)
    return FALSE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;

  width  = widget->allocation.width  - 2 * border - 1;
  height = widget->allocation.height - 2 * border - 1;

  cr = gdk_cairo_create (widget->window);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  cairo_translate (cr, 0.5, 0.5);

  /*  Draw the grid lines  */
  gimp_curve_view_draw_grid (view, style, cr, width, height, border);

  /*  Draw the curve  */
  gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

  x = 0.0;
  y = 1.0 - gimp_curve_map_value (view->curve, 0.0);

  cairo_move_to (cr,
                 border + (gdouble) width  * x,
                 border + (gdouble) height * y);

  for (i = 1; i < 256; i++)
    {
      x = (gdouble) i / 255.0;
      y = 1.0 - gimp_curve_map_value (view->curve, x);

      cairo_line_to (cr,
                     border + (gdouble) width  * x,
                     border + (gdouble) height * y);
    }

  cairo_stroke (cr);

  if (gimp_curve_get_curve_type (view->curve) == GIMP_CURVE_SMOOTH)
    {
      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

      /*  Draw the unselected points  */
      for (i = 0; i < view->curve->n_points; i++)
        {
          if (i == view->selected)
            continue;

          gimp_curve_view_draw_point (view, cr, i, width, height, border);
        }

      cairo_stroke (cr);

      /*  Draw the selected point  */
      if (view->selected != -1)
        {
          gimp_curve_view_draw_point (view, cr, view->selected,
                                      width, height, border);
          cairo_fill (cr);
       }
    }

  if (view->xpos >= 0.0)
    {
      gint  layout_x;
      gint  layout_y;
      gchar buf[32];

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

      /* draw the color line */
      cairo_move_to (cr,
                     border + ROUND ((gdouble) width * view->xpos),
                     border);
      cairo_line_to (cr,
                     border + ROUND ((gdouble) width * view->xpos),
                     border + height - 1);
      cairo_stroke (cr);

      /* and xpos indicator */
      g_snprintf (buf, sizeof (buf), "x:%d", (gint) (view->xpos * 255.999));

      if (! view->xpos_layout)
        view->xpos_layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->xpos_layout, buf, -1);
      pango_layout_get_pixel_size (view->xpos_layout, &layout_x, &layout_y);

      if (view->xpos < 0.5)
        layout_x = border;
      else
        layout_x = -(layout_x + border);

      cairo_move_to (cr,
                     border + (gdouble) width * view->xpos + layout_x,
                     border + height - border - layout_y);
      pango_cairo_show_layout (cr, view->xpos_layout);
      cairo_fill (cr);
    }

  if (view->cursor_x >= 0.0 && view->cursor_x <= 1.0 &&
      view->cursor_y >= 0.0 && view->cursor_y <= 1.0)
    {
      gchar buf[32];
      gint  w, h;

      if (! view->cursor_layout)
        {
          view->cursor_layout = gtk_widget_create_pango_layout (widget,
                                                                "x:888 y:888");
          pango_layout_get_pixel_extents (view->cursor_layout,
                                          NULL, &view->cursor_rect);
        }

      x = border * 2 + 3;
      y = border * 2 + 3;
      w = view->cursor_rect.width;
      h = view->cursor_rect.height;

      cairo_push_group (cr);

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
      cairo_rectangle (cr, x + 0.5, y + 0.5, w, h);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 6);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      cairo_stroke (cr);

      g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
                  (gint) (view->cursor_x         * 255.999),
                  (gint) ((1.0 - view->cursor_y) * 255.999));
      pango_layout_set_text (view->cursor_layout, buf, -1);

      gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_NORMAL]);

      cairo_move_to (cr, x, y);
      pango_cairo_show_layout (cr, view->cursor_layout);
      cairo_fill (cr);

      cairo_pop_group_to_source (cr);
      cairo_paint_with_alpha (cr, 0.6);
    }

  cairo_destroy (cr);

  return FALSE;
}

static void
set_cursor (GimpCurveView *view,
            GdkCursorType  new_cursor)
{
  if (new_cursor != view->cursor_type)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (view));
      GdkCursor  *cursor  = gdk_cursor_new_for_display (display, new_cursor);

      gdk_window_set_cursor (GTK_WIDGET (view)->window, cursor);
      gdk_cursor_unref (cursor);

      view->cursor_type = new_cursor;
    }
}

static gboolean
gimp_curve_view_button_press (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve = view->curve;
  gint           border;
  gint           width, height;
  gdouble        x;
  gdouble        y;
  gint           closest_point;
  gint           i;

  if (! curve || bevent->button != 1)
    return TRUE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  x = (gdouble) (bevent->x - border) / (gdouble) width;
  y = (gdouble) (bevent->y - border) / (gdouble) height;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  closest_point = gimp_curve_get_closest_point (curve, x);

  view->grabbed = TRUE;

  set_cursor (view, GDK_TCROSS);

  switch (gimp_curve_get_curve_type (curve))
    {
    case GIMP_CURVE_SMOOTH:
      /*  determine the leftmost and rightmost points  */
      view->leftmost = -1.0;
      for (i = closest_point - 1; i >= 0; i--)
        {
          gdouble point_x;

          gimp_curve_get_point (curve, i, &point_x, NULL);

          if (point_x >= 0.0)
            {
              view->leftmost = point_x;
              break;
            }
        }

      view->rightmost = 2.0;
      for (i = closest_point + 1; i < curve->n_points; i++)
        {
          gdouble point_x;

          gimp_curve_get_point (curve, i, &point_x, NULL);

          if (point_x >= 0.0)
            {
              view->rightmost = point_x;
              break;
            }
        }

      gimp_curve_view_set_selected (view, closest_point);

      gimp_curve_set_point (curve, view->selected, x, 1.0 - y);
      break;

    case GIMP_CURVE_FREE:
      view->last_x = x;
      view->last_y = y;

      gimp_curve_set_curve (curve, x, 1.0 - y);
      break;
    }

  if (! GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  return TRUE;
}

static gboolean
gimp_curve_view_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  if (bevent->button != 1)
    return TRUE;

  view->grabbed = FALSE;

  set_cursor (view, GDK_FLEUR);

  return TRUE;
}

static gboolean
gimp_curve_view_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  GimpCurveView  *view       = GIMP_CURVE_VIEW (widget);
  GimpCurve      *curve      = view->curve;
  GimpCursorType  new_cursor = GDK_X_CURSOR;
  gint            border;
  gint            width, height;
  gdouble         x;
  gdouble         y;
  gdouble         point_x;
  gint            closest_point;

  if (! curve)
    return TRUE;

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  x = (gdouble) (mevent->x - border) / (gdouble) width;
  y = (gdouble) (mevent->y - border) / (gdouble) height;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  closest_point = gimp_curve_get_closest_point (curve, x);

  switch (gimp_curve_get_curve_type (curve))
    {
    case GIMP_CURVE_SMOOTH:
      if (! view->grabbed) /*  If no point is grabbed...  */
        {
          gimp_curve_get_point (curve, closest_point, &point_x, NULL);

          if (point_x >= 0.0)
            new_cursor = GDK_FLEUR;
          else
            new_cursor = GDK_TCROSS;
        }
      else /*  Else, drag the grabbed point  */
        {
          new_cursor = GDK_TCROSS;

          gimp_data_freeze (GIMP_DATA (curve));

          gimp_curve_set_point (curve, view->selected, -1.0, -1.0);

          if (x > view->leftmost && x < view->rightmost)
            {
              gint n_points = gimp_curve_get_n_points (curve);

              closest_point = ROUND (x * (gdouble) (n_points - 1));

              gimp_curve_get_point (curve, closest_point, &point_x, NULL);

              if (point_x < 0.0)
                gimp_curve_view_set_selected (view, closest_point);

              gimp_curve_set_point (curve, view->selected, x, 1.0 - y);
            }

          gimp_data_thaw (GIMP_DATA (curve));
        }
      break;

    case GIMP_CURVE_FREE:
      if (view->grabbed)
        {
          gint    n_samples = gimp_curve_get_n_samples (curve);
          gdouble x1, x2;
          gdouble y1, y2;

          if (view->last_x > x)
            {
              x1 = x;
              x2 = view->last_x;
              y1 = y;
              y2 = view->last_y;
            }
          else
            {
              x1 = view->last_x;
              x2 = x;
              y1 = view->last_y;
              y2 = y;
            }

          if (x2 != x1)
            {
              gint from = ROUND (x1 * (gdouble) (n_samples - 1));
              gint to   = ROUND (x2 * (gdouble) (n_samples - 1));
              gint i;

              gimp_data_freeze (GIMP_DATA (curve));

              for (i = from; i <= to; i++)
                {
                  gdouble xpos = (gdouble) i / (gdouble) (n_samples - 1);
                  gdouble ypos = (y1 + ((y2 - y1) * (xpos - x1)) / (x2 - x1));

                  gimp_curve_set_curve (curve, xpos, 1.0 - ypos);
                }

              gimp_data_thaw (GIMP_DATA (curve));
            }
          else
            {
              gimp_curve_set_curve (curve, x, 1.0 - y);
            }

          view->last_x = x;
          view->last_y = y;
        }

      if (mevent->state & GDK_BUTTON1_MASK)
        new_cursor = GDK_TCROSS;
      else
        new_cursor = GDK_PENCIL;

      break;
    }

  set_cursor (view, new_cursor);

  gimp_curve_view_set_cursor (view, x, y);

  return TRUE;
}

static gboolean
gimp_curve_view_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *cevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  gimp_curve_view_unset_cursor (view);

  return TRUE;
}

static gboolean
gimp_curve_view_key_press (GtkWidget   *widget,
                           GdkEventKey *kevent)
{
  GimpCurveView *view   = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve  = view->curve;
  gint           i      = view->selected;
  gdouble        x, y;
  gboolean       retval = FALSE;

  if (view->grabbed || ! curve ||
      gimp_curve_get_curve_type (curve) == GIMP_CURVE_FREE)
    return FALSE;

  gimp_curve_get_point (curve, i, NULL, &y);

  switch (kevent->keyval)
    {
    case GDK_Left:
      for (i = i - 1; i >= 0 && ! retval; i--)
        {
          gimp_curve_get_point (curve, i, &x, NULL);

          if (x >= 0.0)
            {
              gimp_curve_view_set_selected (view, i);

              retval = TRUE;
            }
        }
      break;

    case GDK_Right:
      for (i = i + 1; i < curve->n_points && ! retval; i++)
        {
          gimp_curve_get_point (curve, i, &x, NULL);

          if (x >= 0.0)
            {
              gimp_curve_view_set_selected (view, i);

              retval = TRUE;
            }
        }
      break;

    case GDK_Up:
      if (y < 1.0)
        {
          y = y + (kevent->state & GDK_SHIFT_MASK ?
                   (16.0 / 255.0) : (1.0 / 255.0));

          gimp_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

          retval = TRUE;
        }
      break;

    case GDK_Down:
      if (y > 0)
        {
          y = y - (kevent->state & GDK_SHIFT_MASK ?
                   (16.0 / 255.0) : (1.0 / 255.0));

          gimp_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

          retval = TRUE;
        }
      break;

    default:
      break;
    }

  if (retval)
    set_cursor (view, GDK_TCROSS);

  return retval;
}

GtkWidget *
gimp_curve_view_new (void)
{
  return g_object_new (GIMP_TYPE_CURVE_VIEW, NULL);
}

static void
gimp_curve_view_curve_dirty (GimpCurve     *curve,
                             GimpCurveView *view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_curve (GimpCurveView *view,
                           GimpCurve     *curve)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));
  g_return_if_fail (curve == NULL || GIMP_IS_CURVE (curve));

  if (view->curve == curve)
    return;

  if (view->curve)
    {
      g_signal_handlers_disconnect_by_func (view->curve,
                                            gimp_curve_view_curve_dirty,
                                            view);
      g_object_unref (view->curve);
    }

  view->curve = curve;

  if (view->curve)
    {
      g_object_ref (view->curve);
      g_signal_connect (view->curve, "dirty",
                        G_CALLBACK (gimp_curve_view_curve_dirty),
                        view);
    }
}

GimpCurve *
gimp_curve_view_get_curve (GimpCurveView *view)
{
  g_return_val_if_fail (GIMP_IS_CURVE_VIEW (view), NULL);

  return view->curve;
}

void
gimp_curve_view_set_selected (GimpCurveView *view,
                              gint           selected)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->selected = selected;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_xpos (GimpCurveView *view,
                          gdouble        x)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->xpos = x;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}


/*  private functions  */

static void
gimp_curve_view_set_cursor (GimpCurveView *view,
                            gdouble        x,
                            gdouble        y)
{
  view->cursor_x = x;
  view->cursor_y = y;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
gimp_curve_view_unset_cursor (GimpCurveView *view)
{
  view->cursor_x = -1.0;
  view->cursor_y = -1.0;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}
