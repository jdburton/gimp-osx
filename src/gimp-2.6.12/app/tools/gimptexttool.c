/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextTool
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"
#include "core/gimpunit.h"

#include "text/gimptext.h"
#include "text/gimptext-vectors.h"
#include "text/gimptextlayer.h"
#include "text/gimptextundo.h"

#include "vectors/gimpvectors-warp.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"

#include "gimpeditselectiontool.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TEXT_UNDO_TIMEOUT 3


/*  local function prototypes  */

static void gimp_text_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject * gimp_text_tool_constructor    (GType              type,
                                                guint              n_params,
                                                GObjectConstructParam *params);
static void      gimp_text_tool_dispose        (GObject           *object);
static void      gimp_text_tool_finalize       (GObject           *object);

static void      gimp_text_tool_control        (GimpTool          *tool,
                                                GimpToolAction     action,
                                                GimpDisplay       *display);
static void      gimp_text_tool_button_press   (GimpTool          *tool,
                                                GimpCoords        *coords,
                                                guint32            time,
                                                GdkModifierType    state,
                                                GimpDisplay       *display);
static void      gimp_text_tool_button_release (GimpTool          *tool,
                                                GimpCoords        *coords,
                                                guint32            time,
                                                GdkModifierType    state,
                                                GimpButtonReleaseType release_type,
                                                GimpDisplay       *display);
static void      gimp_text_tool_cursor_update  (GimpTool          *tool,
                                                GimpCoords        *coords,
                                                GdkModifierType    state,
                                                GimpDisplay       *display);

static void      gimp_text_tool_connect        (GimpTextTool      *text_tool,
                                                GimpTextLayer     *layer,
                                                GimpText          *text);
static void      gimp_text_tool_layer_notify   (GimpTextLayer     *layer,
                                                GParamSpec        *pspec,
                                                GimpTextTool      *text_tool);
static void      gimp_text_tool_proxy_notify   (GimpText          *text,
                                                GParamSpec        *pspec,
                                                GimpTextTool      *text_tool);
static void      gimp_text_tool_text_notify    (GimpText          *text,
                                                GParamSpec        *pspec,
                                                GimpTextTool      *text_tool);
static gboolean  gimp_text_tool_idle_apply     (GimpTextTool      *text_tool);
static void      gimp_text_tool_apply          (GimpTextTool      *text_tool);

static void      gimp_text_tool_create_vectors (GimpTextTool      *text_tool);
static void      gimp_text_tool_create_vectors_warped
                                               (GimpTextTool      *text_tool);
static void      gimp_text_tool_create_layer   (GimpTextTool      *text_tool,
                                                GimpText          *text);

static void      gimp_text_tool_editor         (GimpTextTool      *text_tool);
static void      gimp_text_tool_editor_update  (GimpTextTool      *text_tool);
static void      gimp_text_tool_text_changed   (GimpTextEditor    *editor,
                                                GimpTextTool      *text_tool);

static void      gimp_text_tool_layer_changed  (GimpImage         *image,
                                                GimpTextTool      *text_tool);
static void      gimp_text_tool_set_image      (GimpTextTool      *text_tool,
                                                GimpImage         *image);
static gboolean  gimp_text_tool_set_drawable   (GimpTextTool      *text_tool,
                                                GimpDrawable      *drawable,
                                                gboolean           confirm);

static gboolean  gimp_text_tool_rectangle_change_complete
                                               (GimpRectangleTool *rect_tool);
void             gimp_rectangle_tool_frame_item(GimpRectangleTool *rect_tool,
                                                GimpItem          *item);

G_DEFINE_TYPE_WITH_CODE (GimpTextTool, gimp_text_tool,
                         GIMP_TYPE_DRAW_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_text_tool_rectangle_tool_iface_init))

#define parent_class gimp_text_tool_parent_class


