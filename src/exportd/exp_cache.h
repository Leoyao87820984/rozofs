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

#ifndef _EXP_CACHE_H
#define _EXP_CACHE_H


#include <rozofs/rozofs.h>
#include <rozofs/common/list.h>
#include <rozofs/common/htable.h>
#include <rozofs/common/mattr.h>
#include <rozofs/common/export_track.h>
#include <rozofs/rpc/eproto.h>
#include "mreg.h"
#include "mdir.h"
#include "mslnk.h"


#if 0
#define FILE_LOCK_POLL_DELAY_MAX  480

typedef struct _rozofs_file_lock_t {
  list_t           next_fid_lock;
  list_t           next_client_lock;
  struct ep_lock_t lock;
} rozofs_file_lock_t;


void                 lv2_cache_free_file_lock(rozofs_file_lock_t * lock) ;
rozofs_file_lock_t * lv2_cache_allocate_file_lock(ep_lock_t * lock) ;
#endif


/** API lv2 cache management functions.
 *
 * lv2 cache is common to several exports to take care of max fd opened.
 */

/** lv2 entry cached */
typedef struct lv2_entry {
    ext_mattr_t attributes; ///< attributes of this entry
    void        *extended_attr_p; /**< pointer to xattr array */
    void        *dirent_root_idx_p; /**< pointer to bitmap of the dirent root file presence : directory only */

    list_t list;        ///< list used by cache    
    union {
        mreg_t mreg;    ///< regular file
        mdir_t mdir;    ///< directory
        mslnk_t mslnk;  ///< symlink
    } container;
    /* 
    ** File locking
    */
    int            nb_locks;    ///< Number of locks on the FID
    list_t         file_lock;   ///< List of the lock on the FID
} lv2_entry_t;

/** lv2 cache
 *
 * used to keep track of open file descriptors and corresponding attributes
 */
typedef struct lv2_cache {
    int max;            ///< max entries in the cache
    int size;           ///< current number of entries
    uint64_t   hit;
    uint64_t   miss;
    uint64_t   lru_del;
    list_t entries;     ///< entries cached
    htable_t htable;    ///< entries hashing
} lv2_cache_t;


/**
*  Per export tracking table 
*/
typedef struct _export_tracking_table_t
{
  exp_trck_top_header_t *tracking_table[ROZOFS_MAXATTR];
} export_tracking_table_t;

#define ROZOFS_TRACKING_ATTR_SIZE 512

extern lv2_cache_t cache;


/*
**__________________________________________________________________
*/
/**
*   init of an exportd attribute cache

    @param: pointer to the cache context
    
    @retval none
*/
void lv2_cache_initialize(lv2_cache_t *cache);
/*
**__________________________________________________________________
*/
/**
*   delete of an exportd attribute cache

    @param: pointer to the cache context
    
    @retval none
*/
void lv2_cache_release(lv2_cache_t *cache);

/*
**__________________________________________________________________
*/
/**
*   delete an entry from the attribute cache

    @param cache: pointer to the level 2 cache
    @param fid : key of the entry to remove
*/
void lv2_cache_del(lv2_cache_t *cache, fid_t fid) ;
/*
**__________________________________________________________________
*/
/**
*   The purpose of that service is to read object attributes and store them in the attributes cache

  @param trk_tb_p: export attributes tracking table
  @param cache : pointer to the export attributes cache
  @param fid : unique identifier of the object
  
  @retval <> NULL: attributes of the object
  @retval == NULL : no attribute returned for the object (see errno for details)
*/

lv2_entry_t *lv2_cache_put(export_tracking_table_t *trk_tb_p,lv2_cache_t *cache, fid_t fid);

/*
**__________________________________________________________________
*/
/** search a fid in the attribute cache
 
 if fid is not cached, try to find it on the underlying file system
 and cache it.
 
  @param trk_tb_p: export tracking table
  @param cache: pointer to the cache associated with the export
  @param fid: the searched fid
 
  @return a pointer to lv2 entry or null on error (errno is set)
*/
lv2_entry_t *export_lookup_fid(export_tracking_table_t *trk_tb_p,lv2_cache_t *cache, fid_t fid);
/*
**__________________________________________________________________
*/
/** store the attributes part of an attribute cache entry  to the export's file system
 *
   @param trk_tb_p: export attributes tracking table
   @param entry: the entry used
 
   @return: 0 on success otherwise -1
 */
