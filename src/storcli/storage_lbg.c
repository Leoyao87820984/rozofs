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



#include <rozofs/rozofs.h>
#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <rpc/pmap_clnt.h>
#include <unistd.h>

#include <rozofs/core/rozofs_socket_family.h>

#include <rozofs/rozofs.h>
#include <rozofs/common/log.h>
#include <rozofs/core/ruc_common.h>
#include <rozofs/core/ruc_sockCtl_api.h>
#include <rozofs/core/rozofs_tx_api.h>
#include <rozofs/core/north_lbg_api.h>
#include <rozofs/rpc/eclient.h>
#include <rozofs/rpc/rpcclt.h>
#include "rozofs_storcli.h"

static north_remote_ip_list_t my_list[STORAGE_NODE_PORTS_MAX];  /**< list of the connection for the exportd */

 /**
 *  socket configuration for the family
 */
static af_unix_socket_conf_t  af_inet_storaged_conf =
{
  1,  //           family: identifier of the socket family    */
  0,         /**< instance number within the family   */
  sizeof(uint32_t),  /* headerSize  -> size of the header to read                 */
  0,                 /* msgLenOffset->  offset where the message length fits      */
  sizeof(uint32_t),  /* msgLenSize  -> size of the message length field in bytes  */
  
  (1024*256), /*  bufSize        -> length of buffer (xmit and received)        */
  (300*1024), /*  so_sendbufsize -> length of buffer (xmit and received)        */
  rozofs_tx_userRcvAllocBufCallBack, /*  userRcvAllocBufCallBack -> user callback for buffer allocation             */
  rozofs_tx_recv_rpc_cbk,            /*  userRcvCallBack         -> callback provided by the connection owner block */
  rozofs_tx_xmit_abort_rpc_cbk,      /*  userDiscCallBack        ->callBack for TCP disconnection detection         */
  NULL,                              /* userConnectCallBack     -> callback for client connection only              */
  NULL,  //    userXmitDoneCallBack; /**< optional call that must be set when the application when to be warned when packet has been sent */
  NULL,  //    userRcvReadyCallBack; /* NULL for default callback                    */
  NULL,  //    userXmitReadyCallBack; /* NULL for default callback                    */
  NULL,  //    userXmitEventCallBack; /* NULL for default callback                    */
  rozofs_tx_get_rpc_msg_len_cbk,        /* userHdrAnalyzerCallBack ->NULL by default, function that analyse the received header that returns the payload  length  */
  ROZOFS_RPC_SRV,       /* recv_srv_type ---> service type for reception : ROZOFS_RPC_SRV or ROZOFS_GENERIC_SRV  */
  0,       /*   rpc_recv_max_sz ----> max rpc reception buffer size : required for ROZOFS_RPC_SRV only */

  NULL,  //    *userRef;           /* user reference that must be recalled in the callbacks */
  NULL,  //    *xmitPool; /* user pool reference or -1 */
  NULL   //    *recvPool; /* user pool reference or -1 */
};

int storcli_next_storio_global_index =0;

int storaged_lbg_initialize(mstorage_t *s) {
    int lbg_size;
    int ret;
    int i;
    
    DEBUG_FUNCTION;    
    
    /*
    ** configure the callback that is intended to perform the polling of the storaged on each TCP connection
    */
   ret =  north_lbg_attach_application_supervision_callback(s->lbg_id,(af_stream_poll_CBK_t)storcli_lbg_cnx_polling);
   if (ret < 0)
   {
     severe("Cannot configure Soraged polling callback");   
   }


   ret =  north_lbg_set_application_tmo4supervision(s->lbg_id,3);
   if (ret < 0)
   {
     severe("Cannot configure application TMO");   
   }   

    /*
    ** store the IP address and port in the list of the endpoint
    */
    lbg_size = s->sclients_nb;
    for (i = 0; i < lbg_size; i++)
    {
      my_list[i].remote_port_host   = s->sclients[i].port;
      my_list[i].remote_ipaddr_host = s->sclients[i].ipv4;
    }
     af_inet_storaged_conf.recv_srv_type = ROZOFS_RPC_SRV;
     af_inet_storaged_conf.rpc_recv_max_sz = rozofs_large_tx_recv_size;
         
     ret = north_lbg_configure_af_inet(s->lbg_id,
                                          s->host,
                                          INADDR_ANY,0,
                                          my_list,
                                          ROZOFS_SOCK_FAMILY_STORAGE_NORTH,lbg_size,&af_inet_storaged_conf);
     if (ret < 0)
     {
      severe("Cannot create Load Balancing Group %d for storaged %s",s->lbg_id,s->host);
      return -1;    
     }
     north_lbg_set_next_global_entry_idx_p(s->lbg_id,&storcli_next_storio_global_index);
     return  0;
}     

