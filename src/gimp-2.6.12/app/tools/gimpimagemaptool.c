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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimagemap.h"
#include "core/gimpimagemapconfig.h"
#include "core/gimplist.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsettingsbox.h"
#include "widgets/gimptooldialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpcoloroptions.h"
#include "gimpimagemaptool.h"
#include "gimpimagemaptool-settings.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void      gimp_image_map_tool_class_init  (GimpImageMapToolClass *klass);
static void      gimp_image_map_tool_base_init   (GimpImageMapToolClass *klass);

static void      gimp_image_map_tool_init        (GimpImageMapTool *im_tool);

static GObject * gimp_image_map_tool_constructor (GType             type,
                                                  guint             n_params,
                                                  GObjectConstructParam *params);
static void      gimp_image_map_tool_finalize    (GObject          *object);

static gboolean  gimp_image_map_tool_initialize  (GimpTool         *tool,
                                                  GimpDisplay      *display,
                                                  GError          **error);
static void      gimp_image_map_tool_control     (GimpTool         *tool,
                                                  GimpToolAction    action,
                                                  GimpDisplay      *display);
static gboolean  gimp_image_map_tool_key_press   (GimpTool         *tool,
                                                  GdkEventKey      *kevent,
                                                  GimpDisplay      *display);

static gboolean  gimp_image_map_tool_pick_color  (GimpColorTool    *color_tool,
                                                  gint              x,
                                                  gint              y,
                                                  GimpImageType    *sample_type,
                                                  GimpRGB          *color,
                                                  gint             *color_index);
static void      gimp_image_map_tool_map         (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_dialog      (GimpImageMapTool *im_tool);
static void      gimp_image_map_tool_reset       (GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_flush       (GimpImageMap     *image_map,
                                                  GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_response    (GtkWidget        *widget,
                                                  gint              response_id,
                                                  GimpImageMapTool *im_tool);

static void      gimp_image_map_tool_notify_preview (GObject           *config,
                                                     GParamSpec        *pspec,
                                                     GimpImageMapTool  *im_tool);
static void      gimp_image_map_tool_gegl_notify    (GObject           *config,
                                                     const GParamSpec  *pspec,
                                                     GimpImageMapTool  *im_tool);


static GimpColorToolClass *parent_class = NULL;


GType
gimp_image_map_tool_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpImageMapToolClass),
        (GBaseInitFunc) gimp_image_map_tool_base_init,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_image_map_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImageMapTool),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_map_tool_init,
      };

      type = g_type_register_static (GIMP_TYPE_COLOR_TOOL,
                                     "GimpImageMapTool",
                                     &info, 0);
    }

  return type;
}


static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_image_map_tool_constructor;
  object_class->finalize     = gimp_image_map_tool_finalize;

  tool_class->initialize     = gimp_image_map_tool_initialize;
  tool_class->control        = gimp_image_map_tool_control;
  tool_class->key_press      = gimp_image_map_tool_key_press;

  color_tool_class->pick     = gimp_image_map_tool_pick_color;

  klass->shell_desc          = NULL;
  klass->settings_name       = NULL;
  klass->import_dialog_title = NULL;
  klass->export_dialog_title = NULL;

  klass->get_operation       = NULL;
  klass->map                 = NULL;
  klass->dialog              = NULL;
  klass->reset               = NULL;
  klass->settings_import     = gimp_image_map_tool_real_settings_import;
  klass->settings_export     = gimp_image_map_tool_real_settings_export;
}

static void
gimp_image_map_tool_base_init (GimpImageMapToolClass *klass)
{
  klass->recent_settings = gimp_list_new (GIMP_TYPE_IMAGE_MAP_CONFIG, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (klass->recent_settings),
                           (GCompareFunc) gimp_image_map_config_compare);
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION);

  image_map_tool->drawable       = NULL;
  image_map_tool->operation      = NULL;
  image_map_tool->config         = NULL;
  image_map_tool->default_config = NULL;
  image_map_tool->image_map      = NULL;

  image_map_tool->shell          = NULL;
  image_map_tool->main_vbox      = NULL;
  image_map_tool->settings_box   = NULL;
  image_map_tool->label_group    = NULL;
}

static GObject *
gimp_image_map_tool_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject               *object;
  GimpImageMapTool      *image_map_tool;
  GimpImageMapToolClass *klass;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  image_map_tool = GIMP_IMAGE_MAP_TOOL (object);
  klass          = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

  if (klass->get_operation)
    image_map_tool->operation = klass->get_operation (image_map_tool,
                                                      &image_map_tool->config);

  return object;
}

