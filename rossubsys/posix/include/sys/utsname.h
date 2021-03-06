/* $Id: utsname.h,v 1.4 2002/10/29 04:45:23 rex Exp $
 */
/*
 * sys/utsname.h
 *
 * system name structure. Conforming to the Single UNIX(r) Specification
 * Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __SYS_UTSNAME_H_INCLUDED__
#define __SYS_UTSNAME_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct utsname
{
 char sysname[255];  /* name of this implementation of the operating system */
 char nodename[255]; /* name of this node within an implementation-dependent
                        communications network */
 char release[255];  /* current release level of this implementation */
 char version[255];  /* current version level of this release */
 char machine[255];  /* name of the hardware type on which the system is
                        running */
};

/* CONSTANTS */

/* PROTOTYPES */
int uname(struct utsname *);

/* MACROS */

#endif /* __SYS_UTSNAME_H_INCLUDED__ */

/* EOF */