void
gimp_text_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_TEXT_TOOL,
                GIMP_TYPE_TEXT_OPTIONS,
                gimp_text_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_FONT_MASK       |
                GIMP_CONTEXT_PALETTE_MASK /* for the color popup's palette tab */,
                "gimp-text-tool",
                _("Text"),
                _("Text Tool: Create or edit text layers"),
                N_("Te_xt"), "T",
                NULL, GIMP_HELP_TOOL_TEXT,
                GIMP_STOCK_TOOL_TEXT,
                data);
}

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor  = gimp_text_tool_constructor;
  object_class->dispose      = gimp_text_tool_dispose;
  object_class->finalize     = gimp_text_tool_finalize;
  object_class->set_property = gimp_rectangle_tool_set_property;
  object_class->get_property = gimp_rectangle_tool_get_property;

  gimp_rectangle_tool_install_properties (object_class);

  tool_class->control        = gimp_text_tool_control;
  tool_class->button_press   = gimp_text_tool_button_press;
  tool_class->motion         = gimp_rectangle_tool_motion;
  tool_class->button_release = gimp_text_tool_button_release;
  tool_class->key_press      = gimp_edit_selection_tool_key_press;
  tool_class->oper_update    = gimp_rectangle_tool_oper_update;
  tool_class->cursor_update  = gimp_text_tool_cursor_update;

  draw_tool_class->draw      = gimp_rectangle_tool_draw;
}

static void
gimp_text_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute           = NULL;
  iface->cancel            = NULL;
  iface->rectangle_change_complete = gimp_text_tool_rectangle_change_complete;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  text_tool->proxy   = NULL;
  text_tool->pending = NULL;
  text_tool->idle_id = 0;

  text_tool->text    = NULL;
  text_tool->layer   = NULL;
  text_tool->image   = NULL;

  gimp_tool_control_set_scroll_lock     (tool->control, TRUE);
  gimp_tool_control_set_precision       (tool->control,
                                         GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_TEXT);
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-font-select-set");

  text_tool->handle_rectangle_change_complete = TRUE;
}

static GObject *
gimp_text_tool_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject         *object;
  GimpTextTool    *text_tool;
  GimpTextOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  text_tool = GIMP_TEXT_TOOL (object);
  options   = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);

  text_tool->proxy = g_object_new (GIMP_TYPE_TEXT, NULL);

  gimp_text_options_connect_text (options, text_tool->proxy);

  g_signal_connect_object (text_tool->proxy, "notify",
                           G_CALLBACK (gimp_text_tool_proxy_notify),
                           text_tool, 0);

  return object;
}

static void
gimp_text_tool_dispose (GObject *object)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (object);

  gimp_text_tool_set_drawable (text_tool, NULL, FALSE);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_text_tool_finalize (GObject *object)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (object);

  if (text_tool->proxy)
    {
      g_object_unref (text_tool->proxy);
      text_tool->proxy = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  gimp_rectangle_tool_control (tool, action, display);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_text_tool_set_drawable (text_tool, NULL, FALSE);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_text_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpTextTool      *text_tool   = GIMP_TEXT_TOOL (tool);
  GimpText          *text        = text_tool->text;
  GimpDrawable      *drawable;
  GimpTextOptions   *options     = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpRectangleTool *rect_tool   = GIMP_RECTANGLE_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));


  /* FIXME: this should certainly be done elsewhere */
  g_object_set (options,
                "highlight", FALSE,
                NULL);

  text_tool->x1 = coords->x;
  text_tool->y1 = coords->y;

  gimp_rectangle_tool_button_press (tool, coords, time, state, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  /* bail out now if the rectangle is narrow and the button
     press is outside the layer */
  if (text_tool->layer &&
      gimp_rectangle_tool_get_function (rect_tool) != GIMP_RECTANGLE_TOOL_CREATING)
    {
      GimpItem *item = GIMP_ITEM (text_tool->layer);
      gdouble   x    = coords->x - item->offset_x;
      gdouble   y    = coords->y - item->offset_y;

      if (x < 0 || x > item->width || y < 0 || y > item->height)
        return;
    }

  drawable = gimp_image_get_active_drawable (display->image);

  gimp_text_tool_set_drawable (text_tool, drawable, FALSE);

  if (GIMP_IS_LAYER (drawable))
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gdouble   x    = coords->x - item->offset_x;
      gdouble   y    = coords->y - item->offset_y;

      coords->x -= item->offset_x;
      coords->y -= item->offset_y;

      if (x > 0 && x < gimp_item_width (item) &&
          y > 0 && y < gimp_item_height (item))
        {
          /*  did the user click on a text layer?  */
          if (gimp_text_tool_set_drawable (text_tool, drawable, TRUE))
            {
              /*  on second click, open the text editor  */
              if (text && text_tool->text == text)
                gimp_text_tool_editor (text_tool);

              return;
            }
        }
    }

  /*  create a new text layer  */
  text_tool->text_box_fixed = FALSE;
  gimp_text_tool_connect (text_tool, NULL, NULL);
  gimp_text_tool_editor (text_tool);
}

#define MIN_LAYER_WIDTH 20