static void
gimp_image_map_tool_finalize (GObject *object)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (object);

  if (image_map_tool->operation)
    {
      g_object_unref (image_map_tool->operation);
      image_map_tool->operation = NULL;
    }

  if (image_map_tool->config)
    {
      g_object_unref (image_map_tool->config);
      image_map_tool->config = NULL;
    }

  if (image_map_tool->default_config)
    {
      g_object_unref (image_map_tool->default_config);
      image_map_tool->default_config = NULL;
    }

  if (image_map_tool->shell)
    {
      gtk_widget_destroy (image_map_tool->shell);
      image_map_tool->shell        = NULL;
      image_map_tool->main_vbox    = NULL;
      image_map_tool->settings_box = NULL;
      image_map_tool->label_group  = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static gboolean
gimp_image_map_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);
  GimpToolInfo     *tool_info      = tool->tool_info;
  GimpDrawable     *drawable;

  /*  set display so the dialog can be hidden on display destruction  */
  tool->display = display;

  if (! image_map_tool->shell)
    {
      GimpImageMapToolClass *klass;
      GtkWidget             *shell;
      GtkWidget             *vbox;
      GtkWidget             *toggle;
      const gchar           *stock_id;

      klass = GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool);

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

      image_map_tool->shell = shell =
        gimp_tool_dialog_new (tool_info,
                              display->shell,
                              klass->shell_desc,

                              GIMP_STOCK_RESET, RESPONSE_RESET,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell),
                                               RESPONSE_RESET,
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (shell, "response",
                               G_CALLBACK (gimp_image_map_tool_response),
                               G_OBJECT (image_map_tool), 0);

      image_map_tool->main_vbox = vbox = gtk_vbox_new (FALSE, 6);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);

      if (klass->settings_name)
        gimp_image_map_tool_add_settings_gui (image_map_tool);

      /*  The preview toggle  */
      toggle = gimp_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview",
                                           _("_Preview"));

      gtk_box_pack_end (GTK_BOX (image_map_tool->main_vbox), toggle,
                        FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect_object (tool_info->tool_options, "notify::preview",
                               G_CALLBACK (gimp_image_map_tool_notify_preview),
                               image_map_tool, 0);

      /*  Fill in subclass widgets  */
      gimp_image_map_tool_dialog (image_map_tool);

      gtk_widget_show (vbox);

      if (image_map_tool->operation)
        g_signal_connect_object (tool_info->gimp->config, "notify::use-gegl",
                                 G_CALLBACK (gimp_image_map_tool_gegl_notify),
                                 image_map_tool, 0);
    }

  drawable = gimp_image_get_active_drawable (display->image);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (image_map_tool->shell),
                                     GIMP_VIEWABLE (drawable),
                                     GIMP_CONTEXT (tool_info->tool_options));

  gtk_widget_show (image_map_tool->shell);

  image_map_tool->drawable = drawable;

  gimp_image_map_tool_create_map (image_map_tool);

  return TRUE;
}

static void
gimp_image_map_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (image_map_tool->shell)
        gtk_dialog_response (GTK_DIALOG (image_map_tool->shell),
                             GTK_RESPONSE_CANCEL);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_image_map_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_Return:
        case GDK_KP_Enter:
        case GDK_ISO_Enter:
          gimp_image_map_tool_response (NULL, GTK_RESPONSE_OK, image_map_tool);
          return TRUE;

        case GDK_BackSpace:
          gimp_image_map_tool_response (NULL, RESPONSE_RESET, image_map_tool);
          return TRUE;

        case GDK_Escape:
          gimp_image_map_tool_response (NULL, GTK_RESPONSE_CANCEL, image_map_tool);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gimp_image_map_tool_pick_color (GimpColorTool *color_tool,
                                gint           x,
                                gint           y,
                                GimpImageType *sample_type,
                                GimpRGB       *color,
                                gint          *color_index)
{
  GimpImageMapTool *tool = GIMP_IMAGE_MAP_TOOL (color_tool);
  gint              off_x, off_y;

  gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  *sample_type = gimp_drawable_type (tool->drawable);

  return gimp_pickable_pick_color (GIMP_PICKABLE (tool->image_map),
                                   x - off_x,
                                   y - off_y,
                                   color_tool->options->sample_average,
                                   color_tool->options->average_radius,
                                   color, color_index);
}

static void
gimp_image_map_tool_map (GimpImageMapTool *tool)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (GIMP_TOOL (tool)->display->shell);
  GimpItem         *item  = GIMP_ITEM (tool->drawable);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;

  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->map (tool);

  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  gimp_item_offsets (item, &off_x, &off_y);

  gimp_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            gimp_item_width  (item),
                            gimp_item_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  visible.x -= off_x;
  visible.y -= off_y;

  gimp_image_map_apply (tool->image_map, &visible);
}

static void
gimp_image_map_tool_dialog (GimpImageMapTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->dialog (tool);
}

static void
gimp_image_map_tool_reset (GimpImageMapTool *tool)
{
  if (GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->reset)
    {
      GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->reset (tool);
    }
  else if (tool->config)
    {
      if (tool->default_config)
        {
          gimp_config_copy (GIMP_CONFIG (tool->default_config),
                            GIMP_CONFIG (tool->config),
                            0);
        }
      else
        {
          gimp_config_reset (GIMP_CONFIG (tool->config));
        }
    }
}

