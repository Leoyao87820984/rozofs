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

#ifndef _ROZOFS_H
#define _ROZOFS_H

#include <stdint.h>
#include <uuid/uuid.h>

#include <config.h>

/**
* Ports definition of RozoFS
*/

#include "rozofs_service_ports.h"

#define P_COUNT     0
#define P_ELAPSE    1
#define P_BYTES     2

#define MICROLONG(time) ((unsigned long long)time.tv_sec * 1000000 + time.tv_usec)


#define ROZOFS_UUID_SIZE 16
/* Instead of using an array of unsigned char for store the UUID, we use an
 * array of uint32_t in the RPC protocol to use less space (see XDR). */
#define ROZOFS_UUID_SIZE_RPC (ROZOFS_UUID_SIZE/sizeof(uint32_t))
#define ROZOFS_UUID_SIZE_NET ROZOFS_UUID_SIZE_RPC
#define ROZOFS_HOSTNAME_MAX 128
/*
** The block size is dependant on the eid
**
** NOTE: THE EFFECTIVE LENGTH IN THE BLOCK HEADER ON DISK IS 16 BITS
**       LONG. THE MAXIMUM BLOCK SIZE IS SO (64K-1)
*/
typedef enum _ROZOFS_BSIZE_E {
  ROZOFS_BSIZE_4K,
  ROZOFS_BSIZE_8K,
  ROZOFS_BSIZE_16K,
  ROZOFS_BSIZE_32K,
} ROZOFS_BSIZE_E ;
#define ROZOFS_BSIZE_MIN        ROZOFS_BSIZE_4K
#define ROZOFS_BSIZE_MAX        ROZOFS_BSIZE_32K
#define ROZOFS_BSIZE_NB         (ROZOFS_BSIZE_MAX+1)
#define ROZOFS_BSIZE_BYTES(val) ((4*1024)<<val)
// Maximum number of block per message
#define ROZOFS_MAX_BLOCK_PER_MSG (256/4)
#define ROZOFS_SAFE_MAX 36
#define ROZOFS_SAFE_MAX_RPC  (ROZOFS_SAFE_MAX/sizeof(uint32_t))
/* Instead of using an array of sid_t for store the dist_set, we use an
 * array of uint32_t in the RPC protocol to use less space (see XDR). */
#define ROZOFS_SAFE_MAX_NET ROZOFS_SAFE_MAX_RPC
#define ROZOFS_DIR_SIZE 4096
#define ROZOFS_PATH_MAX 1024
#define ROZOFS_XATTR_NAME_MAX 255
#define ROZOFS_XATTR_VALUE_MAX 65536
#define ROZOFS_XATTR_LIST_MAX 65536
#define ROZOFS_FILENAME_MAX 255
/* Maximum file size (check for truncate) */
#define ROZOFS_FILESIZE_MAX 0x20000000000LL

/* Value for rpc buffer size used for sproto */
#define ROZOFS_RPC_STORAGE_BUFFER_SIZE (1024*300) 

#define ROZOFS_CLUSTERS_MAX 255 /**< FDL : limit for cluster */
/* Value max for a SID */
#define SID_MAX 255
/* Value min for a SID */
#define SID_MIN 1
/* Nb. max of storage node for one volume */
#define STORAGE_NODES_MAX 64
/* Nb. max of storaged ports on the same storage node */
#define STORAGE_NODE_PORTS_MAX 32
/* Nb. max of storages (couple cid:sid) on the same storage node */
#define STORAGES_MAX_BY_STORAGE_NODE 32
/* First TCP port used */
#define STORAGE_PORT_NUM_BEGIN 40000

#define ROZOFS_INODE_SZ  512  /**< rozofs inode size (memory and disk)*/
#define ROZOFS_NAME_INODE 128 /**< max size for object name in RPC message */
#define ROZOFS_NAME_INODE_RPC (ROZOFS_NAME_INODE/sizeof(uint32_t))
#define ROZOFS_XATTR_BLOCK_SZ 4096 /**< rozofs xattr block size */

#define MAX_DIR_ENTRIES 50
#define ROZOFS_MD5_SIZE 22
#define ROZOFS_MD5_NONE "0000000000000000000000"

#define ROZOFS_GEOREP_MAX_SITE 2 /**< max sites supported for geo-replication   */

#define EXPGW_EID_MAX_IDX 1024 /**< max number of eid  */
#define EXPGW_EXPGW_MAX_IDX 32 /**< max number of export gateway per exportd */
#define EXPGW_EXPORTD_MAX_IDX 64 /**< max number of exportd */

