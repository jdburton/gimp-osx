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

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "pdb-types.h"

#include "base/temp-buf.h"
#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer-filter.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpparamspecs.h"

#include "gimppdb.h"
#include "gimppdb-utils.h"
#include "gimpprocedure.h"
#include "internal-procs.h"


static GValueArray *
brushes_refresh_invoker (GimpProcedure      *procedure,
                         Gimp               *gimp,
                         GimpContext        *context,
                         GimpProgress       *progress,
                         const GValueArray  *args,
                         GError            **error)
{
  gimp_data_factory_data_refresh (gimp->brush_factory);

  return gimp_procedure_get_return_values (procedure, TRUE, NULL);
}

static GValueArray *
brushes_get_list_invoker (GimpProcedure      *procedure,
                          Gimp               *gimp,
                          GimpContext        *context,
                          GimpProgress       *progress,
                          const GValueArray  *args,
                          GError            **error)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *filter;
  gint32 num_brushes = 0;
  gchar **brush_list = NULL;

  filter = g_value_get_string (&args->values[0]);

  if (success)
    {
      brush_list = gimp_container_get_filtered_name_array (gimp->brush_factory->container,
                                                           filter, &num_brushes);
    }

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    {
      g_value_set_int (&return_vals->values[1], num_brushes);
      gimp_value_take_stringarray (&return_vals->values[2], brush_list, num_brushes);
    }

  return return_vals;
}

static GValueArray *
brushes_get_brush_invoker (GimpProcedure      *procedure,
                           Gimp               *gimp,
                           GimpContext        *context,
                           GimpProgress       *progress,
                           const GValueArray  *args,
                           GError            **error)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  gchar *name = NULL;
  gint32 width = 0;
  gint32 height = 0;
  gint32 spacing = 0;

  GimpBrush *brush = gimp_context_get_brush (context);

  if (brush)
    {
      name    = g_strdup (gimp_object_get_name (GIMP_OBJECT (brush)));
      width   = brush->mask->width;
      height  = brush->mask->height;
      spacing = gimp_brush_get_spacing (brush);
    }
  else
    success = FALSE;

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    {
      g_value_take_string (&return_vals->values[1], name);
      g_value_set_int (&return_vals->values[2], width);
      g_value_set_int (&return_vals->values[3], height);
      g_value_set_int (&return_vals->values[4], spacing);
    }

  return return_vals;
}

static GValueArray *
brushes_get_spacing_invoker (GimpProcedure      *procedure,
                             Gimp               *gimp,
                             GimpContext        *context,
                             GimpProgress       *progress,
                             const GValueArray  *args,
                             GError            **error)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  gint32 spacing = 0;

  GimpBrush *brush = gimp_context_get_brush (context);

  if (brush)
    spacing = gimp_brush_get_spacing (brush);
  else
    success = FALSE;

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (&return_vals->values[1], spacing);

  return return_vals;
}

