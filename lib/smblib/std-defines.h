/*
 * Copyright (C) 1996-2020 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/*
   SMBlib Standard Includes

   Copyright (C) 1996, Richard Sharpe
*/

/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _SMBLIB_STD_DEFINES_H
#define _SMBLIB_STD_DEFINES_H

/* RFCNB Standard includes ... */
/* One day we will conditionalize these on OS types ... */

#define BOOL int

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

#endif /* _SMBLIB_STD_DEFINES_H */