#define EXPORT_SLICE_PROCESS_NB 8 /**< number of processes for the slices */


/* Value max for an Exportd Gateway */
#define GWID_MAX 32
/* Value min for a Exportd Gateway */
#define GWID_MIN 1

#define SHAREMEM_PER_FSMOUNT_POWER2 1
#define SHAREMEM_PER_FSMOUNT (1<<SHAREMEM_PER_FSMOUNT_POWER2)
#define SHAREMEM_IDX_READ 0
#define SHAREMEM_IDX_WRITE 1
/* Timeout in seconds for storaged requests by mproto */
#define ROZOFS_MPROTO_TIMEOUT_SEC 4


/*
** Projection files on storages are split in chunks. Each chunk is allocated 
** a specific device on the storage.
*/


#define ROZOFS_STORAGE_FILE_MAX_SIZE             (8UL*1024*1024*1024*1024)    // 8 TiB
#define ROZOFS_STORAGE_MAX_CHUNK_PER_FILE        128
#define ROZOFS_STORAGE_NB_BLOCK_PER_CHUNK(bsize) (ROZOFS_STORAGE_FILE_MAX_SIZE/ROZOFS_STORAGE_MAX_CHUNK_PER_FILE/ROZOFS_BSIZE_BYTES(bsize))
#define ROZOFS_STORAGE_GET_CHUNK_NB(offset)     ((offset)/(ROZOFS_STORAGE_FILE_MAX_SIZE/ROZOFS_STORAGE_MAX_CHUNK_PER_FILE))


/**
* cluster state
*/
typedef enum {
  CID_DEPENDENCY_ST = 0,
  CID_UP_ST,
  CID_DOWNGRADED_ST,
  CID_DOWN_ST,
  CID_MAX_ST
} cid_state_e;

typedef enum {
    LAYOUT_2_3_4, LAYOUT_4_6_8, LAYOUT_8_12_16
} rozofs_layout_t;

typedef uint8_t tid_t; /**< projection id */
typedef uint64_t bid_t; /**< block id */
typedef uuid_t fid_t; /**< file id */
typedef uint8_t sid_t; /**< storage id */
typedef uint16_t cid_t; /**< cluster id */
typedef uint16_t vid_t; /**< volume id */
typedef uint32_t eid_t; /**< export id */

/**
*  type of the exportd attributes
*/
typedef enum
{
   ROZOFS_EXTATTR = 0, /**< extended attributes */
   ROZOFS_TRASH,   /**< pending trash */
   ROZOFS_REG,  /**< regular file & symbolic links */
   ROZOFS_DIR,     /**< directory    */
   ROZOFS_SLNK,    /**< name of symbolic link */
   ROZOFS_DIR_FID,     /**< directory rferenced by its fid  */

   ROZOFS_MAXATTR
} export_attr_type_e;

typedef union
{
   uint64_t fid[2];   /**<   */
   struct {
     uint64_t  vers:4;        /**< fid version */
     uint64_t  fid_high:43;   /**< highest part of the fid: not used */
     uint64_t  opcode:4;      /**< opcode used for metadata log */
     uint64_t  exp_id:3;      /**< exportd identifier: must remain unchanged for a given server */
     uint64_t  eid:10;        /**< export identifier */     
     uint64_t  usr_id:8;     /**< usr defined value-> for exportd;it is the slice   */
     uint64_t  file_id:40;    /**< bitmap file index within the slice                */
     uint64_t  idx:11;     /**< inode relative to the bitmap file index           */
     uint64_t  key:5;     /**< inode relative to the bitmap file index           */
   } s;
   struct {
     uint64_t  vers:4;        /**< fid version */
     uint64_t  fid_high:43;   /**< highest part of the fid: not used */
     uint64_t  opcode:4;      /**< opcode used for metadata log */
     uint64_t  exp_id:3;      /**< exportd identifier: must remain unchanged for a given server */
     uint64_t  eid:10;        /**< export identifier */     
     uint64_t  usr_id:8;     /**< usr defined value-> for exportd;it is the slice   */
     uint64_t  file_id:40;    /**< bitmap file index within the slice                */
     uint64_t  idx:11;     /**< inode relative to the bitmap file index           */
     uint64_t  key:5;     /**< inode relative to the bitmap file index           */
   } meta;
} rozofs_inode_t;

// storage stat

