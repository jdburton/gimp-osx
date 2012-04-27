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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpunit.h"

#include "file/file-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-title.h"
#include "gimpstatusbar.h"

#include "about.h"

#include "gimp-intl.h"


#define MAX_TITLE_BUF 512


static gboolean gimp_display_shell_update_title_idle (gpointer          data);
static gint     gimp_display_shell_format_title      (GimpDisplayShell *display,
                                                      gchar            *title,
                                                      gint              title_len,
                                                      const gchar      *format);


/*  public functions  */

void
gimp_display_shell_title_init (GimpDisplayShell *shell)
{
}

void
gimp_display_shell_title_update (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->title_idle_id)
    g_source_remove (shell->title_idle_id);

  shell->title_idle_id = g_idle_add (gimp_display_shell_update_title_idle,
                                     shell);
}


/*  private functions  */

static gboolean
gimp_display_shell_update_title_idle (gpointer data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  shell->title_idle_id = 0;

  if (shell->display->image)
    {
      GimpDisplayConfig *config = shell->display->config;
      gchar              title[MAX_TITLE_BUF];
      gint               len;

      /* format the title */
      len = gimp_display_shell_format_title (shell, title, sizeof (title),
                                             config->image_title_format);

      if (len)  /* U+2013 EN DASH */
        len += g_strlcpy (title + len, " \342\200\223 ", sizeof (title) - len);

      g_strlcpy (title + len, GIMP_ACRONYM, sizeof (title) - len);

      gdk_window_set_title (GTK_WIDGET (shell)->window, title);

      /* format the statusbar */
      gimp_display_shell_format_title (shell, title, sizeof (title),
                                       config->image_status_format);

      gimp_statusbar_replace (GIMP_STATUSBAR (shell->statusbar), "title",
                              NULL, "%s", title);
    }
  else
    {
      gdk_window_set_title (GTK_WIDGET (shell)->window, GIMP_NAME);

      gimp_statusbar_replace (GIMP_STATUSBAR (shell->statusbar), "title",
                              NULL, " ");
    }

  return FALSE;
}


static gint print (gchar       *buf,
                   gint         len,
                   gint         start,
                   const gchar *fmt,
                   ...) G_GNUC_PRINTF (4, 5);

