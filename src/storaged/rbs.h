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

#ifndef _RBS_H
#define _RBS_H

#include <stdint.h>
#include <limits.h>
#include <uuid/uuid.h>
#include <sys/param.h>

#include <rozofs/rozofs.h>
#include <rozofs/common/list.h>
#include <rozofs/rpc/sclient.h>
#include <rozofs/rpc/mclient.h>

#include "storage.h"
  


#define DEFAULT_REBUILD_RELOOP           4  

#define REBUILD_MSG(fmt, ...) { logmsg(EINFO, fmt, ##__VA_ARGS__); if (!quiet) printf(fmt"\n", ##__VA_ARGS__); }
#define REBUILD_FAILED(fmt, ...) { REBUILD_MSG("storage_rebuild failed !!!"); REBUILD_MSG(fmt, ##__VA_ARGS__); }

/* Timeout in seconds for exportd requests */
#define RBS_TIMEOUT_EPROTO_REQUESTS 25
/* Timeout in seconds for storaged requests with mproto */
#define RBS_TIMEOUT_MPROTO_REQUESTS 15
/* Timeout in seconds for storaged requests with sproto */
#define RBS_TIMEOUT_SPROTO_REQUESTS 10

typedef struct _rebuild_fid_input_t {
  fid_t      fid;
  int        cluster;
  int        layout;
  sid_t      dist[ROZOFS_SAFE_MAX];
} rebuild_fid_input_t;

typedef struct rb_entry {
    fid_t fid; ///< unique file identifier associated with the file
    uint8_t layout; ///< layout used for this file.
    uint32_t bsize; //< Block size as defined in ROZOFS_BSIZE_E
    sid_t dist_set_current[ROZOFS_SAFE_MAX]; ///< currents sids of storage nodes
    // target for this file.
    sclient_t * storages[ROZOFS_SAFE_MAX];
    list_t list;
} rb_entry_t;


/*
** Structures of rebuild counters
*/
typedef struct  _rozofs_rbs_counters_t {
  uint64_t      done_files;
  uint64_t      deleted;
  uint64_t      written;
  uint64_t      written_spare; 
  uint64_t      read;
  uint64_t      read_spare; 
} ROZOFS_RBS_COUNTERS_T;


typedef struct rb_stor {
    sid_t sid;
    char host[ROZOFS_HOSTNAME_MAX];
    mclient_t mclient;
    sclient_t sclients[STORAGE_NODE_PORTS_MAX];
    uint8_t sclients_nb;
    list_t list;
} rb_stor_t;

typedef struct rb_cluster {
    cid_t cid;
    list_t storages;
    list_t list;
} rb_cluster_t;


/** Get name of temporary rebuild directory
 *
 */
char * get_rebuild_directory_name(int rebuildRef) ;
char * get_rebuild_sid_directory_name(int rebuildRef, int cid, int sid, rbs_file_type_e ftype) ;



/*
** Storage information written in the storage configuration
** file by the storage_rebuild process and read by the storage
** list rebuilder processes
*/
typedef struct _rbs_storage_config_t {
  char          config_file[PATH_MAX];
  char          export_hostname[ROZOFS_HOSTNAME_MAX];
  int           site;
  storage_t     storage;
  uint8_t       device; 
  rbs_file_type_e ftype;
} rbs_storage_config_t;