/*
 * Here is what we want to do:
 * 1) If the user clicked on an existing text layer, and no rectangle
 *    yet exists there, we want to create one with the right shape.
 * 2) If the user has modified the rectangle for an existing text layer,
 *    we want to change its shape accordingly.  We do this by falling
 *    through to code that causes the "rectangle-change-complete" signal
 *    to be emitted.
 * 3) If the rectangle that has been swept out is too small, we want to
 *    use dynamic text.
 * 4) Otherwise, we want to use the new rectangle that the user has
 *    created as our text box.  This again is done by causing
 *    "rectangle-change-complete" to be emitted.
 */
static void
gimp_text_tool_button_release (GimpTool              *tool,
                               GimpCoords            *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpRectangleTool *rect_tool      = GIMP_RECTANGLE_TOOL (tool);
  GimpTextTool      *text_tool      = GIMP_TEXT_TOOL (tool);
  GimpText          *text           = text_tool->text;
  gint               x1, y1, x2, y2;

  g_object_get (rect_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (text && text_tool->text == text)
    {
      if (gimp_rectangle_tool_rectangle_is_new (rect_tool))
        {
          /* user has clicked on an existing text layer */

          gimp_tool_control_halt (tool->control);
          text_tool->handle_rectangle_change_complete = FALSE;
          gimp_rectangle_tool_frame_item (rect_tool,
                                          GIMP_ITEM (text_tool->layer));
          text_tool->handle_rectangle_change_complete = TRUE;
          return;
        }
      else
        {
          /* user has modified shape of an existing text layer */
        }
    }
  else if (y2 - y1 < MIN_LAYER_WIDTH)
    {
      /* user has clicked in dead space */

      if (GIMP_IS_TEXT (text_tool->proxy))
        g_object_set (text_tool->proxy,
                      "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                      NULL);

      text_tool->handle_rectangle_change_complete = FALSE;
    }
  else
    {
      /* user has defined box for a new text layer */
    }

  gimp_rectangle_tool_button_release (tool, coords, time, state,
                                      release_type, display);
  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
gimp_text_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  /* FIXME: should do something fancy here... */

  gimp_rectangle_tool_cursor_update (tool, coords, state, display);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_text_tool_connect (GimpTextTool  *text_tool,
                        GimpTextLayer *layer,
                        GimpText      *text)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  g_return_if_fail (text == NULL || (layer != NULL && layer->text == text));

  if (! text && text_tool->editor)
    gtk_widget_destroy (text_tool->editor);

  if (text_tool->text != text)
    {
      GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (tool);

      if (text_tool->text)
        {
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                gimp_text_tool_text_notify,
                                                text_tool);

          if (text_tool->pending)
            gimp_text_tool_apply (text_tool);

          if (options->to_vectors_button)
            {
              gtk_widget_set_sensitive (options->to_vectors_button, FALSE);
              g_signal_handlers_disconnect_by_func (options->to_vectors_button,
                                                    gimp_text_tool_create_vectors,
                                                    text_tool);
            }

          if (options->along_vectors_button)
            {
              gtk_widget_set_sensitive (options->along_vectors_button,
                                        FALSE);
              g_signal_handlers_disconnect_by_func (options->along_vectors_button,
                                                    gimp_text_tool_create_vectors_warped,
                                                    text_tool);
            }

          g_object_unref (text_tool->text);
          text_tool->text = NULL;

          g_object_set (text_tool->proxy, "text", NULL, NULL);
        }

      gimp_context_define_property (GIMP_CONTEXT (options),
                                    GIMP_CONTEXT_PROP_FOREGROUND,
                                    text != NULL);

      if (text)
        {
          gimp_config_sync (G_OBJECT (text), G_OBJECT (text_tool->proxy), 0);

          text_tool->text = g_object_ref (text);

          g_signal_connect (text, "notify",
                            G_CALLBACK (gimp_text_tool_text_notify),
                            text_tool);

          if (options->to_vectors_button)
            {
              g_signal_connect_swapped (options->to_vectors_button, "clicked",
                                        G_CALLBACK (gimp_text_tool_create_vectors),
                                        text_tool);
              gtk_widget_set_sensitive (options->to_vectors_button, TRUE);
            }

          if (options->along_vectors_button)
            {
              g_signal_connect_swapped (options->along_vectors_button, "clicked",
                                        G_CALLBACK (gimp_text_tool_create_vectors_warped),
                                        text_tool);
              gtk_widget_set_sensitive (options->along_vectors_button, TRUE);
            }

          if (text_tool->editor)
            gimp_text_tool_editor_update (text_tool);
        }
    }

  if (text_tool->layer != layer)
    {
      if (text_tool->layer)
        {
          g_signal_handlers_disconnect_by_func (text_tool->layer,
                                                gimp_text_tool_layer_notify,
                                                text_tool);
          text_tool->layer = NULL;
        }

      text_tool->layer = layer;

      if (layer)
        g_signal_connect_object (text_tool->layer, "notify::modified",
                                 G_CALLBACK (gimp_text_tool_layer_notify),
                                 text_tool, 0);
    }
}

static void
gimp_text_tool_layer_notify (GimpTextLayer *layer,
                             GParamSpec    *pspec,
                             GimpTextTool  *text_tool)
{
  if (layer->modified)
    gimp_text_tool_connect (text_tool, NULL, NULL);
}

static void
gimp_text_tool_proxy_notify (GimpText     *text,
                             GParamSpec   *pspec,
                             GimpTextTool *text_tool)
{
  if (! text_tool->text)
    return;

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      text_tool->pending = g_list_append (text_tool->pending, pspec);

      if (text_tool->idle_id)
        g_source_remove (text_tool->idle_id);

      text_tool->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) gimp_text_tool_idle_apply, text_tool,
                         NULL);
    }
}

