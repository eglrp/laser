/***
	*\author uplusplus
	*\date   20130507
	*\file HD_version.h
 	*  This header defines the current core version
 	*/

#ifdef _HD_VERSION_H
#define _HD_VERSION_H

#include "emap_plat_base.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** @name Version Number
 *  Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
 */
/*@{*/
#define HD_MAJOR_VERSION	0
#define HD_MINOR_VERSION	1
#define HD_PATCHLEVEL		0
/*@}*/

typedef struct hd_version {
	e_uint8 major;
	e_uint8 minor;
	e_uint8 patch;
} hd_version_t;

/**
 * This macro can be used to fill a version structure with the compile-time
 * version of the HD library.
 */
#define HD_VERSION(X)							\
{									\
	(X)->major = HD_MAJOR_VERSION;					\
	(X)->minor = HD_MINOR_VERSION;					\
	(X)->patch = HD_PATCHLEVEL;					\
}

/** This macro turns the version numbers into a numeric value:
 *  (1,2,3) -> (1203)
 *  This assumes that there will never be more than 100 patchlevels
 */
#define HD_VERSIONNUM(X, Y, Z)						\
	((X)*1000 + (Y)*100 + (Z))

/** This is the version number macro for the current HD version */
#define HD_COMPILEDVERSION \
	HD_VERSIONNUM(HD_MAJOR_VERSION, HD_MINOR_VERSION, HD_PATCHLEVEL)

/** This macro will evaluate to true if compiled with HD at least X.Y.Z */
#define HD_VERSION_ATLEAST(X, Y, Z) \
	(HD_COMPILEDVERSION >= HD_VERSIONNUM(X, Y, Z))

/** This function gets the version of the dynamically linked HD library.
 *  it should NOT be used to fill a version structure, instead you should
 *  use the HD_Version() macro.
 */
extern DECLSPEC const hd_version * DEV_EXPORT hd_linked_version(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _HD_VERSION_H */
