/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-shadow.h
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

#ifndef __GIMP_DRAWABLE_SHADOW_H__
#define __GIMP_DRAWABLE_SHADOW_H__


TileManager * gimp_drawable_get_shadow_tiles   (GimpDrawable *drawable);
void          gimp_drawable_free_shadow_tiles  (GimpDrawable *drawable);

void          gimp_drawable_merge_shadow_tiles (GimpDrawable *drawable,
                                                gboolean      push_undo,
                                                const gchar  *undo_desc);


#endif /* __GIMP_DRAWABLE_SHADOW_H__ */
