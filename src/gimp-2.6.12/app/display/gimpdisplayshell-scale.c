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

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpunit.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-transform.h"


#define SCALE_TIMEOUT             2
#define SCALE_EPSILON             0.0001
#define ALMOST_CENTERED_THRESHOLD 2

#define SCALE_EQUALS(a,b) (fabs ((a) - (b)) < SCALE_EPSILON)


/*  local function prototypes  */

static void      gimp_display_shell_scale_to             (GimpDisplayShell *shell,
                                                          gdouble           scale,
                                                          gint              viewport_x,
                                                          gint              viewport_y);

static gboolean  gimp_display_shell_scale_image_starts_to_fit
                                                         (GimpDisplayShell *shell,
                                                          gdouble           new_scale,
                                                          gdouble           current_scale,
                                                          gboolean         *vertically,
                                                          gboolean         *horizontally);
static gboolean  gimp_display_shell_scale_viewport_coord_almost_centered
                                                         (GimpDisplayShell *shell,
                                                          gint              x,
                                                          gint              y,
                                                          gboolean         *horizontally,
                                                          gboolean         *vertically);

static void      gimp_display_shell_scale_get_image_center_viewport
                                                         (GimpDisplayShell *shell,
                                                          gint             *image_center_x,
                                                          gint             *image_center_y);


static void      gimp_display_shell_scale_get_zoom_focus (GimpDisplayShell *shell,
                                                          gdouble           new_scale,
                                                          gdouble           current_scale,
                                                          gint             *x,
                                                          gint             *y,
                                                          GimpZoomFocus     zoom_focus);

static gdouble   img2real                                (GimpDisplayShell *shell,
                                                          gboolean          xdir,
                                                          gdouble           a);


/*  public functions  */

/**
 * gimp_display_shell_update_scrollbars_and_rulers:
 * @shell: the #GimpDisplayShell
 *
 **/
void
gimp_display_shell_update_scrollbars_and_rulers (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_update_scrollbars (shell);
  gimp_display_shell_scale_update_rulers (shell);
}

/**
 * gimp_display_shell_scale_update_scrollbars:
 * @shell:
 *
 **/
void
gimp_display_shell_scale_update_scrollbars (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;

  if (! shell->display)
    return;

  image = shell->display->image;

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
    }
  else
    {
      image_width  = shell->disp_width;
      image_height = shell->disp_height;
    }


  /* Horizontal scrollbar */

  shell->hsbdata->value          = shell->offset_x;
  shell->hsbdata->page_size      = shell->disp_width;
  shell->hsbdata->page_increment = shell->disp_width / 2;

  gimp_display_shell_scroll_setup_hscrollbar (shell, shell->offset_x);

  gtk_adjustment_changed (shell->hsbdata);


  /* Vertcal scrollbar */

  shell->vsbdata->value          = shell->offset_y;
  shell->vsbdata->page_size      = shell->disp_height;
  shell->vsbdata->page_increment = shell->disp_height / 2;

  gimp_display_shell_scroll_setup_vscrollbar (shell, shell->offset_y);

  gtk_adjustment_changed (shell->vsbdata);
}

/**
 * gimp_display_shell_scale_update_rulers:
 * @shell:
 *
 **/
