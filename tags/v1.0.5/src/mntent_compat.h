/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * mntent_compat.h
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * mntent_compat.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * mntent_compat.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with mntent_compat.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#else

#ifndef mntent_h_
#define mntent_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#define MOUNTED "mounted"
#define MNTTYPE_NFS "nfs"

//#define MOPTSLEN (256 - (MNAMELEN * 2 + MFSNAMELEN + 2 * sizeof(int)))

struct mntent {
    char *mnt_fsname;           /* file system name */
    char *mnt_dir;              /* file system path prefix */
    char *mnt_type;             /* dbg, efs, nfs */
    char *mnt_opts;             /* ro, hide, etc. */
    int mnt_freq;               /* dump frequency, in days */
    int mnt_passno;             /* pass number on parallel fsck */
};

FILE *setmntent(char *filep, char *type);
struct mntent *getmntent(FILE * filep);
//char * hasmntopt(struct mntent * mnt, char * opt);
int endmntent(FILE * filep);

#endif                          /* mntent_h_ */
#endif                          /* not HAVE_MNTENT_H */
