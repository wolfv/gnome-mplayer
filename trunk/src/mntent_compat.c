/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * mntent_compat.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * mntent_compat.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * mntent_compat.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with mntent_compat.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MNTENT_H
#else
#include "mntent_compat.h"
#include <sys/param.h>
#ifdef HAVE_FSTAB_H
#include <fstab.h>
#endif

#if defined(__NetBSD__) && (__NetBSD_Version__ >= 299000900)
#include <sys/statvfs.h>
#define statfs statvfs
#define f_flags f_flag
#endif

struct statfs *getmntent_mntbufp;
int getmntent_mntcount = 0;
int getmntent_mntpos = 0;
char mntent_global_opts[256];
struct mntent mntent_global_mntent;

FILE *setmntent(char *filep, char *type)
{
    getmntent_mntpos = 0;
#ifdef HAVE_SYS_MOUNT_H
    getmntent_mntcount = getmntinfo(&getmntent_mntbufp, MNT_WAIT);
#endif
    return (FILE *) 1;          // dummy
}

void getmntent_addopt(char **c, const char *s)
{
    int i = strlen(s);
    *(*c)++ = ',';
    strcpy(*c, s);
    *c += i;
}

struct mntent *getmntent(FILE * filep)
{
#ifdef HAVE_SYS_MOUNT_H
    char *c = mntent_global_opts + 2;
    struct fstab *fst;
    if (getmntent_mntpos >= getmntent_mntcount)
        return 0;
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_RDONLY)
        strcpy(mntent_global_opts, "ro");
    else
        strcpy(mntent_global_opts, "rw");

    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_SYNCHRONOUS)
        getmntent_addopt(&c, "sync");
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOEXEC)
        getmntent_addopt(&c, "noexec");
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOSUID)
        getmntent_addopt(&c, "nosuid");
#ifdef MNT_NODEV
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NODEV)
        getmntent_addopt(&c, "nodev");
#endif
#ifdef MNT_UNION
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_UNION)
        getmntent_addopt(&c, "union");
#endif
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_ASYNC)
        getmntent_addopt(&c, "async");
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOATIME)
        getmntent_addopt(&c, "noatime");
#ifdef MNT_NOCLUSTERR
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOCLUSTERR)
        getmntent_addopt(&c, "noclusterr");
#endif
#ifdef MNT_NOCLUSTERW
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOCLUSTERW)
        getmntent_addopt(&c, "noclusterw");
#endif
#ifdef MNT_NOSYMFOLLOW
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_NOSYMFOLLOW)
        getmntent_addopt(&c, "nosymfollow");
#endif
#ifdef MNT_SUIDDIR
    if (getmntent_mntbufp[getmntent_mntpos].f_flags & MNT_SUIDDIR)
        getmntent_addopt(&c, "suiddir");
#endif
    mntent_global_mntent.mnt_fsname = getmntent_mntbufp[getmntent_mntpos].f_mntfromname;
    mntent_global_mntent.mnt_dir = getmntent_mntbufp[getmntent_mntpos].f_mntonname;
    mntent_global_mntent.mnt_type = getmntent_mntbufp[getmntent_mntpos].f_fstypename;
    mntent_global_mntent.mnt_opts = mntent_global_opts;
    if ((fst = getfsspec(getmntent_mntbufp[getmntent_mntpos].f_mntfromname))) {
        mntent_global_mntent.mnt_freq = fst->fs_freq;
        mntent_global_mntent.mnt_passno = fst->fs_passno;
    } else if ((fst = getfsfile(getmntent_mntbufp[getmntent_mntpos].f_mntonname))) {
        mntent_global_mntent.mnt_freq = fst->fs_freq;
        mntent_global_mntent.mnt_passno = fst->fs_passno;
    } else if (strcmp(getmntent_mntbufp[getmntent_mntpos].f_fstypename, "ufs") == 0) {
        if (strcmp(getmntent_mntbufp[getmntent_mntpos].f_mntonname, "/") == 0) {
            mntent_global_mntent.mnt_freq = 1;
            mntent_global_mntent.mnt_passno = 1;
        } else {
            mntent_global_mntent.mnt_freq = 2;
            mntent_global_mntent.mnt_passno = 2;
        }
    } else {
        mntent_global_mntent.mnt_freq = 0;
        mntent_global_mntent.mnt_passno = 0;
    }
    ++getmntent_mntpos;
    return &mntent_global_mntent;
#else
    return NULL;
#endif
}

int endmntent(FILE * filep)
{
    return 0;
}
#endif