void
gimp_display_shell_scale_update_rulers (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    horizontal_lower;
  gdouble    horizontal_upper;
  gdouble    horizontal_max_size;
  gdouble    vertical_lower;
  gdouble    vertical_upper;
  gdouble    vertical_max_size;

  if (! shell->display)
    return;

  image = shell->display->image;

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
    }
  else
    {
      image_width  = shell->disp_width;
      image_height = shell->disp_height;
    }


  /* Initialize values */

  horizontal_lower = 0;
  vertical_lower   = 0;

  if (image)
    {
      horizontal_upper    = img2real (shell, TRUE,
                                      FUNSCALEX (shell, shell->disp_width));
      horizontal_max_size = img2real (shell, TRUE,
                                      MAX (image_width, image_height));

      vertical_upper      = img2real (shell, FALSE,
                                      FUNSCALEY (shell, shell->disp_height));
      vertical_max_size   = img2real (shell, FALSE,
                                      MAX (image_width, image_height));
    }
  else
    {
      horizontal_upper    = image_width;
      horizontal_max_size = MAX (image_width, image_height);

      vertical_upper      = image_height;
      vertical_max_size   = MAX (image_width, image_height);
    }


  /* Adjust due to scrolling */

  if (image)
    {
      horizontal_lower += img2real (shell, TRUE,
                                    FUNSCALEX (shell,
                                               (gdouble) shell->offset_x));
      horizontal_upper += img2real (shell, TRUE,
                                    FUNSCALEX (shell,
                                               (gdouble) shell->offset_x));

      vertical_lower   += img2real (shell, FALSE,
                                    FUNSCALEY (shell,
                                               (gdouble) shell->offset_y));
      vertical_upper   += img2real (shell, FALSE,
                                    FUNSCALEY (shell,
                                               (gdouble) shell->offset_y));
    }

  /* Finally setup the actual rulers */

  gimp_ruler_set_range (GIMP_RULER (shell->hrule),
                        horizontal_lower,
                        horizontal_upper,
                        horizontal_max_size);

  gimp_ruler_set_unit  (GIMP_RULER (shell->hrule),
                        shell->unit);

  gimp_ruler_set_range (GIMP_RULER (shell->vrule),
                        vertical_lower,
                        vertical_upper,
                        vertical_max_size);

  gimp_ruler_set_unit  (GIMP_RULER (shell->vrule),
                        shell->unit);
}

/**
 * gimp_display_shell_scale_revert:
 * @shell:     the #GimpDisplayShell
 *
 * Reverts the display to the previously used scale. If no previous
 * scale exist, then the call does nothing.
 *
 * Return value: %TRUE if the scale was reverted, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  /* don't bother if no scale has been set */
  if (shell->last_scale < SCALE_EPSILON)
    return FALSE;

  shell->last_scale_time = 0;

  gimp_display_shell_scale_by_values (shell,
                                      shell->last_scale,
                                      shell->last_offset_x,
                                      shell->last_offset_y,
                                      FALSE);   /* don't resize the window */

  return TRUE;
}

/**
 * gimp_display_shell_scale_can_revert:
 * @shell: the #GimpDisplayShell
 *
 * Return value: %TRUE if a previous display scale exists, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_can_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return (shell->last_scale > SCALE_EPSILON);
}

/**
 * gimp_display_shell_scale_set_dot_for_dot:
 * @shell:        the #GimpDisplayShell
 * @dot_for_dot:  whether "Dot for Dot" should be enabled
 *
 * If @dot_for_dot is set to %TRUE then the "Dot for Dot" mode (where image and
 * screen pixels are of the same size) is activated. Dually, the mode is
 * disabled if @dot_for_dot is %FALSE.
 **/
void
gimp_display_shell_scale_set_dot_for_dot (GimpDisplayShell *shell,
                                          gboolean          dot_for_dot)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (dot_for_dot != shell->dot_for_dot)
    {
      /* freeze the active tool */
      gimp_display_shell_pause (shell);

      shell->dot_for_dot = dot_for_dot;

      gimp_display_shell_scale_changed (shell);

      gimp_display_shell_scale_resize (shell,
                                       shell->display->config->resize_windows_on_zoom,
                                       FALSE);

      /* re-enable the active tool */
      gimp_display_shell_resume (shell);
    }
}

/**
 * gimp_display_shell_scale:
 * @shell:     the #GimpDisplayShell
 * @zoom_type: whether to zoom in, our or to a specific scale
 * @scale:     ignored unless @zoom_type == %GIMP_ZOOM_TO
 *
 * This function figures out the context of the zoom and behaves
 * appropriatley thereafter.
 *
 **/
