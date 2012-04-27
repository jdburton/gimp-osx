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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayoptions.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-icon.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"


/*  local function prototypes  */

static void   gimp_display_shell_clean_dirty_handler        (GimpImage        *image,
                                                             GimpDirtyMask     dirty_mask,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_undo_event_handler         (GimpImage        *image,
                                                             GimpUndoEvent     event,
                                                             GimpUndo         *undo,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_grid_notify_handler        (GimpGrid         *grid,
                                                             GParamSpec       *pspec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_name_changed_handler       (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_selection_control_handler  (GimpImage        *image,
                                                             GimpSelectionControl control,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_size_changed_detailed_handler
                                                            (GimpImage        *image,
                                                             gint              previous_origin_x,
                                                             gint              previous_origin_y,
                                                             gint              previous_width,
                                                             gint              previous_height,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_resolution_changed_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_quick_mask_changed_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_update_guide_handler       (GimpImage        *image,
                                                             GimpGuide        *guide,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_update_sample_point_handler(GimpImage        *image,
                                                             GimpSamplePoint  *sample_point,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_invalidate_preview_handler (GimpImage        *image,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_profile_changed_handler    (GimpColorManaged *image,
                                                             GimpDisplayShell *shell);

static void   gimp_display_shell_vectors_freeze_handler     (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_thaw_handler       (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_visible_handler    (GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_add_handler        (GimpContainer    *container,
                                                             GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_vectors_remove_handler     (GimpContainer    *container,
                                                             GimpVectors      *vectors,
                                                             GimpDisplayShell *shell);

static void   gimp_display_shell_check_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_title_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_nav_size_notify_handler    (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_monitor_res_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_padding_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_ants_speed_notify_handler  (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_quality_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             GimpDisplayShell *shell);


/*  public functions  */

void
gimp_display_shell_connect (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (GIMP_IS_IMAGE (shell->display->image));

  image = shell->display->image;

  g_signal_connect (image, "clean",
                    G_CALLBACK (gimp_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "dirty",
                    G_CALLBACK (gimp_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "undo-event",
                    G_CALLBACK (gimp_display_shell_undo_event_handler),
                    shell);
  g_signal_connect (image->grid, "notify",
                    G_CALLBACK (gimp_display_shell_grid_notify_handler),
                    shell);
  g_signal_connect (image, "name-changed",
                    G_CALLBACK (gimp_display_shell_name_changed_handler),
                    shell);
  g_signal_connect (image, "selection-control",
                    G_CALLBACK (gimp_display_shell_selection_control_handler),
                    shell);
  g_signal_connect (image, "size-changed-detailed",
                    G_CALLBACK (gimp_display_shell_size_changed_detailed_handler),
                    shell);
  g_signal_connect (image, "resolution-changed",
                    G_CALLBACK (gimp_display_shell_resolution_changed_handler),
                    shell);
  g_signal_connect (image, "quick-mask-changed",
                    G_CALLBACK (gimp_display_shell_quick_mask_changed_handler),
                    shell);
  g_signal_connect (image, "update-guide",
                    G_CALLBACK (gimp_display_shell_update_guide_handler),
                    shell);
  g_signal_connect (image, "update-sample-point",
                    G_CALLBACK (gimp_display_shell_update_sample_point_handler),
                    shell);
  g_signal_connect (image, "invalidate-preview",
                    G_CALLBACK (gimp_display_shell_invalidate_preview_handler),
                    shell);
  g_signal_connect (image, "profile-changed",
                    G_CALLBACK (gimp_display_shell_profile_changed_handler),
                    shell);

  shell->vectors_freeze_handler =
    gimp_container_add_handler (image->vectors, "freeze",
                                G_CALLBACK (gimp_display_shell_vectors_freeze_handler),
                                shell);
  shell->vectors_thaw_handler =
    gimp_container_add_handler (image->vectors, "thaw",
                                G_CALLBACK (gimp_display_shell_vectors_thaw_handler),
                                shell);
  shell->vectors_visible_handler =
    gimp_container_add_handler (image->vectors, "visibility-changed",
                                G_CALLBACK (gimp_display_shell_vectors_visible_handler),
                                shell);

  g_signal_connect (image->vectors, "add",
                    G_CALLBACK (gimp_display_shell_vectors_add_handler),
                    shell);
  g_signal_connect (image->vectors, "remove",
                    G_CALLBACK (gimp_display_shell_vectors_remove_handler),
                    shell);

  g_signal_connect (shell->display->config,
                    "notify::transparency-size",
                    G_CALLBACK (gimp_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::transparency-type",
                    G_CALLBACK (gimp_display_shell_check_notify_handler),
                    shell);

  g_signal_connect (shell->display->config,
                    "notify::image-title-format",
                    G_CALLBACK (gimp_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::image-status-format",
                    G_CALLBACK (gimp_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::navigation-preview-size",
                    G_CALLBACK (gimp_display_shell_nav_size_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::monitor-resolution-from-windowing-system",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::monitor-xresolution",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (shell->display->config,
                    "notify::monitor-yresolution",
                    G_CALLBACK (gimp_display_shell_monitor_res_notify_handler),
                    shell);

  g_signal_connect (shell->display->config->default_view,
                    "notify::padding-mode",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (shell->display->config->default_view,
                    "notify::padding-color",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (shell->display->config->default_fullscreen_view,
                    "notify::padding-mode",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (shell->display->config->default_fullscreen_view,
                    "notify::padding-color",
                    G_CALLBACK (gimp_display_shell_padding_notify_handler),
                    shell);

  g_signal_connect (shell->display->config,
                    "notify::marching-ants-speed",
                    G_CALLBACK (gimp_display_shell_ants_speed_notify_handler),
                    shell);

  g_signal_connect (shell->display->config,
                    "notify::zoom-quality",
                    G_CALLBACK (gimp_display_shell_quality_notify_handler),
                    shell);

  gimp_display_shell_invalidate_preview_handler (image, shell);
  gimp_display_shell_quick_mask_changed_handler (image, shell);
}

void
gimp_display_shell_disconnect (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (GIMP_IS_IMAGE (shell->display->image));

  image = shell->display->image;

  gimp_display_shell_icon_idle_stop (shell);

  if (shell->grid_gc)
    {
      g_object_unref (shell->grid_gc);
      shell->grid_gc = NULL;
    }

  if (shell->pen_gc)
    {
      g_object_unref (shell->pen_gc);
      shell->pen_gc = NULL;
    }

  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_quality_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_ants_speed_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config->default_fullscreen_view,
                                        gimp_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config->default_view,
                                        gimp_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_monitor_res_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_nav_size_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_title_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (shell->display->config,
                                        gimp_display_shell_check_notify_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (image->vectors,
                                        gimp_display_shell_vectors_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image->vectors,
                                        gimp_display_shell_vectors_add_handler,
                                        shell);

  gimp_container_remove_handler (image->vectors,
                                 shell->vectors_visible_handler);
  gimp_container_remove_handler (image->vectors,
                                 shell->vectors_thaw_handler);
  gimp_container_remove_handler (image->vectors,
                                 shell->vectors_freeze_handler);

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_profile_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_invalidate_preview_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_update_guide_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_update_sample_point_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_quick_mask_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_resolution_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_size_changed_detailed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_selection_control_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_name_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image->grid,
                                        gimp_display_shell_grid_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_undo_event_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        gimp_display_shell_clean_dirty_handler,
                                        shell);
}


/*  private functions  */

static void
gimp_display_shell_clean_dirty_handler (GimpImage        *image,
                                        GimpDirtyMask     dirty_mask,
                                        GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_undo_event_handler (GimpImage        *image,
                                       GimpUndoEvent     event,
                                       GimpUndo         *undo,
                                       GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_grid_notify_handler (GimpGrid         *grid,
                                        GParamSpec       *pspec,
                                        GimpDisplayShell *shell)
{
  if (shell->grid_gc)
    {
      g_object_unref (shell->grid_gc);
      shell->grid_gc = NULL;
    }

  gimp_display_shell_expose_full (shell);

  /* update item factory */
  gimp_display_flush (shell->display);
}

static void
gimp_display_shell_name_changed_handler (GimpImage        *image,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_selection_control_handler (GimpImage            *image,
                                              GimpSelectionControl  control,
                                              GimpDisplayShell     *shell)
{
  gimp_display_shell_selection_control (shell, control);
}

static void
gimp_display_shell_resolution_changed_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  gimp_display_shell_scale_changed (shell);

  if (shell->dot_for_dot)
    {
      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          gimp_display_shell_scale_update_rulers (shell);
        }

      gimp_display_shell_scaled (shell);
    }
  else
    {
      /* A resolution change has the same effect as a size change from
       * a display shell point of view. Force a redraw of the display
       * so that we don't get any display garbage.
       */
      gimp_display_shell_scale_resize (shell,
                                       shell->display->config->resize_windows_on_resize,
                                       FALSE);
    }
}

static void
gimp_display_shell_quick_mask_changed_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  GtkImage *gtk_image;

  gtk_image = GTK_IMAGE (gtk_bin_get_child (GTK_BIN (shell->quick_mask_button)));

  g_signal_handlers_block_by_func (shell->quick_mask_button,
                                   gimp_display_shell_quick_mask_toggled,
                                   shell);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->quick_mask_button),
                                shell->display->image->quick_mask_state);

  if (shell->display->image->quick_mask_state)
    gtk_image_set_from_stock (gtk_image, GIMP_STOCK_QUICK_MASK_ON,
                              GTK_ICON_SIZE_MENU);
  else
    gtk_image_set_from_stock (gtk_image, GIMP_STOCK_QUICK_MASK_OFF,
                              GTK_ICON_SIZE_MENU);

  g_signal_handlers_unblock_by_func (shell->quick_mask_button,
                                     gimp_display_shell_quick_mask_toggled,
                                     shell);
}

static void
gimp_display_shell_update_guide_handler (GimpImage        *image,
                                         GimpGuide        *guide,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_expose_guide (shell, guide);
}

static void
gimp_display_shell_update_sample_point_handler (GimpImage        *image,
                                                GimpSamplePoint  *sample_point,
                                                GimpDisplayShell *shell)
{
  gimp_display_shell_expose_sample_point (shell, sample_point);
}

static void
gimp_display_shell_image_size_starts_to_fit (GimpDisplayShell *shell,
                                             gint              previous_width,
                                             gint              previous_height,
                                             gint              new_width,
                                             gint              new_height,
                                             gboolean         *horizontally,
                                             gboolean         *vertically)
{
  *horizontally = SCALEX (shell, previous_width)  >  shell->disp_width  &&
                  SCALEX (shell, new_width)       <= shell->disp_width;
  *vertically   = SCALEY (shell, previous_height) >  shell->disp_height &&
                  SCALEY (shell, new_height)      <= shell->disp_height;
}

static void
gimp_display_shell_size_changed_detailed_handler (GimpImage        *image,
                                                  gint              previous_origin_x,
                                                  gint              previous_origin_y,
                                                  gint              previous_width,
                                                  gint              previous_height,
                                                  GimpDisplayShell *shell)
{
  if (shell->display->config->resize_windows_on_resize)
    {
      /* If the window is resized just center the image in it when it
       * has change size
       */
      gimp_display_shell_shrink_wrap (shell, FALSE);
    }
  else
    {
      GimpImage *image;
      gboolean   horizontally, vertically;
      gint       scaled_previous_origin_x = SCALEX (shell, previous_origin_x);
      gint       scaled_previous_origin_y = SCALEY (shell, previous_origin_y);

      image = GIMP_IMAGE (shell->display->image);

      gimp_display_shell_image_size_starts_to_fit (shell,
                                                   previous_width,
                                                   previous_height,
                                                   gimp_image_get_width (image),
                                                   gimp_image_get_height (image),
                                                   &horizontally,
                                                   &vertically);

      gimp_display_shell_scroll_set_offset (shell,
                                            shell->offset_x + scaled_previous_origin_x,
                                            shell->offset_y + scaled_previous_origin_y);

      gimp_display_shell_scroll_center_image (shell, horizontally, vertically);

      /* The above calls might not lead to a call to
       * gimp_display_shell_scroll_clamp_and_update() in all cases we
       * need it to be called, so simply call it explicitly here at
       * the end
       */
      gimp_display_shell_scroll_clamp_and_update (shell);

      gimp_display_shell_expose_full (shell);
    }
}

static void
gimp_display_shell_invalidate_preview_handler (GimpImage        *image,
                                               GimpDisplayShell *shell)
{
  gimp_display_shell_icon_idle_update (shell);
}

static void
gimp_display_shell_profile_changed_handler (GimpColorManaged *image,
                                            GimpDisplayShell *shell)
{
  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (shell));
}

static void
gimp_display_shell_vectors_freeze_handler (GimpVectors      *vectors,
                                           GimpDisplayShell *shell)
{
  if (shell->paused_count == 0 && gimp_item_get_visible (GIMP_ITEM (vectors)))
    gimp_display_shell_draw_vector (shell, vectors);
}

static void
gimp_display_shell_vectors_thaw_handler (GimpVectors      *vectors,
                                         GimpDisplayShell *shell)
{
  if (shell->paused_count == 0 && gimp_item_get_visible (GIMP_ITEM (vectors)))
    gimp_display_shell_draw_vector (shell, vectors);
}

static void
gimp_display_shell_vectors_visible_handler (GimpVectors      *vectors,
                                            GimpDisplayShell *shell)
{
  if (shell->paused_count == 0)
    gimp_display_shell_draw_vector (shell, vectors);
}

static void
gimp_display_shell_vectors_add_handler (GimpContainer    *container,
                                        GimpVectors      *vectors,
                                        GimpDisplayShell *shell)
{
  if (shell->paused_count == 0 && gimp_item_get_visible (GIMP_ITEM (vectors)))
    gimp_display_shell_draw_vector (shell, vectors);
}

static void
gimp_display_shell_vectors_remove_handler (GimpContainer    *container,
                                           GimpVectors      *vectors,
                                           GimpDisplayShell *shell)
{
  if (shell->paused_count == 0 && gimp_item_get_visible (GIMP_ITEM (vectors)))
    gimp_display_shell_draw_vector (shell, vectors);
}

static void
gimp_display_shell_check_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         GimpDisplayShell *shell)
{
  GimpCanvasPaddingMode padding_mode;
  GimpRGB               padding_color;

  gimp_display_shell_get_padding (shell, &padding_mode, &padding_color);

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
      break;

    default:
      break;
    }

  gimp_display_shell_expose_full (shell);
}

static void
gimp_display_shell_title_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_title_update (shell);
}

static void
gimp_display_shell_nav_size_notify_handler (GObject          *config,
                                            GParamSpec       *param_spec,
                                            GimpDisplayShell *shell)
{
  if (shell->nav_popup)
    {
      gtk_widget_destroy (shell->nav_popup);
      shell->nav_popup = NULL;
    }
}

static void
gimp_display_shell_monitor_res_notify_handler (GObject          *config,
                                               GParamSpec       *param_spec,
                                               GimpDisplayShell *shell)
{
  if (GIMP_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      gimp_get_screen_resolution (gtk_widget_get_screen (GTK_WIDGET (shell)),
                                  &shell->monitor_xres,
                                  &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = GIMP_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = GIMP_DISPLAY_CONFIG (config)->monitor_yres;
    }

  gimp_display_shell_scale_changed (shell);

  if (! shell->dot_for_dot)
    {
      gimp_display_shell_scroll_clamp_and_update (shell);

      gimp_display_shell_scaled (shell);

      gimp_display_shell_expose_full (shell);
    }
}

static void
gimp_display_shell_padding_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           GimpDisplayShell *shell)
{
  GimpDisplayConfig     *display_config;
  gboolean               fullscreen;
  GimpCanvasPaddingMode  padding_mode;
  GimpRGB                padding_color;

  display_config = shell->display->config;

  fullscreen = gimp_display_shell_get_fullscreen (shell);

  /*  if the user did not set the padding mode for this display explicitely  */
  if (! shell->fullscreen_options->padding_mode_set)
    {
      padding_mode  = display_config->default_fullscreen_view->padding_mode;
      padding_color = display_config->default_fullscreen_view->padding_color;

      if (fullscreen)
        {
          gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
        }
      else
        {
          shell->fullscreen_options->padding_mode  = padding_mode;
          shell->fullscreen_options->padding_color = padding_color;
        }
    }

  /*  if the user did not set the padding mode for this display explicitely  */
  if (! shell->options->padding_mode_set)
    {
      padding_mode  = display_config->default_view->padding_mode;
      padding_color = display_config->default_view->padding_color;

      if (fullscreen)
        {
          shell->options->padding_mode  = padding_mode;
          shell->options->padding_color = padding_color;
        }
      else
        {
          gimp_display_shell_set_padding (shell, padding_mode, &padding_color);
        }
    }
}

static void
gimp_display_shell_ants_speed_notify_handler (GObject          *config,
                                              GParamSpec       *param_spec,
                                              GimpDisplayShell *shell)
{
  gimp_display_shell_selection_control (shell, GIMP_SELECTION_PAUSE);
  gimp_display_shell_selection_control (shell, GIMP_SELECTION_RESUME);
}

static void
gimp_display_shell_quality_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           GimpDisplayShell *shell)
{
  gimp_display_shell_expose_full (shell);
}
