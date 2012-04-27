/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-book.h
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

#ifndef __GIMP_SESSION_INFO_BOOK_H__
#define __GIMP_SESSION_INFO_BOOK_H__


struct _GimpSessionInfoBook
{
  gint   position;
  gint   current_page;

  /*  list of GimpSessionInfoDockable  */
  GList *dockables;
};


GimpSessionInfoBook *
             gimp_session_info_book_new         (void);
void         gimp_session_info_book_free        (GimpSessionInfoBook  *info);

void         gimp_session_info_book_serialize   (GimpConfigWriter     *writer,
                                                 GimpSessionInfoBook  *book);
GTokenType   gimp_session_info_book_deserialize (GScanner             *scanner,
                                                 gint                  scope,
                                                 GimpSessionInfoBook **book);

GimpSessionInfoBook *
             gimp_session_info_book_from_widget (GimpDockbook         *dockbook);

GimpDockbook * gimp_session_info_book_restore   (GimpSessionInfoBook  *info,
                                                 GimpDock             *dock);


#endif  /* __GIMP_SESSION_INFO_BOOK_H__ */