void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type,
                          gdouble           new_scale,
                          GimpZoomFocus     zoom_focus)
{
  gint    x, y;
  gdouble current_scale;
  gdouble real_new_scale;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->canvas != NULL);

  current_scale = gimp_zoom_model_get_factor (shell->zoom);

  if (zoom_type != GIMP_ZOOM_TO)
    {
      real_new_scale = gimp_zoom_model_zoom_step (zoom_type, current_scale);
    }
  else
    {
      real_new_scale = new_scale;
    }

  if (! SCALE_EQUALS (real_new_scale, current_scale))
    {
      if (shell->display->config->resize_windows_on_zoom)
        {
          /* If the window is resized on zoom, simply do the zoom and
           * get things rolling
           */
          gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, real_new_scale);
          gimp_display_shell_scaled (shell);

          gimp_display_shell_shrink_wrap (shell, FALSE);
        }
      else
        {
          gboolean starts_fitting_horiz;
          gboolean starts_fitting_vert;
          gboolean zoom_focus_almost_centered_horiz;
          gboolean zoom_focus_almost_centered_vert;
          gboolean image_center_almost_centered_horiz;
          gboolean image_center_almost_centered_vert;
          gint     image_center_x;
          gint     image_center_y;

          gimp_display_shell_scale_get_zoom_focus (shell,
                                                   real_new_scale,
                                                   current_scale,
                                                   &x,
                                                   &y,
                                                   zoom_focus);
          gimp_display_shell_scale_get_image_center_viewport (shell,
                                                              &image_center_x,
                                                              &image_center_y);

          gimp_display_shell_scale_to (shell, real_new_scale, x, y);


          /* If an image axis started to fit due to zooming out or if
           * the focus point is as good as in the center, center on
           * that axis
           */
          gimp_display_shell_scale_image_starts_to_fit (shell,
                                                        real_new_scale,
                                                        current_scale,
                                                        &starts_fitting_horiz,
                                                        &starts_fitting_vert);

          gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                   x,
                                                                   y,
                                                                   &zoom_focus_almost_centered_horiz,
                                                                   &zoom_focus_almost_centered_vert);
          gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                   image_center_x,
                                                                   image_center_y,
                                                                   &image_center_almost_centered_horiz,
                                                                   &image_center_almost_centered_vert);
            
          gimp_display_shell_scroll_center_image (shell,
                                                  starts_fitting_horiz ||
                                                  (zoom_focus_almost_centered_horiz &&
                                                   image_center_almost_centered_horiz),
                                                  starts_fitting_vert ||
                                                  (zoom_focus_almost_centered_vert &&
                                                   image_center_almost_centered_vert));
        }
    }
}

/**
 * gimp_display_shell_scale_fit_in:
 * @shell: the #GimpDisplayShell
 *
 * Sets the scale such that the entire image precisely fits in the display
 * area.
 **/
void
gimp_display_shell_scale_fit_in (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    xres;
  gdouble    yres;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  gimp_image_get_resolution (image, &xres, &yres);

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width  * shell->monitor_xres / xres);
      image_height = ROUND (image_height * shell->monitor_yres / yres);
    }

  zoom_factor = MIN ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell,
                            GIMP_ZOOM_TO,
                            zoom_factor,
                            GIMP_ZOOM_FOCUS_BEST_GUESS);

  gimp_display_shell_scroll_center_image (shell, TRUE, TRUE);
}

/**
 * gimp_display_shell_scale_image_is_within_viewport:
 * @shell:
 *
 * Returns: %TRUE if the (scaled) image is smaller than and within the
 *          viewport.
 **/