static void
gimp_text_tool_text_notify (GimpText     *text,
                            GParamSpec   *pspec,
                            GimpTextTool *text_tool)
{
  g_return_if_fail (text == text_tool->text);

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      GValue value = { 0, };

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (text), pspec->name, &value);

      g_signal_handlers_block_by_func (text_tool->proxy,
                                       gimp_text_tool_proxy_notify,
                                       text_tool);

      g_object_set_property (G_OBJECT (text_tool->proxy), pspec->name, &value);

      g_signal_handlers_unblock_by_func (text_tool->proxy,
                                         gimp_text_tool_proxy_notify,
                                         text_tool);

      g_value_unset (&value);
    }

  if (text_tool->editor && strcmp (pspec->name, "text") == 0)
    gimp_text_tool_editor_update (text_tool);

  /* we need to redraw the rectangle if it is visible and the shape of
     the layer has changed, because of an undo for example. */
  if (strcmp (pspec->name, "box-width") == 0  ||
      strcmp (pspec->name, "box-height") == 0 ||
      text->box_mode == GIMP_TEXT_BOX_DYNAMIC)
    {
      GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);

      text_tool->handle_rectangle_change_complete = FALSE;
      gimp_rectangle_tool_frame_item (rect_tool,
                                      GIMP_ITEM (text_tool->layer));
      text_tool->handle_rectangle_change_complete = TRUE;
    }
}

static gboolean
gimp_text_tool_idle_apply (GimpTextTool *text_tool)
{
  text_tool->idle_id = 0;

  gimp_text_tool_apply (text_tool);

  return FALSE;
}

