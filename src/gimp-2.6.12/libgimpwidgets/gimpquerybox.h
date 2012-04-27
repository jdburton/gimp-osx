/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpquerybox.h
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_QUERY_BOX_H__
#define __GIMP_QUERY_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  query box callback prototypes  */
typedef void (* GimpQueryStringCallback)  (GtkWidget   *query_box,
                                           const gchar *string,
                                           gpointer     data);

typedef void (* GimpQueryIntCallback)     (GtkWidget   *query_box,
                                           gint         value,
                                           gpointer     data);

typedef void (* GimpQueryDoubleCallback)  (GtkWidget   *query_box,
                                           gdouble      value,
                                           gpointer     data);

typedef void (* GimpQuerySizeCallback)    (GtkWidget   *query_box,
                                           gdouble      size,
                                           GimpUnit     unit,
                                           gpointer     data);

typedef void (* GimpQueryBooleanCallback) (GtkWidget   *query_box,
                                           gboolean     value,
                                           gpointer     data);


#define GIMP_QUERY_BOX_VBOX(qbox) g_object_get_data (G_OBJECT (qbox), \
                                                     "gimp-query-box-vbox")


/*  some simple query dialogs  */
GtkWidget * gimp_query_string_box  (const gchar              *title,
                                    GtkWidget                *parent,
                                    GimpHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    const gchar              *initial,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    GimpQueryStringCallback   callback,
                                    gpointer                  data);

GtkWidget * gimp_query_int_box     (const gchar              *title,
                                    GtkWidget                *parent,
                                    GimpHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gint                      initial,
                                    gint                      lower,
                                    gint                      upper,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    GimpQueryIntCallback      callback,
                                    gpointer                  data);

GtkWidget * gimp_query_double_box  (const gchar              *title,
                                    GtkWidget                *parent,
                                    GimpHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gdouble                   initial,
                                    gdouble                   lower,
                                    gdouble                   upper,
                                    gint                      digits,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    GimpQueryDoubleCallback   callback,
                                    gpointer                  data);

GtkWidget * gimp_query_size_box    (const gchar              *title,
                                    GtkWidget                *parent,
                                    GimpHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gdouble                   initial,
                                    gdouble                   lower,
                                    gdouble                   upper,
                                    gint                      digits,
                                    GimpUnit                  unit,
                                    gdouble                   resolution,
                                    gboolean                  dot_for_dot,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    GimpQuerySizeCallback     callback,
                                    gpointer                  data);

GtkWidget * gimp_query_boolean_box (const gchar              *title,
                                    GtkWidget                *parent,
                                    GimpHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *stock_id,
                                    const gchar              *message,
                                    const gchar              *true_button,
                                    const gchar              *false_button,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    GimpQueryBooleanCallback  callback,
                                    gpointer                  data);


G_END_DECLS

#endif /* __GIMP_QUERY_BOX_H__ */