gboolean
gimp_display_shell_scale_image_is_within_viewport (GimpDisplayShell *shell,
                                                   gboolean         *horizontally,
                                                   gboolean         *vertically)
{
  gint     sw, sh;
  gboolean horizontally_dummy, vertically_dummy;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  if (! horizontally) horizontally = &horizontally_dummy;
  if (! vertically)   vertically   = &vertically_dummy;

  gimp_display_shell_draw_get_scaled_image_size (shell, &sw, &sh);

  *horizontally = sw              <= shell->disp_width       &&
                  shell->offset_x <= 0                       &&
                  shell->offset_x >= sw - shell->disp_width;

  *vertically   = sh              <= shell->disp_height      &&
                  shell->offset_y <= 0                       &&
                  shell->offset_y >= sh - shell->disp_height;

  return *vertically && *horizontally;
}

/**
 * gimp_display_shell_scale_fill:
 * @shell: the #GimpDisplayShell
 *
 * Sets the scale such that the entire display area is precisely filled by the
 * image.
 **/
void
gimp_display_shell_scale_fill (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    xres;
  gdouble    yres;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  gimp_image_get_resolution (image, &xres, &yres);

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width  * shell->monitor_xres / xres);
      image_height = ROUND (image_height * shell->monitor_yres / yres);
    }

  zoom_factor = MAX ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell,
                            GIMP_ZOOM_TO,
                            zoom_factor,
                            GIMP_ZOOM_FOCUS_BEST_GUESS);

  gimp_display_shell_scroll_center_image (shell, TRUE, TRUE);
}

/**
 * gimp_display_shell_scale_handle_zoom_revert:
 * @shell:
 *
 * Handle the updating of the Revert Zoom variables.
 **/
void
gimp_display_shell_scale_handle_zoom_revert (GimpDisplayShell *shell)
{
  guint now;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  now = time (NULL);

  if (now - shell->last_scale_time >= SCALE_TIMEOUT)
    {
      shell->last_scale    = gimp_zoom_model_get_factor (shell->zoom);
      shell->last_offset_x = shell->offset_x;
      shell->last_offset_y = shell->offset_y;
    }

  shell->last_scale_time = now;
}

/**
 * gimp_display_shell_scale_by_values:
 * @shell:         the #GimpDisplayShell
 * @scale:         the new scale
 * @offset_x:      the new X offset
 * @offset_y:      the new Y offset
 * @resize_window: whether the display window should be resized
 *
 * Directly sets the image scale and image offsets used by the display. If
 * @resize_window is %TRUE then the display window is resized to better
 * accomodate the image, see gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_by_values (GimpDisplayShell *shell,
                                    gdouble           scale,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gboolean          resize_window)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /*  Abort early if the values are all setup already. We don't
   *  want to inadvertently resize the window (bug #164281).
   */
  if (SCALE_EQUALS (gimp_zoom_model_get_factor (shell->zoom), scale) &&
      shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  gimp_display_shell_scale_handle_zoom_revert (shell);

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  gimp_display_shell_scale_resize (shell, resize_window, FALSE);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_scale_shrink_wrap:
 * @shell: the #GimpDisplayShell
 *
 * Convenience function with the same functionality as
 * gimp_display_shell_scale_resize(@shell, TRUE, grow_only).
 **/
void
gimp_display_shell_scale_shrink_wrap (GimpDisplayShell *shell,
                                      gboolean          grow_only)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_resize (shell, TRUE, grow_only);
}

/**
 * gimp_display_shell_scale_resize:
 * @shell:          the #GimpDisplayShell
 * @resize_window:  whether the display window should be resized
 * @grow_only:      whether shrinking of the window is allowed or not
 *
 * Function commonly called after a change in display scale to make the changes
 * visible to the user. If @resize_window is %TRUE then the display window is
 * resized to accomodate the display image as per
 * gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_resize (GimpDisplayShell *shell,
                                 gboolean          resize_window,
                                 gboolean          grow_only)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  if (resize_window)
    gimp_display_shell_shrink_wrap (shell, grow_only);

  gimp_display_shell_scroll_clamp_and_update (shell);
  gimp_display_shell_scaled (shell);

  gimp_display_shell_expose_full (shell);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_calculate_scale_x_and_y:
 * @shell:
 * @scale:
 * @scale_x:
 * @scale_y:
 *
 **/
