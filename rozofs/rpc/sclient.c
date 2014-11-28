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

#include <string.h>
#include <errno.h>

#include <rozofs/common/log.h>
#include <rozofs/rozofs_srv.h>

#include "sproto.h"
#include "rpcclt.h"
#include "sclient.h"

int sclient_initialize(sclient_t * sclt, struct timeval timeout) {
    int status = -1;
    DEBUG_FUNCTION;

    sclt->status = 0;

    if (rpcclt_initialize(&sclt->rpcclt, sclt->host, STORAGE_PROGRAM,
            STORAGE_VERSION, ROZOFS_RPC_BUFFER_SIZE, ROZOFS_RPC_BUFFER_SIZE,
            sclt->port, timeout) != 0) {
        // storageclt_release can change errno
        int xerrno = errno;
        //storageclt_release(clt);
        sclt->status = 0;
        errno = xerrno;
        goto out;
    }
    sclt->status = 1;

    status = 0;
out:
    return status;
}

// XXX Useless

void sclient_release(sclient_t * clt) {
    DEBUG_FUNCTION;
    if (clt && clt->rpcclt.client)
        rpcclt_release(&clt->rpcclt);
}

sp_write_ret_t * rbs_write(sp_write_arg_t *argp, CLIENT *clnt) {
	static sp_write_ret_t clnt_res;
        struct timeval TIMEOUT = { 25, 0 };
	
	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, SP_WRITE,
		(xdrproc_t) xdr_sp_write_arg_t, (caddr_t) argp,
		(xdrproc_t) xdr_sp_status_ret_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
int sclient_write_rbs(sclient_t * clt, cid_t cid, sid_t sid, uint8_t layout, uint32_t bsize,
        uint8_t spare, sid_t dist_set[ROZOFS_SAFE_MAX], fid_t fid, uint32_t bid,
        uint32_t nb_proj, const bin_t * bins, uint32_t rebuild_ref) {
    int status = -1;
    sp_write_ret_t *ret = 0;
    sp_write_arg_t args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid         = cid;
    args.sid         = sid;
    args.layout      = layout;
    args.spare       = spare;
    args.bsize       = bsize;
    args.rebuild_ref = rebuild_ref;
    memcpy(args.dist_set, dist_set, sizeof (sid_t) * ROZOFS_SAFE_MAX); 	
    memcpy(args.fid, fid, sizeof (uuid_t));
    args.bid         = bid;
    args.nb_proj     = nb_proj;
    args.bins.bins_len = nb_proj * (rozofs_get_max_psize(layout,bsize)
            * sizeof (bin_t) + sizeof (rozofs_stor_bins_hdr_t) + sizeof(rozofs_stor_bins_footer_t));
    args.bins.bins_val = (char *) bins;

    if (!(clt->rpcclt.client) ||
            !(ret = sp_write_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_write failed: no response from storage server"
                " (%s, %u, %u)", clt->host, clt->port, sid);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        severe("sclient_write failed: storage write response failure (%s)",
                strerror(errno));
        errno = ret->sp_write_ret_t_u.error;
        goto out;
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_write_ret_t, (char *) ret);
    return status;
}


int sclient_read_rbs(sclient_t * clt, cid_t cid, sid_t sid, uint8_t layout, uint32_t bsize,
        uint8_t spare, sid_t dist_set[ROZOFS_SAFE_MAX], fid_t fid, bid_t bid,
        uint32_t nb_proj, uint32_t * nb_proj_recv, bin_t * bins) {
    int status = -1;
    sp_read_ret_t *ret = 0;
    sp_read_arg_t args;

    DEBUG_FUNCTION;
    
    *nb_proj_recv = 0;

    // Fill request
    args.cid = cid;
    args.sid = sid;
    args.layout = layout;
    args.bsize = bsize;
    args.spare = spare;
    memcpy(args.dist_set, dist_set, sizeof (sid_t) * ROZOFS_SAFE_MAX);
    memcpy(args.fid, fid, sizeof (fid_t));
    args.bid = bid;
    args.nb_proj = nb_proj;

    if (!(clt->rpcclt.client) ||
            !(ret = sp_read_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_read_rbs failed: storage read failed "
                "(no response from storage server: %s)", clt->host);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_read_ret_t_u.error;
        goto out;
	
    }
    if (ret->sp_read_ret_t_u.rsp.bins.bins_len != 0) {
      // XXX ret->sp_read_ret_t_u.bins.bins_len is coherent
      // XXX could we avoid memcpy ??
      memcpy(bins, ret->sp_read_ret_t_u.rsp.bins.bins_val,
              ret->sp_read_ret_t_u.rsp.bins.bins_len);

      *nb_proj_recv = ret->sp_read_ret_t_u.rsp.bins.bins_len /
              ((rozofs_get_max_psize(layout,bsize) * sizeof (bin_t))
              + sizeof (rozofs_stor_bins_hdr_t) + sizeof (rozofs_stor_bins_footer_t));
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_read_ret_t, (char *) ret);
    return status;
}
int sclient_remove_rbs(sclient_t * clt, cid_t cid, sid_t sid, uint8_t layout, fid_t fid) {
    int status = -1;
    sp_status_ret_t *ret = 0;
    sp_remove_arg_t args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid = cid;
    args.sid = sid;
    args.layout = layout;
    memcpy(args.fid, fid, sizeof (fid_t));

    if (!(clt->rpcclt.client) ||
            !(ret = sp_remove_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_remove_rbs failed: storage remove failed "
                "(no response from storage server: %s)", clt->host);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_status_ret_t_u.error;
        if (errno != ENOENT) {
            severe("sclient_remove_rbs failed (error from %s): (%s)",
                    clt->host, strerror(errno));
            goto out;
        }
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_status_ret_t, (char *) ret);
    return status;
}
int sclient_remove_chunk_rbs(sclient_t * clt, cid_t cid, sid_t sid, uint8_t layout, uint8_t spare, uint32_t bsize,
                               sid_t dist_set[ROZOFS_SAFE_MAX], 
			      fid_t fid, int chunk, uint32_t ref) {
    int status = -1;
    sp_status_ret_t *ret = 0;
    sp_remove_chunk_arg_t args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid         = cid;
    args.sid         = sid;
    args.layout      = layout;
    args.bsize       = bsize;
    args.spare       = spare;
    memcpy(args.dist_set, dist_set, sizeof (sid_t) * ROZOFS_SAFE_MAX);
    memcpy(args.fid, fid, sizeof (fid_t));
    args.chunk       = chunk;
    args.rebuild_ref = ref;

    if (!(clt->rpcclt.client) ||
            !(ret = sp_remove_chunk_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_remove_chunk_rbs failed: storage remove failed "
                "(no response from storage server: %s)", clt->host);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_status_ret_t_u.error;
        if (errno != ENOENT) {
            severe("sclient_remove_chunk_rbs failed (error from %s): (%s)",
                    clt->host, strerror(errno));
            goto out;
        }
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_status_ret_t, (char *) ret);
    return status;
}
uint32_t sclient_rebuild_start_rbs(sclient_t * clt, cid_t cid, sid_t sid, fid_t fid, 
                                   sp_device_e device, uint8_t chunk, uint8_t spare,
				   uint64_t block_start, uint64_t block_stop) {
    uint32_t                ref = 0;
    sp_rebuild_start_ret_t *ret = 0;
    sp_rebuild_start_arg_t  args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid = cid;
    args.sid = sid;
    args.device    = device;
    args.chunk     = chunk; /* Only valid when device is SP_NEW_DEVICE */
    args.spare     = spare; /* Only valid when device is SP_NEW_DEVICE */    
    args.start_bid = block_start;
    args.stop_bid  = block_stop;
    memcpy(args.fid, fid, sizeof (fid_t));

    if (!(clt->rpcclt.client) ||
            !(ret = sp_rebuild_start_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_rebuild_start_rbs failed:"
                "(no response from storage server: %s)", clt->host);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_rebuild_start_ret_t_u.error;
        if (errno != ENOENT) {
            severe("sclient_rebuild_start_rbs failed (error from %s): (%s)",
                    clt->host, strerror(errno));
            goto out;
        }
    }
    ref = ret->sp_rebuild_start_ret_t_u.rebuild_ref;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_rebuild_start_ret_t, (char *) ret);
    return ref;
}
int sclient_rebuild_stop_rbs(sclient_t * clt, cid_t cid, sid_t sid, fid_t fid, uint32_t ref, sp_status_t result) {
    int status = -1;
    sp_rebuild_stop_ret_t *ret = 0;
    sp_rebuild_stop_arg_t args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid = cid;
    args.sid = sid;
    args.rebuild_ref = ref;
    args.status      = result;
    memcpy(args.fid, fid, sizeof (fid_t));

    if (!(clt->rpcclt.client) ||
            !(ret = sp_rebuild_stop_1(&args, clt->rpcclt.client))) {
        clt->status = 0;
        warning("sclient_rebuild_stop_rbs failed:"
                "(no response from storage server: %s)", clt->host);
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_rebuild_stop_ret_t_u.error;
        if (errno != ENOENT) {
            severe("sclient_rebuild_stop_rbs failed (error from %s): (%s)",
                    clt->host, strerror(errno));
            goto out;
        }
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_rebuild_stop_ret_t, (char *) ret);
    return status;
}
// XXX Never used yet

int storageclt_truncate(sclient_t * clt, cid_t cid, sid_t sid, uint8_t layout,
        uint8_t spare, sid_t dist_set[ROZOFS_SAFE_MAX], fid_t fid,
        tid_t proj_id, bid_t bid) {
    int status = -1;
    sp_status_ret_t *ret = 0;
    sp_truncate_arg_t args;

    DEBUG_FUNCTION;

    // Fill request
    args.cid = cid;
    args.sid = sid;
    args.layout = layout;
    args.spare = spare;
    memcpy(args.dist_set, dist_set, sizeof (sid_t) * ROZOFS_SAFE_MAX);
    memcpy(args.fid, fid, sizeof (fid_t));
    args.bid = bid;
    args.proj_id = proj_id;

    ret = sp_truncate_1(&args, clt->rpcclt.client);
    if (ret == 0) {
        errno = EPROTO;
        goto out;
    }
    if (ret->status != 0) {
        errno = ret->sp_status_ret_t_u.error;
        goto out;
    }
    status = 0;
out:
    if (ret)
        xdr_free((xdrproc_t) xdr_sp_status_ret_t, (char *) ret);
    return status;
}
