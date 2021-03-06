/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpradioaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpradioaction.h"


static void   gimp_radio_action_connect_proxy     (GtkAction        *action,
                                                   GtkWidget        *proxy);
static void   gimp_radio_action_set_proxy_tooltip (GimpRadioAction  *action,
                                                   GtkWidget        *proxy);
static void   gimp_radio_action_tooltip_notify    (GimpRadioAction  *action,
                                                   const GParamSpec *pspec,
                                                   gpointer          data);


G_DEFINE_TYPE (GimpRadioAction, gimp_radio_action, GTK_TYPE_RADIO_ACTION)

#define parent_class gimp_radio_action_parent_class


static void
gimp_radio_action_class_init (GimpRadioActionClass *klass)
{
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  action_class->connect_proxy = gimp_radio_action_connect_proxy;
}

static void
gimp_radio_action_init (GimpRadioAction *action)
{
  g_signal_connect (action, "notify::tooltip",
                    G_CALLBACK (gimp_radio_action_tooltip_notify),
                    NULL);
}

static void
gimp_radio_action_connect_proxy (GtkAction *action,
                                 GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  gimp_radio_action_set_proxy_tooltip (GIMP_RADIO_ACTION (action), proxy);
}


/*  public functions  */

GtkRadioAction *
gimp_radio_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *stock_id,
                       gint         value)
{
  GtkRadioAction *action;

  action = g_object_new (GIMP_TYPE_RADIO_ACTION,
                         "name",     name,
                         "label",    label,
                         "tooltip",  tooltip,
                         "value",    value,
                         NULL);

  if (stock_id)
    {
      if (gtk_icon_factory_lookup_default (stock_id))
        g_object_set (action, "stock-id", stock_id, NULL);
      else
        g_object_set (action, "icon-name", stock_id, NULL);
    }

  return action;
}


/*  private functions  */


static void
gimp_radio_action_set_proxy_tooltip (GimpRadioAction *action,
                                     GtkWidget       *proxy)
{
  gchar *tooltip;

  g_object_get (action, "tooltip", &tooltip, NULL);

  if (tooltip)
    {
      gimp_help_set_help_data (proxy, tooltip,
                               g_object_get_qdata (G_OBJECT (proxy),
                                                   GIMP_HELP_ID));
      g_free (tooltip);
    }
}

static void
gimp_radio_action_tooltip_notify (GimpRadioAction  *action,
                                  const GParamSpec *pspec,
                                  gpointer          data)
{
  GSList *list;

  for (list = gtk_action_get_proxies (GTK_ACTION (action));
       list;
       list = g_slist_next (list))
    {
      gimp_radio_action_set_proxy_tooltip (action, list->data);
    }
}