void
gimp_display_shell_calculate_scale_x_and_y (const GimpDisplayShell *shell,
                                            gdouble                 scale,
                                            gdouble                *scale_x,
                                            gdouble                *scale_y)
{
  gdouble xres;
  gdouble yres;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_IMAGE (shell->display->image));

  gimp_image_get_resolution (shell->display->image, &xres, &yres);

  if (scale_x) *scale_x = scale * SCREEN_XRES (shell) / xres;
  if (scale_y) *scale_y = scale * SCREEN_YRES (shell) / yres;
}

void
gimp_display_shell_set_initial_scale (GimpDisplayShell *shell,
                                      gdouble           scale,
                                      gint             *display_width,
                                      gint             *display_height)
{
  GdkScreen *screen;
  gint       image_width;
  gint       image_height;
  gint       shell_width;
  gint       shell_height;
  gint       screen_width;
  gint       screen_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  screen = gtk_widget_get_screen (GTK_WIDGET (shell));

  image_width  = gimp_image_get_width  (shell->display->image);
  image_height = gimp_image_get_height (shell->display->image);

  screen_width  = gdk_screen_get_width (screen)  * 0.75;
  screen_height = gdk_screen_get_height (screen) * 0.75;

  /* We need to zoom before we use SCALE[XY] */
  gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

  shell_width  = SCALEX (shell, image_width);
  shell_height = SCALEY (shell, image_height);

  if (shell->display->config->initial_zoom_to_fit)
    {
      /*  Limit to the size of the screen...  */
      if (shell_width > screen_width || shell_height > screen_height)
        {
          gdouble new_scale;
          gdouble current = gimp_zoom_model_get_factor (shell->zoom);

          new_scale = current * MIN (((gdouble) screen_height) / shell_height,
                                     ((gdouble) screen_width)  / shell_width);

          new_scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_OUT, new_scale);

          /*  Since zooming out might skip a zoom step we zoom in
           *  again and test if we are small enough.
           */
          gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO,
                                gimp_zoom_model_zoom_step (GIMP_ZOOM_IN,
                                                           new_scale));

          if (SCALEX (shell, image_width) > screen_width ||
              SCALEY (shell, image_height) > screen_height)
            gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, new_scale);

          shell_width  = SCALEX (shell, image_width);
          shell_height = SCALEY (shell, image_height);
        }
    }
  else
    {
      /*  Set up size like above, but do not zoom to fit. Useful when
       *  working on large images.
       */
      if (shell_width > screen_width)
        shell_width = screen_width;

      if (shell_height > screen_height)
        shell_height = screen_height;
    }

  if (display_width)
    *display_width = shell_width;

  if (display_height)
    *display_height = shell_height;
}

/**
 * gimp_display_shell_scale_to:
 * @shell:
 * @scale:
 * @viewport_x:
 * @viewport_y:
 *
 * Zooms. The display offsets are adjusted so that the point specified
 * by @x and @y doesn't change it's position on screen.
 **/
static void
gimp_display_shell_scale_to (GimpDisplayShell *shell,
                             gdouble           scale,
                             gint              viewport_x,
                             gint              viewport_y)
{
  gdouble scale_x, scale_y;
  gdouble image_focus_x, image_focus_y;
  gint    target_offset_x, target_offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  gimp_display_shell_untransform_xy_f (shell,
                                       viewport_x,
                                       viewport_y,
                                       &image_focus_x,
                                       &image_focus_y,
                                       FALSE);

  gimp_display_shell_calculate_scale_x_and_y (shell, scale, &scale_x, &scale_y);

  target_offset_x = scale_x * image_focus_x - viewport_x;
  target_offset_y = scale_y * image_focus_y - viewport_y;

  /* Note that we never come here if we need to
   * resize_windows_on_zoom
   */
  gimp_display_shell_scale_by_values (shell,
                                      scale,
                                      target_offset_x,
                                      target_offset_y,
                                      FALSE);
}