static GValueArray *
brushes_set_spacing_invoker (GimpProcedure      *procedure,
                             Gimp               *gimp,
                             GimpContext        *context,
                             GimpProgress       *progress,
                             const GValueArray  *args,
                             GError            **error)
{
  gboolean success = TRUE;
  gint32 spacing;

  spacing = g_value_get_int (&args->values[0]);

  if (success)
    {
      gimp_brush_set_spacing (gimp_context_get_brush (context), spacing);
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static GValueArray *
brushes_get_brush_data_invoker (GimpProcedure      *procedure,
                                Gimp               *gimp,
                                GimpContext        *context,
                                GimpProgress       *progress,
                                const GValueArray  *args,
                                GError            **error)
{
  gboolean success = TRUE;
  GValueArray *return_vals;
  const gchar *name;
  gchar *actual_name = NULL;
  gdouble opacity = 0.0;
  gint32 spacing = 0;
  gint32 paint_mode = 0;
  gint32 width = 0;
  gint32 height = 0;
  gint32 length = 0;
  guint8 *mask_data = NULL;

  name = g_value_get_string (&args->values[0]);

  if (success)
    {
      GimpBrush *brush;

      if (name && strlen (name))
        brush = gimp_pdb_get_brush (gimp, name, FALSE, error);
      else
        brush = gimp_context_get_brush (context);

      if (brush)
        {
          actual_name = g_strdup (gimp_object_get_name (GIMP_OBJECT (brush)));
          opacity     = 1.0;
          spacing     = gimp_brush_get_spacing (brush);
          paint_mode  = 0;
          width       = brush->mask->width;
          height      = brush->mask->height;
          length      = brush->mask->height * brush->mask->width;
          mask_data   = g_memdup (temp_buf_data (brush->mask), length);
        }
      else
        success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    {
      g_value_take_string (&return_vals->values[1], actual_name);
      g_value_set_double (&return_vals->values[2], opacity);
      g_value_set_int (&return_vals->values[3], spacing);
      g_value_set_enum (&return_vals->values[4], paint_mode);
      g_value_set_int (&return_vals->values[5], width);
      g_value_set_int (&return_vals->values[6], height);
      g_value_set_int (&return_vals->values[7], length);
      gimp_value_take_int8array (&return_vals->values[8], mask_data, length);
    }

  return return_vals;
}

void
register_brushes_procs (GimpPDB *pdb)
{
  GimpProcedure *procedure;

  /*
   * gimp-brushes-refresh
   */
  procedure = gimp_procedure_new (brushes_refresh_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-refresh");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-refresh",
                                     "Refresh current brushes. This function always succeeds.",
                                     "This procedure retrieves all brushes currently in the user's brush path and updates the brush dialogs accordingly.",
                                     "Seth Burgess",
                                     "Seth Burgess",
                                     "1997",
                                     NULL);
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-brushes-get-list
   */
  procedure = gimp_procedure_new (brushes_get_list_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-get-list");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-get-list",
                                     "Retrieve a complete listing of the available brushes.",
                                     "This procedure returns a complete listing of available GIMP brushes. Each name returned can be used as input to the 'gimp-context-set-brush' procedure.",
                                     "Spencer Kimball & Peter Mattis",
                                     "Spencer Kimball & Peter Mattis",
                                     "1995-1996",
                                     NULL);
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("filter",
                                                       "filter",
                                                       "An optional regular expression used to filter the list",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("num-brushes",
                                                          "num brushes",
                                                          "The number of brushes in the brush list",
                                                          0, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_string_array ("brush-list",
                                                                 "brush list",
                                                                 "The list of brush names",
                                                                 GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-brushes-get-brush
   */
  procedure = gimp_procedure_new (brushes_get_brush_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-get-brush");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-get-brush",
                                     "This procedure is deprecated! Use 'gimp-context-get-brush' instead.",
                                     "This procedure is deprecated! Use 'gimp-context-get-brush' instead.",
                                     "",
                                     "",
                                     "",
                                     "gimp-context-get-brush");
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_string ("name",
                                                           "name",
                                                           "The brush name",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("width",
                                                          "width",
                                                          "The brush width",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("height",
                                                          "height",
                                                          "The brush height",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("spacing",
                                                          "spacing",
                                                          "The brush spacing",
                                                          0, 1000, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-brushes-get-spacing
   */
  procedure = gimp_procedure_new (brushes_get_spacing_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-get-spacing");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-get-spacing",
                                     "This procedure is deprecated! Use 'gimp-brush-get-spacing' instead.",
                                     "This procedure is deprecated! Use 'gimp-brush-get-spacing' instead.",
                                     "",
                                     "",
                                     "",
                                     "gimp-brush-get-spacing");
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("spacing",
                                                          "spacing",
                                                          "The brush spacing",
                                                          0, 1000, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-brushes-set-spacing
   */
  procedure = gimp_procedure_new (brushes_set_spacing_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-set-spacing");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-set-spacing",
                                     "This procedure is deprecated! Use 'gimp-brush-set-spacing' instead.",
                                     "This procedure is deprecated! Use 'gimp-brush-set-spacing' instead.",
                                     "",
                                     "",
                                     "",
                                     "gimp-brush-set-spacing");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_int32 ("spacing",
                                                      "spacing",
                                                      "The brush spacing",
                                                      0, 1000, 0,
                                                      GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-brushes-get-brush-data
   */
  procedure = gimp_procedure_new (brushes_get_brush_data_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-brushes-get-brush-data");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-brushes-get-brush-data",
                                     "This procedure is deprecated! Use 'gimp-brush-get-pixels' instead.",
                                     "This procedure is deprecated! Use 'gimp-brush-get-pixels' instead.",
                                     "",
                                     "",
                                     "",
                                     "gimp-brush-get-pixels");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("name",
                                                       "name",
                                                       "The brush name (\"\" means current active brush)",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_string ("actual-name",
                                                           "actual name",
                                                           "The brush name",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   g_param_spec_double ("opacity",
                                                        "opacity",
                                                        "The brush opacity",
                                                        0, 100, 0,
                                                        GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("spacing",
                                                          "spacing",
                                                          "The brush spacing",
                                                          0, 1000, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   g_param_spec_enum ("paint-mode",
                                                      "paint mode",
                                                      "The paint mode",
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("width",
                                                          "width",
                                                          "The brush width",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("height",
                                                          "height",
                                                          "The brush height",
                                                          G_MININT32, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int32 ("length",
                                                          "length",
                                                          "Length of brush mask data",
                                                          0, G_MAXINT32, 0,
                                                          GIMP_PARAM_READWRITE));
  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_int8_array ("mask-data",
                                                               "mask data",
                                                               "The brush mask data",
                                                               GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}