static void
gimp_text_tool_apply (GimpTextTool *text_tool)
{
  const GParamSpec *pspec = NULL;
  GimpImage        *image;
  GimpTextLayer    *layer;
  GObject          *src;
  GObject          *dest;
  GList            *list;
  gboolean          push_undo  = TRUE;
  gboolean          undo_group = FALSE;

  if (text_tool->idle_id)
    {
      g_source_remove (text_tool->idle_id);
      text_tool->idle_id = 0;
    }

  g_return_if_fail (text_tool->text != NULL);
  g_return_if_fail (text_tool->layer != NULL);

  layer = text_tool->layer;
  image = gimp_item_get_image (GIMP_ITEM (layer));

  g_return_if_fail (layer->text == text_tool->text);

  /*  Walk over the list of changes and figure out if we are changing
   *  a single property or need to push a full text undo.
   */
  for (list = text_tool->pending;
       list && list->next && list->next->data == list->data;
       list = list->next)
    /* do nothing */;

  if (g_list_length (list) == 1)
    pspec = list->data;

  /*  If we are changing a single property, we don't need to push
   *  an undo if all of the following is true:
   *   - the redo stack is empty
   *   - the last item on the undo stack is a text undo
   *   - the last undo changed the same text property on the same layer
   *   - the last undo happened less than TEXT_UNDO_TIMEOUT seconds ago
   */
  if (pspec)
    {
      GimpUndo *undo = gimp_image_undo_can_compress (image, GIMP_TYPE_TEXT_UNDO,
                                                     GIMP_UNDO_TEXT_LAYER);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        {
          GimpTextUndo *text_undo = GIMP_TEXT_UNDO (undo);

          if (text_undo->pspec == pspec)
            {
              if (gimp_undo_get_age (undo) < TEXT_UNDO_TIMEOUT)
                {
                  GimpTool *tool = GIMP_TOOL (text_tool);

                  push_undo = FALSE;
                  gimp_undo_reset_age (undo);
                  gimp_undo_refresh_preview (undo,
                                             GIMP_CONTEXT (gimp_tool_get_options (tool)));
                }
            }
        }
    }

  if (push_undo)
    {
      if (layer->modified)
        {
          undo_group = TRUE;
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT, NULL);

          gimp_image_undo_push_text_layer_modified (image, NULL, layer);
          gimp_image_undo_push_drawable_mod (image,
                                             NULL, GIMP_DRAWABLE (layer));
        }

      gimp_image_undo_push_text_layer (image, NULL, layer, pspec);
    }

  src  = G_OBJECT (text_tool->proxy);
  dest = G_OBJECT (text_tool->text);

  g_signal_handlers_block_by_func (dest,
                                   gimp_text_tool_text_notify, text_tool);

  g_object_freeze_notify (dest);

  for (; list; list = list->next)
    {
      GValue  value = { 0, };

      /*  look ahead and compress changes  */
      if (list->next && list->next->data == list->data)
        continue;

      pspec = list->data;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (src,  pspec->name, &value);
      g_object_set_property (dest, pspec->name, &value);

      g_value_unset (&value);
    }

  g_list_free (text_tool->pending);
  text_tool->pending = NULL;

  g_object_thaw_notify (dest);

  g_signal_handlers_unblock_by_func (dest,
                                     gimp_text_tool_text_notify, text_tool);

  if (push_undo)
    {
      g_object_set (layer, "modified", FALSE, NULL);

      if (undo_group)
        gimp_image_undo_group_end (image);
    }

  /* if we're doing dynamic text, we want to update the
   * shape of the rectangle */
  if (layer->text->box_mode == GIMP_TEXT_BOX_DYNAMIC)
    {
      text_tool->handle_rectangle_change_complete = FALSE;
      gimp_rectangle_tool_frame_item (GIMP_RECTANGLE_TOOL (text_tool),
                                      GIMP_ITEM (layer));
      text_tool->handle_rectangle_change_complete = TRUE;
    }

  gimp_image_flush (image);
}

static void
gimp_text_tool_create_vectors (GimpTextTool *text_tool)
{
  GimpVectors *vectors;

  if (! text_tool->text || ! text_tool->image)
    return;

  vectors = gimp_text_vectors_new (text_tool->image, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      gimp_item_offsets (GIMP_ITEM (text_tool->layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (vectors), x, y, FALSE);
    }

  gimp_image_add_vectors (text_tool->image, vectors, -1);

  gimp_image_flush (text_tool->image);
}

static void
gimp_text_tool_create_vectors_warped (GimpTextTool *text_tool)
{
  GimpVectors   *vectors0;
  GimpVectors   *vectors;
  GimpText      *text      = text_tool->text;
  gdouble        box_height;

  if (! text || ! text_tool->image || ! text_tool->layer)
    return;

  box_height = gimp_item_height (GIMP_ITEM (text_tool->layer));

  vectors0 = gimp_image_get_active_vectors (text_tool->image);
  if (! vectors0)
    return;

  vectors = gimp_text_vectors_new (text_tool->image, text_tool->text);

  gimp_vectors_warp_vectors (vectors0, vectors, 0.5 * box_height);

  gimp_image_add_vectors (text_tool->image, vectors, -1);
  gimp_image_set_active_vectors (text_tool->image, vectors);
  gimp_item_set_visible (GIMP_ITEM (vectors), TRUE, FALSE);

  gimp_image_flush (text_tool->image);
}

static gdouble
pixels_to_units (Gimp    *gimp,
                 gdouble  pixels,
                 GimpUnit unit,
                 gdouble  resolution)
{
  gdouble factor;

  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      return pixels;

    default:
      factor = _gimp_unit_get_factor (gimp, unit);

      return pixels * factor / resolution;
      break;
    }
}

