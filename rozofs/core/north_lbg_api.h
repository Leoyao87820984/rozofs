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


#ifndef NORTH_LBG_API_H
#define NORTH_LBG_API_H
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>

#include <rozofs/common/types.h>

#include "ruc_common.h"
#include "ruc_list.h"
#include "af_unix_socket_generic_api.h"
#include "af_unix_socket_generic.h"
#include "rozofs_socket_family.h"
#include "ppu_trace.h"
#include "uma_dbg_api.h"
#include "north_lbg.h"

/*
**____________________________________________________
*/
/**
* Check if there is some pending buffer in the global pending xmit queue

  @param lbg_p: pointer to a load balancing group
  @param entry_p: pointer to an element of a load balancing group
  @param xmit_credit : number of element that can be removed from the pending xmit list

  @retvak none;

*/
void north_lbg_poll_xmit_queue(north_lbg_ctx_t  *lbg_p, north_lbg_entry_ctx_t  *entry_p,int xmit_credit);

/*
**____________________________________________________
*/
/**
   north_lbg_module_init

  create the Transaction context pool

@param     : north_lbg_ctx_count  : number of Transaction context


@retval   : RUC_OK : done
@retval          RUC_NOK : out of memory
*/
uint32_t north_lbg_module_init(uint32_t north_lbg_ctx_count);

/*__________________________________________________________________________
*/
/**
*  API to allocate a  load balancing Group context with no configuration 
  
  Once the context is allocated, the state of the object is set to NORTH_LBG_DEPENDENCY.

 @param none
 
  @retval >= reference of the load balancer object
  @retval < 0 error (out of context ??)
*/
int north_lbg_create_no_conf();

/*__________________________________________________________________________
*/
/**
*  create a north load balancing object

  @param @name : name of the load balancer
  @param  basename_p : Base name of the remote sunpath
  @param @family of the load balancer
  @param first_instance: index of the first instance
  @param nb_instances: number of instances
  
  @retval >= reference of the load balancer object
  @retval < 0 error (out of context ??)
*/
int north_lbg_create_af_unix(char *name,
                             char *basename_p,
                             int family,
                             int first_instance,
                             int  nb_instances,
                             af_unix_socket_conf_t *conf_p);



/*__________________________________________________________________________
*/
/**
*  create a north load balancing object with AF_INET

  @param @name : name of the load balancer
  @param  basename_p : Base name of the remote sunpath
  @param @family of the load balancer

  @retval >= reference of the load balancer object
  @retval < 0 error (out of context ??)
*/


typedef struct _north_remote_ip_list_t
{
   uint32_t  remote_ipaddr_host; /**< IP address in host format  */
   uint16_t  remote_port_host;   /**< port in  host format       */
} north_remote_ip_list_t;


int north_lbg_create_af_inet(char *name,
                             uint32_t src_ipaddr_host,
                             uint16_t src_port_host,
                             north_remote_ip_list_t *remote_ip_p,
                             int family,int  nb_instances,af_unix_socket_conf_t *conf_p);


/*__________________________________________________________________________
*/ 
 /**
*  API to configure a load balancing group.
   The load balancing group must have been created previously with north_lbg_create_no_conf() 
  
 @param none
 
  @retval >= reference of the load balancer object
  @retval < 0 error (out of context ??)
*/
int north_lbg_configure_af_inet(int lbg_idx,char *name,
                                uint32_t src_ipaddr_host,
                                uint16_t src_port_host,
                                north_remote_ip_list_t *remote_ip_p,
                                int family,int  nb_instances,af_unix_socket_conf_t *conf_p);

/*__________________________________________________________________________
*/ 
 /**
*  API to re-configure the destination ports of a load balancing group.
   The load balancing group must have been configured previously with north_lbg_configure_af_inet() 
  
 @param lbg_idx index of the load balancing group
 @param remote_ip_p table of new destination ports
 @param nb_instances number of instances in remote_ip_p table
 
  @retval >= reference of the load balancer object
  @retval < 0 error (out of context ??)
*/
int north_lbg_re_configure_af_inet_destination_port(int lbg_idx,north_remote_ip_list_t *remote_ip_p, int  nb_instances);
/*__________________________________________________________________________
*/
/**
*  create a north load balancing object with AF_INET

  @param lbg_idx : reference of the load balancing group
  @param buf_p: pointer to the buffer to send

  retval 0 : success
  retval -1 : error
*/
int north_lbg_send(int  lbg_idx,void *buf_p);


/*__________________________________________________________________________
*/
/**
* Load Balncing group deletion API

  - delete all the TCP of AF_UNIX conections
  - stop the timer  assoicated with each connection
  - release all the xmit pending buffers associated with the load balancing group 

 @param lbg_id : user ereference of the load balancing group
 
 @retval 0 : success
 @retval < 0  errno (see errno for details)
*/
int  north_lbg_delete(int lbg_id);

/*__________________________________________________________________________
*/
/**
*  API to display the load balancing group id and its current state

  @param lbg_id : index of the load balancing group
  @param buffer : output buffer
  
  @retval : pointer to the next entry in the input buffer
*
*/
char *north_lbg_display_lbg_id_and_state(char * buffer,int lbg_id);


char *north_lbg_display_lbg_state(char * pchar,int lbg_id);



/*
**__________________________________________________________________________
*/
/**
*  Attach a supervision Application callback with the load balancing group
   That callback is configured on each entry of the LBG
   
   @param lbg_idx: reference of the load balancing group
   @param supervision_callback supervision_callback

  retval 0 : success
  retval -1 : error
*/
int  north_lbg_attach_application_supervision_callback(int lbg_idx,af_stream_poll_CBK_t supervision_callback);


/*
**__________________________________________________________________________
*/
/**
*  Configure the TMO of the application for connexion supervision
   
   @param lbg_idx: reference of the load balancing group
   @param tmo_sec : timeout value

  retval 0 : success
  retval -1 : error
*/
int  north_lbg_set_application_tmo4supervision(int lbg_idx,int tmo_sec);

int north_lbg_set_next_global_entry_idx_p(int lbg_idx, int * next_global_entry_idx_p);

/*__________________________________________________________________________
*/

int north_lbg_send_from_shaper(int  lbg_idx,void *buf_p);
/*__________________________________________________________________________
*/
/**
*  create a north load balancing object with AF_INET

  @param lbg_idx : reference of the load balancing group
  @param buf_p: pointer to the buffer to send
  @param rsp_size : expected response size in byte
  @param disk_time: estimated disk_time in us
  
  retval 0 : success
  retval -1 : error
*/
int north_lbg_send_with_shaping(int  lbg_idx,void *buf_p,uint32_t rsp_size,uint32_t disk_time)
;
#endif