/*_____________________________________________
** Create the storage configuration file
*/
static inline int rbs_write_storage_config_file(int rebuildRef, rbs_storage_config_t * st) {
  char           filename[FILENAME_MAX];
  char         * dir;  
  char         * pChar = filename;
  int            fd;
  int            ret;
        
  /*
  ** storage config file name
  */
  dir = get_rebuild_sid_directory_name(rebuildRef,st->storage.cid,st->storage.sid, st->ftype);
  pChar += rozofs_string_append(pChar,dir);
  pChar += rozofs_string_append(pChar,"/storage.conf");

  /*
  ** open
  */
  fd = open(filename, O_WRONLY| O_CREAT | O_TRUNC, 0755);
  if (fd < 0) {
	severe("open(%s) for writing %s\n",filename,strerror(errno));
	return -1;
  }
  /*
  ** write
  */
  ret = pwrite(fd,st,sizeof(*st),0);
  if (ret < 0) {
	severe("pwrite(%s) %s\n",filename,strerror(errno));
	close(fd);
	return -1;
  }	  
  
  close(fd);
//  info("storage.conf %s written",filename);  
  return 0;  
}
/*_____________________________________________
** Read the storage configuration file
*/
static inline rbs_storage_config_t * rbs_read_storage_config_file(char *path, rbs_storage_config_t * st) {
  char           filename[FILENAME_MAX];
  char         * pChar = filename;
  int            fd;
  int            ret;
        
  /*
  ** storage config file name
  */
  pChar += rozofs_string_append(pChar,path);
  pChar += rozofs_string_append(pChar,"/storage.conf");

  /*
  ** open
  */
  fd = open(filename, O_RDONLY);
  if (fd < 0) {
	severe("open(%s) for writing %s\n",filename,strerror(errno));
	return NULL;
  }
  /*
  ** read
  */
  ret = pread(fd,st,sizeof(*st),0);
  if (ret != sizeof(*st)) {
	severe("pread(%s) %s\n",filename,strerror(errno));
	close(fd);
	return NULL;
  }	  
  
  close(fd);
  return st;  
}
/*_____________________________________________
** Read the count file
*/
static inline uint64_t rbs_read_file_count(int rebuildRef, int cid, int sid, rbs_file_type_e ftype) {
  char       fileName[FILENAME_MAX];
  char     * dir;
  uint64_t   res = 0;
  
  dir = get_rebuild_sid_directory_name(rebuildRef, cid, sid, ftype);
  sprintf(fileName,"%s/count", dir);
  
  
  int fd = open(fileName,O_RDONLY);
  if (fd<0) {
	severe("Can not read %s %s",fileName,strerror(errno));
	errno = ENOENT;
	return 0;
  }
  if (read(fd,&res,sizeof(res))!= sizeof(res)){
	severe("Can not read %s %s",fileName,strerror(errno));
	errno = ENOENT;
	return 0;
  }
  
  close(fd);
  errno = 0;
  return res;
}  
/** Initialize connections (via mproto and sproto) to one storage
 *
 * @param rb_stor: storage to connect.
 *
 * @return: 0 on success -1 otherwise (errno is set)
 */
int rbs_stor_cnt_initialize(rb_stor_t * rb_stor, int cid) ;

/** Init connections for storage members of a given cluster but not for the 
 *  storage with sid=sid
 *
 * @param cluster_entries: list of cluster(s).
 * @param cid: unique id of cluster that owns this storage.
 * @param sid: the unique id for the storage to rebuild.
 * @param failed: number of failed server comprising the rebuilt server
 * @param available: number of available server
 *
 */
void rbs_init_cluster_cnts(list_t * cluster_entries, 
                          cid_t cid,
                          sid_t sid,
			  int * failed,
			  int * available);
	
int rbs_get_rb_entry_cnts(rb_entry_t * rb_entry,
        list_t * clusters_list,
        cid_t cid_to_search,
        sid_t sid_to_rebuild,
        uint8_t nb_cnt_required);

/** Release the list of cluster(s)
 *
 * @param cluster_entries: list of cluster(s).
 */
void rbs_release_cluster_list(list_t * cluster_entries);
/** Remove every file in a directory 
 *
 */
int rbs_empty_dir(char * dirname) ;
/** Check if the storage is present on cluster list
 *
 * @param cluster_entries: list of cluster(s).
 * @param cid: unique id of cluster that owns this storage.
 * @param sid: the unique id for the storage to rebuild.
 *
 * @return: 0 on success -1 otherwise (errno is set)
 */
int rbs_check_cluster_list(list_t * cluster_entries, cid_t cid, sid_t sid) ;
#endif