static void
gimp_text_tool_create_layer (GimpTextTool *text_tool,
                             GimpText     *text)
{
  GimpTool  *tool = GIMP_TOOL (text_tool);
  GimpImage *image;
  GimpLayer *layer;

  if (text)
    {
      text = gimp_config_duplicate (GIMP_CONFIG (text));
    }
  else
    {
      gchar *str;

      str = gimp_text_editor_get_text (GIMP_TEXT_EDITOR (text_tool->editor));

      g_object_set (text_tool->proxy,
                    "text", str,
                    NULL);

      g_free (str);

      text = gimp_config_duplicate (GIMP_CONFIG (text_tool->proxy));
    }

  image = tool->display->image;
  layer = gimp_text_layer_new (image, text);

  g_object_unref (text);

  if (! layer)
    return;

  gimp_text_tool_connect (text_tool, GIMP_TEXT_LAYER (layer), text);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  if (gimp_image_floating_sel (image))
    {
      g_signal_handlers_block_by_func (image,
                                       gimp_text_tool_layer_changed,
                                       text_tool);

      floating_sel_anchor (gimp_image_floating_sel (image));

      g_signal_handlers_unblock_by_func (image,
                                         gimp_text_tool_layer_changed,
                                         text_tool);
    }

  GIMP_ITEM (layer)->offset_x = text_tool->x1;
  GIMP_ITEM (layer)->offset_y = text_tool->y1;

  gimp_image_add_layer (image, layer, -1);

  if (text_tool->text_box_fixed)
    {
      GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);
      GimpItem          *item      = GIMP_ITEM (layer);
      gint               x1, y1, x2, y2;
      gdouble            xres, yres;

      gimp_image_get_resolution (text_tool->image, &xres, &yres);

      g_object_get (rect_tool,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);
      g_object_set (text_tool->proxy,
                    "box-mode",   GIMP_TEXT_BOX_FIXED,
                    "box-width",  pixels_to_units (text_tool->image->gimp,
                                                   x2 - x1,
                                                   text_tool->proxy->box_unit,
                                                   xres),
                    "box-height", pixels_to_units (text_tool->image->gimp,
                                                   y2 - y1,
                                                   text_tool->proxy->box_unit,
                                                   yres),
                    NULL);
      gimp_item_translate (item,
                           x1 - item->offset_x,
                           y1 - item->offset_y,
                           TRUE);
    }
  else
    {
      text_tool->handle_rectangle_change_complete = FALSE;
      gimp_rectangle_tool_frame_item (GIMP_RECTANGLE_TOOL (text_tool),
                                      GIMP_ITEM (layer));
      text_tool->handle_rectangle_change_complete = TRUE;
    }

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);

  gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), FALSE);
}

static void
gimp_text_tool_editor (GimpTextTool *text_tool)
{
  GimpTextOptions   *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpDialogFactory *dialog_factory;
  GtkWindow         *parent  = NULL;

  if (text_tool->editor)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor));
      return;
    }

  dialog_factory = gimp_dialog_factory_from_name ("toplevel");

  if (GIMP_TOOL (text_tool)->display)
    parent = GTK_WINDOW (GIMP_TOOL (text_tool)->display->shell);

  text_tool->editor = gimp_text_options_editor_new (parent, options,
                                                    dialog_factory->menu_factory,
                                                    _("GIMP Text Editor"));

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor),
                             (gpointer) &text_tool->editor);

  gimp_dialog_factory_add_foreign (dialog_factory,
                                   "gimp-text-tool-dialog",
                                   text_tool->editor);

  if (text_tool->text)
    gimp_text_editor_set_text (GIMP_TEXT_EDITOR (text_tool->editor),
                               text_tool->text->text, -1);

  g_signal_connect_object (text_tool->editor, "text-changed",
                           G_CALLBACK (gimp_text_tool_text_changed),
                           text_tool, 0);

  gtk_widget_show (text_tool->editor);
}

static void
gimp_text_tool_editor_update (GimpTextTool *text_tool)
{
  gchar *str = NULL;

  if (! text_tool->editor)
    return;

  if (text_tool->text)
    g_object_get (text_tool->text, "text", &str, NULL);

  g_signal_handlers_block_by_func (text_tool->editor,
                                   gimp_text_tool_text_changed, text_tool);

  gimp_text_editor_set_text (GIMP_TEXT_EDITOR (text_tool->editor),
                             str, str ? -1 : 0);

  g_signal_handlers_unblock_by_func (text_tool->editor,
                                     gimp_text_tool_text_changed, text_tool);

  g_free (str);
}

static void
gimp_text_tool_text_changed (GimpTextEditor *editor,
                             GimpTextTool   *text_tool)
{
  if (text_tool->text)
    {
      gchar *text;

      text = gimp_text_editor_get_text (GIMP_TEXT_EDITOR (text_tool->editor));

      g_object_set (text_tool->proxy, "text", text, NULL);

      g_free (text);
    }
  else
    {
      gimp_text_tool_create_layer (text_tool, NULL);
    }
}