int export_lv2_write_attributes(export_tracking_table_t *trk_tb_p,lv2_entry_t *entry);
/*
**__________________________________________________________________
*/
/**
*  Create the attributes of a directory/regular file or symbolic link

  create an oject according to its type. The service performs the allocation of the fid. 
  It is assumed that all the other fields of the object attributes are already been filled in.
  
  @param trk_tb_p: export attributes tracking table
  @param slice: slice of the parent directory
  @param global_attr_p : pointer to the attributes of the object
  @param type: type of the object (ROZOFS_REG: regular file, ROZOFS_SLNK: symbolic link, ROZOFS_DIR : directory
  @param link: pointer to the symbolic link (significant for ROZOFS_SLNK only)
  
  @retval 0 on success: (the attributes contains the lower part of the fid that is allocated by the service)
  @retval -1 on error (see errno for details)
*/
int exp_attr_create(export_tracking_table_t *trk_tb_p,uint32_t slice,ext_mattr_t *global_attr_p,int type,char *link);
/*
**__________________________________________________________________
*/
/**
*  Create the extended attributes of a directory/regular file or symbolic link

  create an oject according to its type. The service performs the allocation of the fid. 
  It is assumed that all the other fields of the object attributes are already been filled in.
  
  @param trk_tb_p: export attributes tracking table
  @param entry : pointer to the inode and extended attributes of the inode (header)
  @param block_ref_p : pointer to the reference of the allocated block (Not Significant if retval is -1)
  
  @retval 0 on success: (the attributes contains the lower part of the fid that is allocated by the service)
  @retval -1 on error (see errno for details)
*/
int exp_xattr_block_create(export_tracking_table_t *trk_tb_p,lv2_entry_t *entry,uint64_t *block_ref_p);

/**
*  Create an entry in the trash for a file to delete

 
  
  @param trk_tb_p: export attributes tracking table
  @param slice: slice of the parent directory
  @param global_attr_p : pointer to the attributes relative to the object to delete
  @param link: pointer to the symbolic link (significant for ROZOFS_SLNK only)
  
  @retval 0 on success: (the attributes contains the lower part of the fid that is allocated by the service)
  @retval -1 on error (see errno for details)
*/
int exp_trash_entry_create(export_tracking_table_t *trk_tb_p,uint32_t slice,void *global_attr_p);

/*
**__________________________________________________________________
*/
/** store the extended attributes part of an attribute cache entry to the export's file system
 *
   @param trk_tb_p: export attributes tracking table
   @param entry: the entry used
 
   @return: 0 on success otherwise -1
 */
int export_lv2_write_xattr(export_tracking_table_t *trk_tb_p,lv2_entry_t *entry);

/*
**__________________________________________________________________
*/
/**
   read the extended attributes block from disk
  
   attributes can be the one of a regular file, symbolic link or a directory.
   The type of the object is indicated within the lower part of the fid (field key)
   
   @param trk_tb_p: export attributes tracking table
   @param entry : pointer to the array where attributes will be returned
   
   @retval 0 on success
   @retval -1 on error (see errno for details
*/
int exp_meta_get_xattr_block(export_tracking_table_t *trk_tb_p,lv2_entry_t *entry_p);
/*
**__________________________________________________________________
*/
/**
*    delete an inode associated with an object

   @param trk_tb_p: export attributes tracking table
   @param fid: fid of the object (key)
   
   @retval 0 on success
   @retval -1 on error
*/
int exp_attr_delete(export_tracking_table_t *trk_tb_p,fid_t fid);
/*
**__________________________________________________________________
*/
/**
*  Export tracking table create

   That service is called at export creation time. Its purpose is to allocate
   data structure for export attributes management.
   
   @param root_path : root path of the export
   @param create_flag : assert to 1 if tracking files MUST be created
   
   @retval <> NULL: pointer to the attributes tracking table
   @retval == NULL : error (see errno for details)
*/
export_tracking_table_t *exp_create_attributes_tracking_context(char *root_path, int create);
/*
**__________________________________________________________________
*/
/**
*  Export tracking table deletion

   That service is called at export creation time. Its purpose is to allocate
   data structure for export attributes management.
   
   @param tab_p  : pointer to the attributes tracking table 
   
   @retval none
*/
void exp_release_attributes_tracking_context(export_tracking_table_t *tab_p);

/*
**__________________________________________________________________
*/
/**
   read the attributes from disk
  
   attributes can be the one of a regular file, symbolic link or a directory.
   The type of the object is indicated within the lower part of the fid (field key)
   
   @param trk_tb_p: export attributes tracking table
   @param fid: unique file identifier
   @param entry : pointer to the array where attributes will be returned
   
   @retval 0 on success
   @retval -1 on error (see errno for details
*/
int exp_meta_get_object_attributes(export_tracking_table_t *trk_tb_p,fid_t fid,lv2_entry_t *entry_p);
#endif
