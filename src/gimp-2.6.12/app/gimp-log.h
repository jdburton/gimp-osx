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

#ifndef __GIMP_LOG_H__
#define __GIMP_LOG_H__


typedef enum
{
  GIMP_LOG_TOOL_EVENTS    = 1 << 0,
  GIMP_LOG_TOOL_FOCUS     = 1 << 1,
  GIMP_LOG_DND            = 1 << 2,
  GIMP_LOG_HELP           = 1 << 3,
  GIMP_LOG_DIALOG_FACTORY = 1 << 4,
  GIMP_LOG_MENUS          = 1 << 5,
  GIMP_LOG_SAVE_DIALOG    = 1 << 6,
  GIMP_LOG_IMAGE_SCALE    = 1 << 7,
  GIMP_LOG_SHADOW_TILES   = 1 << 8,
  GIMP_LOG_SCALE          = 1 << 9
} GimpLogFlags;


extern GimpLogFlags gimp_log_flags;


void   gimp_log_init (void);
void   gimp_log      (const gchar *function,
                      gint         line,
                      const gchar *domain,
                      const gchar *format,
                      ...) G_GNUC_PRINTF (4, 5);
void   gimp_logv     (const gchar *function,
                      gint         line,
                      const gchar *domain,
                      const gchar *format,
                      va_list      args);


#ifdef G_HAVE_ISO_VARARGS

#define GIMP_LOG(type, ...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (G_STRFUNC, __LINE__, #type, __VA_ARGS__); \
        } G_STMT_END

#elif defined(G_HAVE_GNUC_VARARGS)

#define GIMP_LOG(type, format...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (G_STRFUNC, __LINE__, #type, format); \
        } G_STMT_END

#else /* no varargs macros */

/* need to expand all the short forms
 * to make them known constants at compile time
 */
#define TOOL_EVENTS    GIMP_LOG_TOOL_EVENTS
#define TOOL_FOCUS     GIMP_LOG_TOOL_FOCUS
#define DND            GIMP_LOG_DND
#define HELP           GIMP_LOG_HELP
#define DIALOG_FACTORY GIMP_LOG_DIALOG_FACTORY
#define MENUS          GIMP_LOG_MENUS
#define SAVE_DIALOG    GIMP_LOG_SAVE_DIALOG
#define IMAGE_SCALE    GIMP_LOG_IMAGE_SCALE
#define SHADOW_TILES   GIMP_LOG_SHADOW_TILES
#define SCALE          GIMP_LOG_SCALE

#if 0 /* last resort */
#  define GIMP_LOG /* nothing => no varargs, no log */
#endif

static void
GIMP_LOG (GimpLogFlags flags,
          const gchar *format,
          ...)
{
  va_list args;
  va_start (args, format);
  if (gimp_log_flags & flags)
    gimp_logv ("", 0, "", format, args);
  va_end (args);
}

#endif  /* !__GNUC__ */

#endif /* __GIMP_LOG_H__ */