#define  RESPONSE_NEW 1

static void
gimp_text_tool_confirm_response (GtkWidget    *widget,
                                 gint          response_id,
                                 GimpTextTool *text_tool)
{
  GimpTextLayer *layer = text_tool->layer;

  gtk_widget_destroy (widget);

  if (layer && layer->text)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          gimp_text_tool_create_layer (text_tool, layer->text);
          break;

        case GTK_RESPONSE_ACCEPT:
          gimp_text_tool_connect (text_tool, layer, layer->text);

          /*  cause the text layer to be rerendered  */
          if (text_tool->proxy)
            g_object_notify (G_OBJECT (text_tool->proxy), "text");

          gimp_text_tool_editor (text_tool);
          break;

        default:
          break;
        }
    }
}

static void
gimp_text_tool_confirm_dialog (GimpTextTool *text_tool)
{
  GimpTool  *tool = GIMP_TOOL (text_tool);
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;

  g_return_if_fail (text_tool->layer != NULL);

  if (text_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->confirm_dialog));
      return;
    }

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (text_tool->layer),
                                     GIMP_CONTEXT (gimp_tool_get_options (tool)),
                                     _("Confirm Text Editing"),
                                     "gimp-text-tool-confirm",
                                     GIMP_STOCK_TEXT_LAYER,
                                     _("Confirm Text Editing"),
                                     tool->display->shell,
                                     gimp_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     GTK_STOCK_CANCEL,       GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_EDIT,         GTK_RESPONSE_ACCEPT,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_NEW,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_text_tool_confirm_response),
                    text_tool);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("The layer you selected is a text layer but "
                           "it has been modified using other tools. "
                           "Editing the layer with the text tool will "
                           "discard these modifications."
                           "\n\n"
                           "You can edit the layer or create a new "
                           "text layer from its text attributes."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  text_tool->confirm_dialog = dialog;
  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_nullify_pointer),
                            &text_tool->confirm_dialog);
}

static void
gimp_text_tool_layer_changed (GimpImage    *image,
                              GimpTextTool *text_tool)
{
  GimpLayer         *layer     = gimp_image_get_active_layer (image);
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);

  gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), FALSE);

  if (text_tool->layer)
    {
      if (! gimp_rectangle_tool_rectangle_is_new (rect_tool))
        {
          text_tool->handle_rectangle_change_complete = FALSE;
          gimp_rectangle_tool_frame_item (rect_tool,
                                          GIMP_ITEM (text_tool->layer));
          text_tool->handle_rectangle_change_complete = TRUE;
        }
    }
}

static void
gimp_text_tool_set_image (GimpTextTool *text_tool,
                          GimpImage    *image)
{
  if (text_tool->image == image)
    return;

  if (text_tool->image)
    {
      g_signal_handlers_disconnect_by_func (text_tool->image,
                                            gimp_text_tool_layer_changed,
                                            text_tool);

      g_object_remove_weak_pointer (G_OBJECT (text_tool->image),
                                    (gpointer) &text_tool->image);
      text_tool->image = NULL;
    }

  if (image)
    {
      GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
      gdouble          xres;
      gdouble          yres;

      gimp_image_get_resolution (image, &xres, &yres);

      text_tool->image = image;
      g_object_add_weak_pointer (G_OBJECT (text_tool->image),
                                 (gpointer) &text_tool->image);

      g_signal_connect_object (text_tool->image, "active-layer-changed",
                               G_CALLBACK (gimp_text_tool_layer_changed),
                               text_tool, 0);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_entry), 0,
                                      yres, FALSE);
    }
}

static gboolean
gimp_text_tool_set_drawable (GimpTextTool *text_tool,
                             GimpDrawable *drawable,
                             gboolean      confirm)
{
  GimpImage *image = NULL;

  if (text_tool->confirm_dialog)
    gtk_widget_destroy (text_tool->confirm_dialog);

  if (drawable)
    image = gimp_item_get_image (GIMP_ITEM (drawable));

  gimp_text_tool_set_image (text_tool, image);

  if (GIMP_IS_TEXT_LAYER (drawable) && GIMP_TEXT_LAYER (drawable)->text)
    {
      GimpTextLayer *layer = GIMP_TEXT_LAYER (drawable);

      if (layer == text_tool->layer && layer->text == text_tool->text)
        return TRUE;

      if (layer->modified)
        {
          if (confirm)
            {
              gimp_text_tool_connect (text_tool, layer, NULL);
              gimp_text_tool_confirm_dialog (text_tool);
              return TRUE;
            }
        }
      else
        {
          gimp_text_tool_connect (text_tool, layer, layer->text);
          return TRUE;
        }
    }

  gimp_text_tool_connect (text_tool, NULL, NULL);
  text_tool->layer = NULL;

  return FALSE;
}

