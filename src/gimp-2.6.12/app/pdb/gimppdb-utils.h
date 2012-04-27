/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PDB_UTILS_H__
#define __GIMP_PDB_UTILS_H__


GimpBrush     * gimp_pdb_get_brush              (Gimp               *gimp,
                                                 const gchar        *name,
                                                 gboolean            writable,
                                                 GError            **error);
GimpBrush     * gimp_pdb_get_generated_brush    (Gimp               *gimp,
                                                 const gchar        *name,
                                                 gboolean            writable,
                                                 GError            **error);
GimpPattern   * gimp_pdb_get_pattern            (Gimp               *gimp,
                                                 const gchar        *name,
                                                 GError            **error);
GimpGradient  * gimp_pdb_get_gradient           (Gimp               *gimp,
                                                 const gchar        *name,
                                                 gboolean            writable,
                                                 GError            **error);
GimpPalette   * gimp_pdb_get_palette            (Gimp               *gimp,
                                                 const gchar        *name,
                                                 gboolean            writable,
                                                 GError            **error);
GimpFont      * gimp_pdb_get_font               (Gimp               *gimp,
                                                 const gchar        *name,
                                                 GError            **error);
GimpBuffer    * gimp_pdb_get_buffer             (Gimp               *gimp,
                                                 const gchar        *name,
                                                 GError            **error);
GimpPaintInfo * gimp_pdb_get_paint_info         (Gimp               *gimp,
                                                 const gchar        *name,
                                                 GError            **error);

gboolean        gimp_pdb_item_is_attached       (GimpItem           *item,
                                                 GError            **error);
gboolean        gimp_pdb_item_is_floating       (GimpItem           *item,
                                                 GimpImage          *dest_image,
                                                 GError            **error);
gboolean        gimp_pdb_layer_is_text_layer    (GimpLayer          *layer,
                                                 GError            **error);
gboolean        gimp_pdb_image_is_base_type     (GimpImage          *image,
                                                 GimpImageBaseType   type,
                                                 GError            **error);
gboolean        gimp_pdb_image_is_not_base_type (GimpImage          *image,
                                                 GimpImageBaseType   type,
                                                 GError            **error);

GimpStroke    * gimp_pdb_get_vectors_stroke     (GimpVectors        *vectors,
                                                 gint                stroke_ID,
                                                 GError            **error);


#endif /* __GIMP_PDB_UTILS_H__ */
