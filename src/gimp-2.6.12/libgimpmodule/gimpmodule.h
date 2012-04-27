/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmodule.h
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#ifndef __GIMP_MODULE_H__
#define __GIMP_MODULE_H__

#include <gmodule.h>

#include <libgimpmodule/gimpmoduletypes.h>

#include <libgimpmodule/gimpmoduledb.h>

G_BEGIN_DECLS


/*  increment the ABI version each time one of the following changes:
 *
 *  - the libgimpmodule implementation (if the change affects modules).
 *  - one of the classes implemented by modules (currently GimpColorDisplay,
 *    GimpColorSelector and GimpController).
 */
#define GIMP_MODULE_ABI_VERSION 0x0004


typedef enum
{
  GIMP_MODULE_STATE_ERROR,       /* missing gimp_module_register function
                                  * or other error
                                  */
  GIMP_MODULE_STATE_LOADED,      /* an instance of a type implemented by
                                  * this module is allocated
                                  */
  GIMP_MODULE_STATE_LOAD_FAILED, /* gimp_module_register returned FALSE
                                  */
  GIMP_MODULE_STATE_NOT_LOADED   /* there are no instances allocated of
                                  * types implemented by this module
                                  */
} GimpModuleState;


struct _GimpModuleInfo
{
  guint32  abi_version;
  gchar   *purpose;
  gchar   *author;
  gchar   *version;
  gchar   *copyright;
  gchar   *date;
};


typedef const GimpModuleInfo * (* GimpModuleQueryFunc)    (GTypeModule *module);
typedef gboolean               (* GimpModuleRegisterFunc) (GTypeModule *module);

/* GimpModules have to implement these */
G_MODULE_EXPORT const GimpModuleInfo * gimp_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               gimp_module_register (GTypeModule *module);


#define GIMP_TYPE_MODULE            (gimp_module_get_type ())
#define GIMP_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MODULE, GimpModule))
#define GIMP_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MODULE, GimpModuleClass))
#define GIMP_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MODULE))
#define GIMP_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MODULE))
#define GIMP_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MODULE, GimpModuleClass))


typedef struct _GimpModuleClass GimpModuleClass;

struct _GimpModule
{
  GTypeModule      parent_instance;

  /*< public >*/
  gchar           *filename;     /* path to the module                       */
  gboolean         verbose;      /* verbose error reporting                  */
  GimpModuleState  state;        /* what's happened to the module            */
  gboolean         on_disk;      /* TRUE if file still exists                */
  gboolean         load_inhibit; /* user requests not to load at boot time   */

  /* stuff from now on may be NULL depending on the state the module is in   */
  /*< private >*/
  GModule         *module;       /* handle on the module                     */

  /*< public >*/
  GimpModuleInfo  *info;         /* returned values from module_query        */
  gchar           *last_module_error;

  /*< private >*/
  GimpModuleQueryFunc     query_module;
  GimpModuleRegisterFunc  register_module;
};

struct _GimpModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (GimpModule *module);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType         gimp_module_get_type         (void) G_GNUC_CONST;

GimpModule  * gimp_module_new              (const gchar     *filename,
                                            gboolean         load_inhibit,
                                            gboolean         verbose);

gboolean      gimp_module_query_module     (GimpModule      *module);

void          gimp_module_modified         (GimpModule      *module);
void          gimp_module_set_load_inhibit (GimpModule      *module,
                                            gboolean         load_inhibit);

const gchar * gimp_module_state_name       (GimpModuleState  state);


#ifndef GIMP_DISABLE_DEPRECATED
GType         gimp_module_register_enum    (GTypeModule      *module,
                                            const gchar      *name,
                                            const GEnumValue *const_static_values);
#endif /* GIMP_DISABLE_DEPRECATED */


/*  GimpModuleInfo functions  */

GimpModuleInfo * gimp_module_info_new  (guint32               abi_version,
                                        const gchar          *purpose,
                                        const gchar          *author,
                                        const gchar          *version,
                                        const gchar          *copyright,
                                        const gchar          *date);
GimpModuleInfo * gimp_module_info_copy (const GimpModuleInfo *info);
void             gimp_module_info_free (GimpModuleInfo       *info);


G_END_DECLS

#endif  /* __GIMP_MODULE_H__ */