static gboolean
gimp_display_shell_scale_image_starts_to_fit (GimpDisplayShell *shell,
                                              gdouble           new_scale,
                                              gdouble           current_scale,
                                              gboolean         *vertically,
                                              gboolean         *horizontally)
{
  gboolean vertically_dummy;
  gboolean horizontally_dummy;

  if (! vertically)   vertically   = &vertically_dummy;
  if (! horizontally) horizontally = &horizontally_dummy;

  /* The image can only start to fit if we zoom out */
  if (new_scale > current_scale)
    {
      *vertically   = FALSE;
      *horizontally = FALSE;
    }
  else
    {
      gint current_scale_width;
      gint current_scale_height;
      gint new_scale_width;
      gint new_scale_height;

      gimp_display_shell_draw_get_scaled_image_size_for_scale (shell,
                                                               current_scale,
                                                               &current_scale_width,
                                                               &current_scale_height);

      gimp_display_shell_draw_get_scaled_image_size_for_scale (shell,
                                                               new_scale,
                                                               &new_scale_width,
                                                               &new_scale_height);

      *vertically   = (current_scale_width  >  shell->disp_width &&
                       new_scale_width      <= shell->disp_width);
      *horizontally = (current_scale_height >  shell->disp_height &&
                       new_scale_height     <= shell->disp_height);
    }

  return *vertically && *horizontally;
}

static gboolean
gimp_display_shell_scale_image_stops_to_fit (GimpDisplayShell *shell,
                                             gdouble           new_scale,
                                             gdouble           current_scale,
                                             gboolean         *vertically,
                                             gboolean         *horizontally)
{
  return gimp_display_shell_scale_image_starts_to_fit (shell,
                                                       current_scale,
                                                       new_scale,
                                                       vertically,
                                                       horizontally);
}

/**
 * gimp_display_shell_scale_viewport_coord_almost_centered:
 * @shell:
 * @x:
 * @y:
 * @horizontally:
 * @vertically:
 *
 **/
static gboolean
gimp_display_shell_scale_viewport_coord_almost_centered (GimpDisplayShell *shell,
                                                         gint              x,
                                                         gint              y,
                                                         gboolean         *horizontally,
                                                         gboolean         *vertically)
{
  gboolean local_horizontally;
  gboolean local_vertically;
  gint     center_x = shell->disp_width  / 2;
  gint     center_y = shell->disp_height / 2;

  local_horizontally = (x > center_x - ALMOST_CENTERED_THRESHOLD &&
                        x < center_x + ALMOST_CENTERED_THRESHOLD);

  local_vertically   = (y > center_y - ALMOST_CENTERED_THRESHOLD &&
                        y < center_y + ALMOST_CENTERED_THRESHOLD);

  if (horizontally) *horizontally = local_horizontally;
  if (vertically)   *vertically   = local_vertically;

  return local_horizontally && local_vertically;
}

static void
gimp_display_shell_scale_get_image_center_viewport (GimpDisplayShell *shell,
                                                    gint             *image_center_x,
                                                    gint             *image_center_y)
{
  gint sw, sh;

  gimp_display_shell_draw_get_scaled_image_size (shell,
                                                 &sw,
                                                 &sh);

  if (image_center_x) *image_center_x = -shell->offset_x + sw / 2;
  if (image_center_y) *image_center_y = -shell->offset_y + sh / 2;
}

/**
 * gimp_display_shell_scale_get_zoom_focus:
 * @shell:
 * @new_scale:
 * @x:
 * @y:
 *
 * Calculates the viewport coordinate to focus on when zooming
 * independently for each axis.
 **/
