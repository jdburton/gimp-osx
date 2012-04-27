/* gimpversion.h
 *
 * This is a generated file.  Please modify 'configure.in'
 */

#ifndef __GIMP_VERSION_H__
#define __GIMP_VERSION_H__

G_BEGIN_DECLS


#define GIMP_MAJOR_VERSION                              (2)
#define GIMP_MINOR_VERSION                              (6)
#define GIMP_MICRO_VERSION                              (12)
#define GIMP_VERSION                                    "2.6.12"
#define GIMP_API_VERSION                                "2.0"
#define GIMP_CHECK_VERSION(major, minor, micro) \
    (GIMP_MAJOR_VERSION > (major) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION > (minor)) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION == (minor) && \
      GIMP_MICRO_VERSION >= (micro)))


G_END_DECLS

#endif /* __GIMP_VERSION_H__ */