typedef struct sstat {
    uint64_t size;
    uint64_t free;
} sstat_t;


/**
 *  Header structure for one projection
 */
typedef union {
    uint64_t u64[2];

    struct {
        uint64_t timestamp : 64; ///<  time stamp.
        uint64_t effective_length : 16; ///<  effective length of the rebuilt block size: MAX is 64K.
        uint64_t projection_id : 8; ///<  index of the projection -> needed to find out angles/sizes: MAX is 255.
        uint64_t version : 8; ///<  version of rozofs. (not used yet)
        uint64_t filler : 32; ///<  for future usage.
    } s;
} rozofs_stor_bins_hdr_t;

typedef struct {
        uint64_t timestamp : 64; ///<  time stamp.
} rozofs_stor_bins_footer_t;



typedef struct child {
    char *name;
    fid_t fid;
    struct child *next;
} child_t;

#include "common/transform.h"
/**
* data structure related to optimized Mojette Usage
*/
typedef struct _projection_opt_t {
    bin_t *bins;
    int    projection_id;;
} projection_opt_t; 

typedef struct _trans_lk_table_t
{
   int min;
   int max;
   void *data[];
} trans_lk_table_t;

typedef void (*inverse_prog)(pxl_t *support,projection_opt_t * projections);
/**
* structure used to define the encoding rule of the lookup table
*/
typedef struct encode_t
{
  int nb_bits;   /**< number of bits that must be taken for the index */
  int nb_proj_per_grp;  /**< number of projections taken a each level */
  int nb_level;         /**< number of levels */
} encode_t;
/**
*  end of Mojette Optimized data structure
*/

/**
 *  By default the system uses 256 slices with 4096 subslices per slice
 */
#define MAX_SLICE_BIT 8
#define MAX_SLICE_NB (1<<MAX_SLICE_BIT)
#define MAX_SUBSLICE_BIT 12
#define MAX_SUBSLICE_NB (1<<MAX_SUBSLICE_BIT)
/*
 **__________________________________________________________________
 */
static inline void mstor_get_slice_and_subslice(fid_t fid, uint32_t *slice, uint32_t *subslice) {
    //uint32_t hash = 0;
    //uint8_t *c = 0;

    rozofs_inode_t *rozo_inode_p = (rozofs_inode_t*)fid;
     *subslice = 0;
     *slice = rozo_inode_p->s.usr_id & ((1 << MAX_SLICE_BIT) - 1);;
#if 0    
    for (c = fid; c != fid + 8; c++)
        hash = *c + (hash << 6) + (hash << 16) - hash;

    *slice = hash & ((1 << MAX_SLICE_BIT) - 1);
    hash = hash >> MAX_SLICE_BIT;
    *subslice = hash & ((1 << MAX_SUBSLICE_BIT) - 1);
#endif
}

/*
**__________________________________________________________________
*/
/**
*  Get the slice number from the upper part of the unique file id (fid)

  @param fid : unique file identifier
  
  @retval : slice value
*/
static inline void exp_trck_get_slice(fid_t fid, uint32_t *slice) {
    uint32_t hash = 0;
    uint8_t *c = 0;

    int i;
    rozofs_inode_t *rozo_inode_p = (rozofs_inode_t*)fid;
    c = (uint8_t*)&rozo_inode_p->fid[1];
    
    for (i= 0; i < sizeof(uint64_t); c++,i++)
        hash = *c + (hash << 6) + (hash << 16) - hash;

    *slice = hash & ((1 << MAX_SLICE_BIT) - 1);
    hash = hash >> MAX_SLICE_BIT;
}

/**
*  check if the slice is local to the export process
  
   @param slice : slice number
   
   @retval 1 : the slice is local
   @retval 0: the slice is not local
*/
static inline int exp_trck_is_local_slice(uint32_t slice)
{
   return 1;

}


/**
*  Generate a fake FID for rozoFS
  
   @param fid : pointer to the fid
   @param export_id : reference of the export host
   
   
   @retval 1 : the slice is local
   @retval 0: the slice is not local
*/
#define ROZOFS_FID_VERSION_0 0
static inline void rozofs_uuid_generate(fid_t fid,uint8_t export_id)
{
  rozofs_inode_t *fake_inode;

  fake_inode = (rozofs_inode_t*)fid;
  fake_inode->fid[0] = 0;
  fake_inode->fid[1] = 0;
  fake_inode->s.vers = ROZOFS_FID_VERSION_0;
  fake_inode->s.exp_id = export_id;
  
}



#endif