static void
gimp_display_shell_scale_get_zoom_focus (GimpDisplayShell *shell,
                                         gdouble           new_scale,
                                         gdouble           current_scale,
                                         gint             *x,
                                         gint             *y,
                                         GimpZoomFocus     zoom_focus)
{
  GimpZoomFocus real_zoom_focus = zoom_focus;
  gint          image_center_x, image_center_y;
  gint          other_x, other_y;

  /* Calculate stops-to-fit focus point */
  gimp_display_shell_scale_get_image_center_viewport (shell,
                                                      &image_center_x,
                                                      &image_center_y);

  /* Calculate other focus point */
  {
    GdkEvent *event;
    gboolean  event_looks_sane;
    gboolean  cursor_within_canvas;
    gint      canvas_pointer_x, canvas_pointer_y;

    /*  Center on the mouse position instead of the display center if
     *  one of the following conditions are fulfilled and pointer is
     *  within the canvas:
     *
     *   (1) there's no current event (the action was triggered by an
     *       input controller)
     *   (2) the event originates from the canvas (a scroll event)
     *   (3) the event originates from the shell (a key press event)
     *
     *  Basically the only situation where we don't want to center on
     *  mouse position is if the action is being called from a menu.
     */

    event = gtk_get_current_event ();

    event_looks_sane = ! event ||
                       gtk_get_event_widget (event) == shell->canvas ||
                       gtk_get_event_widget (event) == GTK_WIDGET (shell);


    gtk_widget_get_pointer (shell->canvas,
                            &canvas_pointer_x,
                            &canvas_pointer_y);

    cursor_within_canvas = canvas_pointer_x >= 0 &&
                           canvas_pointer_y >= 0 &&
                           canvas_pointer_x <  shell->disp_width &&
                           canvas_pointer_y <  shell->disp_height;


    if (event_looks_sane && cursor_within_canvas)
      {
        other_x = canvas_pointer_x;
        other_y = canvas_pointer_y;
      }
    else
      {
        other_x = shell->disp_width  / 2;
        other_y = shell->disp_height / 2;
      }
  }

  /* Decide which one to use for each axis */
  if (zoom_focus == GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS)
    {
      gboolean centered;

      centered = gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                          image_center_x,
                                                                          image_center_y,
                                                                          NULL,
                                                                          NULL);
      real_zoom_focus = (centered ?
                         GIMP_ZOOM_FOCUS_IMAGE_CENTER :
                         GIMP_ZOOM_FOCUS_BEST_GUESS);
    }
  else
    {
      real_zoom_focus = zoom_focus;
    }

  switch (real_zoom_focus)
    {
    case GIMP_ZOOM_FOCUS_POINTER:
      *x = other_x;
      *y = other_y;
      break;

    case GIMP_ZOOM_FOCUS_IMAGE_CENTER:
      *x = image_center_x;
      *y = image_center_y;
      break;

    case GIMP_ZOOM_FOCUS_BEST_GUESS:
    default:
      {
        gboolean within_horizontally, within_vertically;
        gboolean stops_horizontally, stops_vertically;

        gimp_display_shell_scale_image_is_within_viewport (shell,
                                                           &within_horizontally,
                                                           &within_vertically);

        gimp_display_shell_scale_image_stops_to_fit (shell,
                                                     new_scale,
                                                     current_scale,
                                                     &stops_horizontally,
                                                     &stops_vertically);

        *x = within_horizontally && ! stops_horizontally ? image_center_x : other_x;
        *y = within_vertically   && ! stops_vertically   ? image_center_y : other_y;
      }
      break;
    }
}

/* scale image coord to realworld units (cm, inches, pixels)
 *
 * 27/Feb/1999 I tried inlining this, but the result was slightly
 * slower (poorer cache locality, probably) -- austin
 */
static gdouble
img2real (GimpDisplayShell *shell,
          gboolean          xdir,
          gdouble           len)
{
  gdouble xres;
  gdouble yres;
  gdouble res;

  if (shell->unit == GIMP_UNIT_PIXEL)
    return len;

  gimp_image_get_resolution (shell->display->image, &xres, &yres);

  if (xdir)
    res = xres;
  else
    res = yres;

  return len * _gimp_unit_get_factor (shell->display->gimp, shell->unit) / res;
}
