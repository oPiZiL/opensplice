#ifndef U_SUBSCRIBERQOS_H
#define U_SUBSCRIBERQOS_H

#if defined (__cplusplus)
extern "C" {
#endif

#include "u_types.h"
#include "v_kernel.h"
#include "os_if.h"

#ifdef OSPL_BUILD_USER
#define OS_API OS_API_EXPORT
#else
#define OS_API OS_API_IMPORT
#endif
/* !!!!!!!!NOTE From here no more includes are allowed!!!!!!! */

OS_API v_subscriberQos u_subscriberQosNew    (v_subscriberQos tmpl);
OS_API u_result        u_subscriberQosInit   (v_subscriberQos q);
OS_API void            u_subscriberQosDeinit (v_subscriberQos q);
OS_API void            u_subscriberQosFree   (v_subscriberQos q);

#undef OS_API 

#if defined (__cplusplus)
}
#endif

#endif /* U_SUBSCRIBERQOS_H */