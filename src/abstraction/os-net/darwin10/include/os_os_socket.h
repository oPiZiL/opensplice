/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR PrismTech
 *   Limited, its affiliated companies and licensors. All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#ifndef OS_DARWIN_SOCKET_H
#define OS_DARWIN_SOCKET_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <sys/select.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <ifaddrs.h>

/* Keep defines before common header */
#define OS_SOCKET_HAS_IPV6      1
#define OS_IFNAMESIZE           IF_NAMESIZE
#define OS_SOCKET_HAS_SA_LEN    1
#define OS_NO_SIOCGIFINDEX      1
#define OS_NO_NETLINK

#include "../common/include/os_socket.h"


#if defined (__cplusplus)
}
#endif

#endif /* OS_DARWIN_SOCKET_H */
