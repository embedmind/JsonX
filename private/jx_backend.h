/**************************************************************************/
/*                                                                        */
/*  @file jx_backend.h                                                    */
/*  @brief Internal backend adapter contract for JsonX                    */
/*                                                                        */
/*  This file keeps backend-specific parser/serializer types out of the   */
/*  JsonX parser core.                                                    */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#ifndef JX_BACKEND_H
#define JX_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "jx_types.h"

/**************************************************************************/
/*                                                                        */
/*  Backend API                                                           */
/*                                                                        */
/**************************************************************************/

void jx_backend_init_hooks(void *(*malloc_fn)(size_t size), void (*free_fn)(void *ptr));
void jx_backend_reset_hooks(void);
const char *jx_backend_get_error_ptr(void);

JX_STATUS jx_backend_parse_into_elements(char *buffer,
                                         JX_ELEMENT *elements,
                                         size_t element_count,
                                         JX_PARSE_MODE mode);
bool jx_backend_write_elements(JX_ELEMENT *elements,
                               size_t element_count,
                               char *buffer,
                               size_t buffer_size,
                               JX_FORMAT format);

#ifdef __cplusplus
}
#endif

#endif /* JX_BACKEND_H */