static gint
print (gchar       *buf,
       gint         len,
       gint         start,
       const gchar *fmt,
       ...)
{
  va_list args;
  gint    printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static gint
gimp_display_shell_format_title (GimpDisplayShell *shell,
                                 gchar            *title,
                                 gint              title_len,
                                 const gchar      *format)
{
  Gimp      *gimp;
  GimpImage *image;
  gint       num, denom;
  gint       i = 0;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), 0);

  image = shell->display->image;
  gimp  = shell->display->gimp;

  if (! image)
    {
      title[0] = '\n';
      return 0;
    }

  gimp_zoom_model_get_fraction (shell->zoom, &num, &denom);

  while (i < title_len && *format)
    {
      switch (*format)
        {
        case '%':
          format++;
          switch (*format)
            {
            case 0:
              /* format string ends within %-sequence, print literal '%' */

            case '%':
              title[i++] = '%';
              break;

            case 'f': /* pruned filename */
              {
                const gchar *uri = gimp_image_get_uri (image);
                gchar       *basename;

                basename = file_utils_uri_display_basename (uri);

                i += print (title, title_len, i, "%s", basename);

                g_free (basename);
              }
              break;

            case 'F': /* full filename */
              {
                gchar *filename;
                const gchar *uri = gimp_image_get_uri (image);

                filename = file_utils_uri_display_name (uri);

                i += print (title, title_len, i, "%s", filename);

                g_free (filename);
              }
              break;

            case 'p': /* PDB id */
              i += print (title, title_len, i, "%d", gimp_image_get_ID (image));
              break;

            case 'i': /* instance */
              i += print (title, title_len, i, "%d", shell->display->instance);
              break;

            case 't': /* type */
              {
                const gchar *image_type_str = NULL;
                gboolean     empty          = gimp_image_is_empty (image);

                switch (gimp_image_base_type (image))
                  {
                  case GIMP_RGB:
                    image_type_str = empty ? _("RGB-empty") : _("RGB");
                    break;
                  case GIMP_GRAY:
                    image_type_str = empty ? _("grayscale-empty") : _("grayscale");
                    break;
                  case GIMP_INDEXED:
                    image_type_str = empty ? _("indexed-empty") : _("indexed");
                    break;
                  default:
                    g_assert_not_reached ();
                    break;
                  }

                i += print (title, title_len, i, "%s", image_type_str);
              }
              break;

            case 's': /* user source zoom factor */
              i += print (title, title_len, i, "%d", denom);
              break;

            case 'd': /* user destination zoom factor */
              i += print (title, title_len, i, "%d", num);
              break;

            case 'z': /* user zoom factor (percentage) */
              {
                gdouble  scale = gimp_zoom_model_get_factor (shell->zoom);

                i += print (title, title_len, i,
                            scale >= 0.15 ? "%.0f" : "%.2f", 100.0 * scale);
              }
              break;

            case 'D': /* dirty flag */
              if (format[1] == 0)
                {
                  /* format string ends within %D-sequence, print literal '%D' */
                  i += print (title, title_len, i, "%%D");
                  break;
                }
              if (image->dirty)
                title[i++] = format[1];
              format++;
              break;

            case 'C': /* clean flag */
              if (format[1] == 0)
                {
                  /* format string ends within %C-sequence, print literal '%C' */
                  i += print (title, title_len, i, "%%C");
                  break;
                }
              if (! image->dirty)
                title[i++] = format[1];
              format++;
              break;

            case 'B': /* dirty flag (long) */
              if (image->dirty)
                i += print (title, title_len, i, "%s", _("(modified)"));
              break;

            case 'A': /* clean flag (long) */
              if (! image->dirty)
                i += print (title, title_len, i, "%s", _("(clean)"));
              break;

            case 'm': /* memory used by image */
              {
                GimpObject *object = GIMP_OBJECT (image);
                gchar      *str;

                str = g_format_size_for_display (gimp_object_get_memsize (object,
                                                                          NULL));

                i += print (title, title_len, i, "%s", str);

                g_free (str);
              }
              break;

            case 'l': /* number of layers */
              i += print (title, title_len, i, "%d",
                          gimp_container_num_children (image->layers));
              break;

            case 'L': /* number of layers (long) */
              {
                gint num = gimp_container_num_children (image->layers);

                i += print (title, title_len, i,
                            ngettext ("%d layer", "%d layers", num), num);
              }
              break;

            case 'n': /* active drawable name */
              {
                GimpDrawable *drawable = gimp_image_get_active_drawable (image);

                if (drawable)
                  {
                    gchar *desc;

                    desc = gimp_viewable_get_description (GIMP_VIEWABLE (drawable),
                                                          NULL);

                    i += print (title, title_len, i, "%s", desc);

                    g_free (desc);
                  }
                else
                  {
                    i += print (title, title_len, i, "%s", _("(none)"));
                  }
              }
              break;

            case 'P': /* active drawable PDB id */
              {
                GimpDrawable *drawable = gimp_image_get_active_drawable (image);

                if (drawable)
                  i += print (title, title_len, i, "%d",
                              gimp_item_get_ID (GIMP_ITEM (drawable)));
                else
                  i += print (title, title_len, i, "%s", _("(none)"));
              }
              break;

            case 'W': /* width in real-world units */
              if (shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              _gimp_unit_get_digits (gimp, shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              (gimp_image_get_width (image) *
                               _gimp_unit_get_factor (gimp, shell->unit) /
                               xres));
                  break;
                }
              /* else fallthru */
            case 'w': /* width in pixels */
              i += print (title, title_len, i, "%d",
                          gimp_image_get_width (image));
              break;

            case 'H': /* height in real-world units */
              if (shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              _gimp_unit_get_digits (gimp, shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              (gimp_image_get_height (image) *
                               _gimp_unit_get_factor (gimp, shell->unit) /
                               yres));
                  break;
                }
              /* else fallthru */
            case 'h': /* height in pixels */
              i += print (title, title_len, i, "%d",
                          gimp_image_get_height (image));
              break;

            case 'u': /* unit symbol */
              i += print (title, title_len, i, "%s",
                          _gimp_unit_get_symbol (gimp, shell->unit));
              break;

            case 'U': /* unit abbreviation */
              i += print (title, title_len, i, "%s",
                          _gimp_unit_get_abbreviation (gimp, shell->unit));
              break;

              /* Other cool things to be added:
               * %r = xresolution
               * %R = yresolution
               * %� = image's fractal dimension
               * %� = the answer to everything
               */

            default:
              /* format string contains unknown %-sequence, print it literally */
              i += print (title, title_len, i, "%%%c", *format);
              break;
            }
          break;

        default:
          title[i++] = *format;
          break;
        }

      format++;
    }

  title[MIN (i, title_len - 1)] = '\0';

  return i;
}
