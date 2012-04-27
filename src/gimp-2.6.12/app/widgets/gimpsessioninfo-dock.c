/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-dock.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpsessioninfo.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dock.h"


enum
{
  SESSION_INFO_BOOK
};


/*  public functions  */

void
gimp_session_info_dock_serialize (GimpConfigWriter *writer,
                                  GList            *books)
{
  GList *list;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (books != NULL);

  gimp_config_writer_open (writer, "dock");

  for (list = books; list; list = g_list_next (list))
    gimp_session_info_book_serialize (writer, list->data);

  gimp_config_writer_close (writer);
}

GTokenType
gimp_session_info_dock_deserialize (GScanner        *scanner,
                                    gint             scope,
                                    GimpSessionInfo *info)
{
  GTokenType token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (info != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "book",
                              GINT_TO_POINTER (SESSION_INFO_BOOK));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
              GimpSessionInfoBook *book;

            case SESSION_INFO_BOOK:
              g_scanner_set_scope (scanner, scope + 1);
              token = gimp_session_info_book_deserialize (scanner, scope + 1,
                                                          &book);

              if (token == G_TOKEN_LEFT_PAREN)
                {
                  info->books = g_list_append (info->books, book);
                  g_scanner_set_scope (scanner, scope);
                }
              else
                return token;

              break;

            default:
              return token;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  g_scanner_scope_remove_symbol (scanner, scope, "book");

  return token;
}

GList *
gimp_session_info_dock_from_widget (GimpDock *dock)
{
  GList *list;
  GList *infos = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  for (list = dock->dockbooks; list; list = g_list_next (list))
    {
      GimpSessionInfoBook *book;

      book = gimp_session_info_book_from_widget (list->data);

      infos = g_list_prepend (infos, book);
    }

  return g_list_reverse (infos);
}

static void
gimp_session_info_dock_paned_size_allocate (GtkWidget     *paned,
                                            GtkAllocation *allocation,
                                            gpointer       data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_dock_paned_size_allocate,
                                        data);

  gtk_paned_set_position (GTK_PANED (paned), GPOINTER_TO_INT (data));
}

static void
gimp_session_info_dock_paned_map (GtkWidget *paned,
                                  gpointer   data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_dock_paned_map,
                                        data);

  g_signal_connect_after (paned, "size-allocate",
                          G_CALLBACK (gimp_session_info_dock_paned_size_allocate),
                          data);
}

void
gimp_session_info_dock_restore (GimpSessionInfo   *info,
                                GimpDialogFactory *factory,
                                GdkScreen         *screen)
{
  GimpDock *dock;
  GList    *books;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  dock = GIMP_DOCK (gimp_dialog_factory_dock_new (factory, screen));

  if (dock && info->aux_info)
    gimp_session_info_aux_set_list (GTK_WIDGET (dock), info->aux_info);

  for (books = info->books; books; books = g_list_next (books))
    {
      GimpSessionInfoBook *book_info = books->data;
      GtkWidget           *dockbook;
      GtkWidget           *parent;

      dockbook = GTK_WIDGET (gimp_session_info_book_restore (book_info, dock));

      parent = gtk_widget_get_parent (dockbook);

      if (GTK_IS_VPANED (parent))
        {
          GtkPaned *paned = GTK_PANED (parent);

          if (dockbook == gtk_paned_get_child2 (paned))
            g_signal_connect_after (paned, "map",
                                    G_CALLBACK (gimp_session_info_dock_paned_map),
                                    GINT_TO_POINTER (book_info->position));
        }
    }

  gtk_widget_show (GTK_WIDGET (dock));
}