void
gimp_text_tool_set_layer (GimpTextTool *text_tool,
                          GimpLayer    *layer)
{
  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));
  g_return_if_fail (layer == NULL || GIMP_IS_LAYER (layer));

  if (gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), TRUE))
    {
      GimpTool    *tool = GIMP_TOOL (text_tool);
      GimpItem    *item = GIMP_ITEM (layer);
      GimpContext *context;
      GimpDisplay *display;

      context = gimp_get_user_context (tool->tool_info->gimp);
      display = gimp_context_get_display (context);

      if (! display || display->image != item->image)
        {
          GList *list;

          display = NULL;

          for (list = GIMP_LIST (tool->tool_info->gimp->displays)->list;
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (display->image == item->image)
                {
                  gimp_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      tool->display = display;

      if (tool->display)
        {
          tool->drawable = GIMP_DRAWABLE (layer);

          gimp_text_tool_editor (text_tool);
        }
    }
}

static gboolean
gimp_text_tool_rectangle_change_complete (GimpRectangleTool *rect_tool)
{
  GimpTextTool   *text_tool     = GIMP_TEXT_TOOL (rect_tool);
  GimpText       *text          = text_tool->text;
  GimpImage      *image         = text_tool->image;
  GimpItem       *item ;
  gint            x1, y1, x2, y2;

  if (text_tool->handle_rectangle_change_complete)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image, &xres, &yres);

      g_object_get (rect_tool,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      text_tool->text_box_fixed = TRUE;

      if (! text)
        {
          /*
           * we can't set properties for the text layer, because
           * it isn't created until some text has been inserted,
           * so we need to make a special note that will remind
           * us what to do when we actually create the layer
           */
          return TRUE;
        }

      g_object_set (text_tool->proxy,
                    "box-mode",   GIMP_TEXT_BOX_FIXED,
                    "box-width",  pixels_to_units (image->gimp,
                                                   x2 - x1,
                                                   text_tool->proxy->box_unit,
                                                   xres),
                    "box-height", pixels_to_units (image->gimp,
                                                   y2 - y1,
                                                   text_tool->proxy->box_unit,
                                                   yres),
                    NULL);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT,
                               _("Reshape Text Layer"));
      item = GIMP_ITEM (text_tool->layer);
      gimp_item_translate (item,
                           x1 - item->offset_x,
                           y1 - item->offset_y,
                           TRUE);
      gimp_text_tool_apply (text_tool);
      gimp_image_undo_group_end (image);
    }

  return TRUE;
}

/* this doesn't belong here but this is the only place
   it is used at the moment -- Bill
*/

/**
 * gimp_rectangle_tool_frame_item:
 * @rect_tool: a #GimpRectangleTool interface
 * @item:      a #GimpItem attached to the image on which a
 *             rectangle is being shown.
 *
 * Convenience function to set the corners of the rectangle to
 * match the bounds of the specified item.  The rectangle interface
 * must be active (i.e., showing a rectangle), and the item must be
 * attached to the image on which the rectangle is active.
 */
void
gimp_rectangle_tool_frame_item (GimpRectangleTool *rect_tool,
                                GimpItem          *item)
{
  GimpDisplay *display  = GIMP_TOOL (rect_tool)->display;
  gint         offset_x;
  gint         offset_y;
  gint         width;
  gint         height;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (display != NULL);
  g_return_if_fail (display->image == item->image);

  width  = gimp_item_width (item);
  height = gimp_item_height (item);

  gimp_item_offsets (item, &offset_x, &offset_y);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_tool));

  gimp_rectangle_tool_set_function (rect_tool,
                                    GIMP_RECTANGLE_TOOL_CREATING);

  g_object_set (rect_tool,
                "x1", offset_x,
                "y1", offset_y,
                "x2", offset_x + width,
                "y2", offset_y + height,
                NULL);

  /*
   * kludge to force handle sizes to update.  This call may be
   * harmful if this function is ever moved out of the text tool code.
   */
  gimp_rectangle_tool_set_constraint (rect_tool,
                                      GIMP_RECTANGLE_CONSTRAIN_NONE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_tool));
}