void
gimp_image_map_tool_create_map (GimpImageMapTool *tool)
{
  Gimp     *gimp;
  gboolean  use_gegl;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool));

  gimp = GIMP_TOOL (tool)->tool_info->gimp;

  if (tool->image_map)
    {
      gimp_image_map_clear (tool->image_map);
      g_object_unref (tool->image_map);
    }

  g_assert (tool->operation || tool->apply_func);

  use_gegl = gimp_use_gegl (gimp) || ! tool->apply_func;

  tool->image_map = gimp_image_map_new (tool->drawable,
                                        GIMP_TOOL (tool)->tool_info->blurb,
                                        use_gegl ? tool->operation : NULL,
                                        tool->apply_func,
                                        tool->apply_data);

  g_signal_connect (tool->image_map, "flush",
                    G_CALLBACK (gimp_image_map_tool_flush),
                    tool);
}

static void
gimp_image_map_tool_flush (GimpImageMap     *image_map,
                           GimpImageMapTool *image_map_tool)
{
  GimpTool    *tool = GIMP_TOOL (image_map_tool);
  GimpDisplay *display = tool->display;

  gimp_projection_flush_now (gimp_image_get_projection (display->image));
  gimp_display_flush_now (display);
}

static void
gimp_image_map_tool_response (GtkWidget        *widget,
                              gint              response_id,
                              GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_image_map_tool_reset (image_map_tool);
      gimp_image_map_tool_preview (image_map_tool);
      break;

    case GTK_RESPONSE_OK:
      gimp_dialog_factory_hide_dialog (image_map_tool->shell);

      if (image_map_tool->image_map)
        {
          GimpImageMapOptions *options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

          gimp_tool_control_set_preserve (tool->control, TRUE);

          if (! options->preview)
            gimp_image_map_tool_map (image_map_tool);

          gimp_image_map_commit (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (tool->display->image);

          if (image_map_tool->config)
            gimp_settings_box_add_current (GIMP_SETTINGS_BOX (image_map_tool->settings_box));
        }

      tool->display  = NULL;
      tool->drawable = NULL;
      break;

    default:
      gimp_dialog_factory_hide_dialog (image_map_tool->shell);

      if (image_map_tool->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_abort (image_map_tool->image_map);
          g_object_unref (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (tool->display->image);
        }

      tool->display  = NULL;
      tool->drawable = NULL;
      break;
    }
}

static void
gimp_image_map_tool_notify_preview (GObject          *config,
                                    GParamSpec       *pspec,
                                    GimpImageMapTool *image_map_tool)
{
  GimpTool            *tool    = GIMP_TOOL (image_map_tool);
  GimpImageMapOptions *options = GIMP_IMAGE_MAP_OPTIONS (config);

  if (image_map_tool->image_map)
    {
      if (options->preview)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_tool_map (image_map_tool);

          gimp_tool_control_set_preserve (tool->control, FALSE);
        }
      else
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_clear (image_map_tool->image_map);

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_map_tool_flush (image_map_tool->image_map,
                                     image_map_tool);
        }
    }
}

void
gimp_image_map_tool_preview (GimpImageMapTool *image_map_tool)
{
  GimpTool            *tool;
  GimpImageMapOptions *options;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (image_map_tool));

  tool    = GIMP_TOOL (image_map_tool);
  options = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);

  if (image_map_tool->image_map && options->preview)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_set_preserve (tool->control, FALSE);
    }
}

static void
gimp_image_map_tool_gegl_notify (GObject          *config,
                                 const GParamSpec *pspec,
                                 GimpImageMapTool *im_tool)
{
  if (im_tool->image_map)
    {
      gimp_tool_control_set_preserve (GIMP_TOOL (im_tool)->control, TRUE);

      gimp_image_map_tool_create_map (im_tool);

      gimp_tool_control_set_preserve (GIMP_TOOL (im_tool)->control, FALSE);

      gimp_image_map_tool_preview (im_tool);
    }
}

void
gimp_image_map_tool_edit_as (GimpImageMapTool *im_tool,
                             const gchar      *new_tool_id,
                             GimpConfig       *config)
{
  GimpDisplay  *display;
  GimpContext  *user_context;
  GimpToolInfo *tool_info;
  GimpTool     *new_tool;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (im_tool));
  g_return_if_fail (new_tool_id);
  g_return_if_fail (GIMP_IS_CONFIG (config));

  display = GIMP_TOOL (im_tool)->display;

  user_context = gimp_get_user_context (display->gimp);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (display->gimp->tool_info_list,
                                      new_tool_id);

  gimp_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->gimp, display);

  new_tool = tool_manager_get_active (display->gimp);

  GIMP_IMAGE_MAP_TOOL (new_tool)->default_config = g_object_ref (config);

  gimp_image_map_tool_reset (GIMP_IMAGE_MAP_TOOL (new_tool));
}

GtkWidget *
gimp_image_map_tool_dialog_get_vbox (GimpImageMapTool *tool)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool), NULL);

  return tool->main_vbox;
}


GtkSizeGroup *
gimp_image_map_tool_dialog_get_label_group (GimpImageMapTool *tool)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_MAP_TOOL (tool), NULL);

  return tool->label_group;
}
