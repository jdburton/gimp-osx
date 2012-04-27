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

#ifndef __DIALOGS_H__
#define __DIALOGS_H__


extern GimpDialogFactory *global_dialog_factory;
extern GimpDialogFactory *global_dock_factory;
extern GimpDialogFactory *global_toolbox_factory;
extern GimpDialogFactory *global_display_factory;

extern GimpContainer     *global_recent_docks;


void        dialogs_init              (Gimp            *gimp,
                                       GimpMenuFactory *menu_factory);
void        dialogs_exit              (Gimp            *gimp);

void        dialogs_load_recent_docks (Gimp            *gimp);
void        dialogs_save_recent_docks (Gimp            *gimp);

GtkWidget * dialogs_get_toolbox       (void);


#endif /* __DIALOGS_H__ */
