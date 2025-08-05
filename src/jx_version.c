/**************************************************************************/
/*                                                                        */
/*  @file jx_version.c                                                    */
/*  @brief Version Information Source File (JsonX)                        */
/*                                                                        */
/*  Implements jx_version() function returning version string.            */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#include "jx_version.h"

/**************************************************************************/
/*                                                                        */
/*  Version API                                                           */
/*                                                                        */
/**************************************************************************/

const char *jx_get_version_string(void)
{
    return JX_VERSION_STRING_FULL;
}
