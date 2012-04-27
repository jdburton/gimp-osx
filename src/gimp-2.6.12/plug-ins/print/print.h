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

typedef enum
{
  CENTER_NONE         = 0,
  CENTER_HORIZONTALLY = 1,
  CENTER_VERTICALLY   = 2,
  CENTER_BOTH         = 3
} PrintCenterMode;

typedef struct
{
  gint32              image_id;
  gint32              drawable_id;
  GimpUnit            unit;
  gdouble             xres;
  gdouble             yres;
  GimpUnit            image_unit;
  gdouble             offset_x;
  gdouble             offset_y;
  PrintCenterMode     center;
  gboolean            use_full_page;
  GtkPrintOperation  *operation;
} PrintData;

