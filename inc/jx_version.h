/**************************************************************************/
/*                                                                        */
/*  @file jx_version.h                                                   */
/*  @brief Version Definitions for JsonX Library                          */
/*                                                                        */
/*  Contains constants and functions to query library version.            */
/*                                                                        */
/*  @author Mihail Zamurca                                               */
/*                                                                        */
/**************************************************************************/

#ifndef JX_VERSION_H
#define JX_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  Version Information                                                   */
/*                                                                        */
/**************************************************************************/

#define JX_VERSION_MAJOR     1
#define JX_VERSION_MINOR     0
#define JX_VERSION_PATCH     0

#define _JX_STRINGIFY(x)     #x
#define JX_STRINGIFY(x)      _JX_STRINGIFY(x)

#define JX_VERSION_STRING_RAW \
    "JsonX v" JX_STRINGIFY(JX_VERSION_MAJOR) "." JX_STRINGIFY(JX_VERSION_MINOR) "." JX_STRINGIFY(JX_VERSION_PATCH)

#define JX_VERSION_STRING_FULL \
    JX_VERSION_STRING_RAW " - (C) Mihail Zamurca, MIT Licensed"
/**************************************************************************/
/*                                                                        */
/*  API Functions                                                         */
/*                                                                        */
/**************************************************************************/

const char *jx_get_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* JX_VERSION_H */
