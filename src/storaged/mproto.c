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

#include <limits.h>
#include <errno.h>

#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/profile.h>
#include <rozofs/common/common_config.h>
#include <rozofs/rpc/spproto.h>
#include <rozofs/rpc/mproto.h>
#include <rozofs/core/rozofs_rpc_non_blocking_generic_srv.h>
#include <rozofs/core/rozofs_share_memory.h>

#include "storage.h"
#include "storaged.h"
#include "sconfig.h"

#define MAX_STORAGED_CNX_TBL 128     
storage_t * st_per_cnx[MAX_STORAGED_CNX_TBL]={0};

static inline storage_t * get_storage(cid_t cid, sid_t sid, uint32_t cnx_id) {
  storage_t * st;

  /*
  ** Retrieve storage context used for this connection
  */
  if (cnx_id<MAX_STORAGED_CNX_TBL) {
    st = st_per_cnx[cnx_id];
    if ((st!=NULL) 
    &&  (st->cid == cid) 
    &&  (st->sid == sid)) return st;
  }
  
  /*
  ** Lookup for the storaage the request argument
  */
  st = storaged_lookup(cid, sid);
  
  /*
  ** Save the storage in the connection table
  */
  if (cnx_id<MAX_STORAGED_CNX_TBL) {
    st_per_cnx[cnx_id] = st;
  }
  
  return st;
}

DECLARE_PROFILING(spp_profiler_t);

void mp_null_1_svc_nb(void * pt_req, 
                       rozorpc_srv_ctx_t  * rozorpc_srv_ctx_p,
                       void * pt_resp, uint32_t cnx_id) { 
}

void mp_stat_1_svc_nb(void * pt_req, rozorpc_srv_ctx_t *rozorpc_srv_ctx_p,
        void * pt_resp, uint32_t cnx_id) {

    mp_stat_arg_t * args = (mp_stat_arg_t *) pt_req;
    mp_stat_ret_t * ret = (mp_stat_ret_t *) pt_resp;
    storage_t     * st = 0;
    uint64_t        ssize;
    uint64_t        sfree;
    int             device;
    storage_device_info_t *info;
    
    DEBUG_FUNCTION;

    START_PROFILING(stat);

    ret->status = MP_FAILURE;

    if ((st = storaged_lookup(args->cid, args->sid)) == 0) {
        ret->mp_stat_ret_t_u.error = errno;
        goto out;
    }

    sfree = 0;
    ssize = 0;
    
    /*
    ** Let's resolve the share memory address
    */
    if (st->info == NULL) {
      st->info = rozofs_share_memory_resolve_from_name(st->root);
    }	    
    info = st->info;
    if (info == NULL) {
      ret->mp_stat_ret_t_u.error = ENOENT;
      goto out;
    }    
    

    for (device=0; device < st->device_number; device++) {
      sfree += info[device].free;
      ssize += info[device].size;
    }  
    
    ret->mp_stat_ret_t_u.sstat.size = ssize;
    ret->mp_stat_ret_t_u.sstat.free = sfree;

    ret->status = MP_SUCCESS;

out:
    STOP_PROFILING(stat);
}

void mp_remove_1_svc_nb(void * pt_req, 
                        rozorpc_srv_ctx_t *rozorpc_srv_ctx_p,
                        void * pt_resp, 
			uint32_t cnx_id) {

    mp_status_ret_t * ret = (mp_status_ret_t *) pt_resp;
    mp_remove_arg_t * args = (mp_remove_arg_t*) pt_req;
    storage_t *st = 0;

    DEBUG_FUNCTION;

    START_PROFILING(remove);

    ret->status = MP_FAILURE;

    if ((st = storaged_lookup(args->cid, args->sid)) == 0) {
        ret->mp_status_ret_t_u.error = errno;
        goto out;
    }

    if (storage_rm_file(st, (unsigned char *) args->fid) != 0) {
        ret->mp_status_ret_t_u.error = errno;
        goto out;
    }

    ret->status = MP_SUCCESS;

out:
    STOP_PROFILING(remove);
}
void mp_remove2_1_svc_nb(void * pt_req, 
                         rozorpc_srv_ctx_t *rozorpc_srv_ctx_p,
			 void * pt_resp, 
			 uint32_t cnx_id) {

    mp_status_ret_t * ret = (mp_status_ret_t *) pt_resp;
    mp_remove2_arg_t * args = (mp_remove2_arg_t*) pt_req;
    storage_t *st = 0;

    DEBUG_FUNCTION;

    START_PROFILING(remove);

    ret->status = MP_FAILURE;

    if ((st = get_storage(args->cid, args->sid, cnx_id)) == 0) {
        ret->mp_status_ret_t_u.error = errno;
        goto out;
    }

    if (storage_rm2_file(st, (unsigned char *) args->fid, args->spare) != 0) {
        ret->mp_status_ret_t_u.error = errno;
        goto out;
    }

    ret->status = MP_SUCCESS;

out:
    STOP_PROFILING(remove);
}
void mp_ports_1_svc_nb(void * pt_req, 
                       rozorpc_srv_ctx_t *rozorpc_srv_ctx_p,
                       void * pt_resp, 
		       uint32_t cnx_id) {

    mp_ports_ret_t * ret = (mp_ports_ret_t *) pt_resp;

    START_PROFILING(ports);

    ret->status = MP_SUCCESS;

    memcpy(&ret->mp_ports_ret_t_u.ports.io_addr, storaged_config.io_addr,
            sizeof(storaged_config.io_addr));
	    
    if (common_config.storio_multiple_mode == 0) {
      ret->mp_ports_ret_t_u.ports.mode = MP_SINGLE;
    }    
    else {
      ret->mp_ports_ret_t_u.ports.mode = MP_MULTIPLE;
    }
    
    STOP_PROFILING(ports);
}

void mp_list_bins_files_1_svc_nb(void * pt_req,
                                 rozorpc_srv_ctx_t *rozorpc_srv_ctx_p, 
				 void * pt_resp, 
				 uint32_t cnx_id) {

    mp_list_bins_files_ret_t * ret = (mp_list_bins_files_ret_t *) pt_resp;
    mp_list_bins_files_arg_t * args = (mp_list_bins_files_arg_t*)  pt_req;

    storage_t *st = 0;

    ret->status = MP_FAILURE;

    START_PROFILING(list_bins_files);

    DEBUG_FUNCTION;

    if ((st = get_storage(args->cid, args->sid, cnx_id)) == 0) {
        ret->mp_list_bins_files_ret_t_u.error = errno;
        goto out;
    }

    // It's necessary
    memset(ret, 0, sizeof(mp_list_bins_files_ret_t));

    if (storage_list_bins_files_to_rebuild(st, args->rebuild_sid,
            &args->device,
            &args->spare,
	    &args->slice,
            &args->cookie,
            (bins_file_rebuild_t **)
            & ret->mp_list_bins_files_ret_t_u.reply.children,
            (uint8_t *) & ret->mp_list_bins_files_ret_t_u.reply.eof) != 0) {
      ret->mp_list_bins_files_ret_t_u.error = errno;
      goto out;
    }

    ret->mp_list_bins_files_ret_t_u.reply.cookie = args->cookie;
    ret->mp_list_bins_files_ret_t_u.reply.spare = args->spare;    
    ret->mp_list_bins_files_ret_t_u.reply.device = args->device;
    ret->mp_list_bins_files_ret_t_u.reply.slice = args->slice;

    ret->status = MP_SUCCESS;

out:
    STOP_PROFILING(list_bins_files);
}
