/*
 Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
 This file is part of Rozofs.

 Rozofs is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation, version 2.

 Rozofs is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see
 <http://www.gnu.org/licenses/>.
 */

#ifndef _XATTR_MAIN_H
#define _XATTR_MAIN_H

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "rozofs_ext4.h"
#include "xattr.h"


/*
 * Find the handler for the prefix and dispatch its get() operation.
 */
ssize_t
rozofs_getxattr(struct dentry *dentry, const char *name, void *buffer, size_t size);

/*
 * Combine the results of the list() operation from every xattr_handler in the
 * list.
 */
ssize_t
rozofs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size);

/*
 * Find the handler for the prefix and dispatch its set() operation.
 */
int
rozofs_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags);

/*
 * Find the handler for the prefix and dispatch its set() operation to remove
 * any associated extended attribute.
 */
int
rozofs_removexattr(struct dentry *dentry, const char *name);

/*
 * Find the handler for the prefix and dispatch its get() operation.
   Caution: the allocated buffer used for storing the extended attributes is not released by this function
 */
ssize_t
rozofs_getxattr_raw(struct dentry *dentry, const char *name, void *buffer, size_t size);

/*
**_____________________________________________________________________________
*/
/**
*  Get the block that contains an extended attribute in raw mode

   @param inode 
   @param buffer : buffer for storing extended attriute value
   @param buffer_size : size of the buffer
   
   @retval >=0 size on success
   @retval < 0 on error
*/
int
ext4_xattr_block_get_raw(lv2_entry_t *inode,void *buffer, u_int *buffer_size);
/*
**_____________________________________________________________________________
*/
/**
*  Get  all the extended attributes from the root inode

   @param inode 
   @param buffer : buffer for storing extended attributes
   @param buffer_size : size of the buffer
   
   @retval >=0 size on success
   @retval < 0 on error
*/
int
ext4_xattr_ibody_get_raw(lv2_entry_t *inode,void *buffer, u_int *buffer_size);

#endif
