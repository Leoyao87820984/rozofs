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

/* need for crypt */
#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>  
#include <stdarg.h>    
#include <string.h>  
#include <strings.h>
#include <semaphore.h>
#include <pthread.h>
#include <config.h>

#include <rozofs/rozofs.h>
#include <rozofs/common/log.h>
#include <rozofs/common/profile.h>
#include <rozofs/common/common_config.h>
#include <rozofs/core/ruc_common.h>
#include <rozofs/core/ruc_sockCtl_api.h>
#include <rozofs/core/ruc_timer_api.h>
#include <rozofs/core/uma_tcp_main_api.h>
#include <rozofs/core/uma_dbg_api.h>
#include <rozofs/core/ruc_tcpServer_api.h>
#include <rozofs/core/ruc_tcp_client_api.h>
#include <rozofs/core/uma_well_known_ports_api.h>
#include <rozofs/core/af_unix_socket_generic_api.h>
#include <rozofs/core/north_lbg_api.h>
#include <rozofs/core/ruc_list.h>
#include <rozofs/core/af_unix_socket_generic_api.h>
#include <rozofs/core/rozofs_rpc_non_blocking_generic_srv.h>
#include <rozofs/core/rozofs_share_memory.h>
#include <rozofs/core/rozofs_ip_utilities.h>
#include <rozofs/rpc/eproto.h>
#include <rozofs/rpc/epproto.h>

#include "storaged_nblock_init.h"
#include "storaged_north_intf.h"
#include "storage.h"

uint32_t storio_nb = 0;
DECLARE_PROFILING(spp_profiler_t);

extern sconfig_t storaged_config;
extern char * pHostArray[];

int storaged_update_device_info(storage_t * st);
/*
 **_________________________________________________________________________
 *      PUBLIC FUNCTIONS
 **_________________________________________________________________________
 */


#define sp_display_probe(the_profiler, the_probe){\
  uint64_t rate;\
  uint64_t cpu;\
  if ((the_profiler.the_probe[P_COUNT] == 0) || (the_profiler.the_probe[P_ELAPSE] == 0) ){\
      cpu = rate = 0;\
  } else {\
      rate = (the_profiler.the_probe[P_COUNT] * 1000000 / the_profiler.the_probe[P_ELAPSE]);\
      cpu = the_profiler.the_probe[P_ELAPSE] / the_profiler.the_probe[P_COUNT];\
  }\
  *pChar++ = ' ';\
  pChar += rozofs_string_padded_append(pChar,16, rozofs_left_alignment,#the_probe);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,the_profiler.the_probe[P_COUNT]);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,rate);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,cpu);\
  pChar += rozofs_string_append(pChar," |\n");\
}

#define sp_clear_probe(the_profiler, the_probe)\
    {\
      the_profiler.the_probe[P_COUNT] = 0;\
      the_profiler.the_probe[P_ELAPSE] = 0;\
    }
    
#define sp_display_io_probe(the_profiler, the_probe)\
    {\
        uint64_t rate;\
        uint64_t cpu;\
        uint64_t throughput;\
        if ((the_profiler.the_probe[P_COUNT] == 0) || (the_profiler.the_probe[P_ELAPSE] == 0) ){\
            cpu = rate = throughput = 0;\
        } else {\
            rate = (the_profiler.the_probe[P_COUNT] * 1000000 / the_profiler.the_probe[P_ELAPSE]);\
            cpu = the_profiler.the_probe[P_ELAPSE] / the_profiler.the_probe[P_COUNT];\
            throughput = (the_profiler.the_probe[P_BYTES] / 1024 /1024 * 1000000 / the_profiler.the_probe[P_ELAPSE]);\
        }\
  *pChar++ = ' ';\
  pChar += rozofs_string_padded_append(pChar,17, rozofs_left_alignment,#the_probe);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,the_profiler.the_probe[P_COUNT]);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,rate);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,cpu);\
  pChar += rozofs_string_append(pChar," |");\
  pChar += rozofs_u64_padded_append(pChar,13, rozofs_right_alignment,throughput);\
  pChar += rozofs_string_append(pChar," |\n");\
}

static char * show_profile_storaged_master_display_help(char * pChar) {
  pChar += rozofs_string_append(pChar,"usage:\nprofiler reset       : reset statistics\nprofiler             : display statistics\n");  
  return pChar; 
}

static void show_profile_storaged_master_display(char * argv[], uint32_t tcpRef, void *bufRef) {
    char *pChar = uma_dbg_get_buffer();
    time_t elapse;
    int days, hours, mins, secs;
    time_t  this_time = time(0);


    // Compute uptime for storaged process
    elapse = (int) (this_time - gprofiler.uptime);
    days = (int) (elapse / 86400);
    hours = (int) ((elapse / 3600) - (days * 24));
    mins = (int) ((elapse / 60) - (days * 1440) - (hours * 60));
    secs = (int) (elapse % 60);
    pChar += sprintf(pChar, "GPROFILER version %s uptime =  %d days, %2.2d:%2.2d:%2.2d\n", gprofiler.vers,days, hours, mins, secs);

    // Print general profiling values for storaged
    pChar += rozofs_string_append(pChar, "storaged: ");
    pChar += rozofs_string_append(pChar, (char*)gprofiler.vers);
    pChar += rozofs_string_append(pChar, " - ");
    pChar += rozofs_u64_padded_append(pChar, 16, rozofs_right_alignment,gprofiler.nb_io_processes);
    pChar += rozofs_string_append(pChar, " IO process(es)\n");

    // Print header for operations profiling values for storaged
    pChar += rozofs_string_append(pChar, "                  |    CALL      | RATE(msg/s)  |   CPU(us)    |\n");
    pChar += rozofs_string_append(pChar, "------------------+--------------+--------------+--------------+\n");

    // Print master storaged process profiling values
    sp_display_probe(gprofiler, stat);
    sp_display_probe(gprofiler, ports);
    sp_display_probe(gprofiler, remove);
    sp_display_probe(gprofiler, list_bins_files);
    if (argv[1] != NULL) {

        if (strcmp(argv[1], "reset") == 0) {

            sp_clear_probe(gprofiler, stat);
            sp_clear_probe(gprofiler, ports);
            sp_clear_probe(gprofiler, remove);
            sp_clear_probe(gprofiler, list_bins_files);
	    pChar += sprintf(pChar,"Reset Done\n");  
	    gprofiler.uptime = this_time;  	      
        }
        else {
          pChar = show_profile_storaged_master_display_help(pChar);
        }
    }
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}

static void show_storio_nb(char * argv[], uint32_t tcpRef, void *bufRef) {
    char *pChar = uma_dbg_get_buffer();
    uint64_t  bitmask[4] = {0};
    list_t   *l = NULL;
    uint8_t   cid,rank,bit; 
          
    pChar +=  rozofs_string_append(pChar,"storio_nb : ");
    pChar +=  rozofs_u32_append(pChar,storio_nb);
    pChar +=  rozofs_string_append(pChar,"\nmode : ");
    if (common_config.storio_multiple_mode) {
      pChar +=  rozofs_string_append(pChar,"multiple");
    }  
    else {
      pChar +=  rozofs_string_append(pChar,"single");
    }      
    pChar += rozofs_string_append(pChar,"\ncids : ");
             
    /* For each storage on configuration file */
    list_for_each_forward(l, &storaged_config.storages) {

      storage_config_t *sc = list_entry(l, storage_config_t, list);
      cid = sc->cid;

      /* Is this storage already started */
      rank = (cid-1)/64;
      bit  = (cid-1)%64; 
      if (bitmask[rank] & (1ULL<<bit)) {
	continue;
      }

      bitmask[rank] &= (1ULL<<bit);
      pChar += rozofs_u32_append(pChar,cid);
      *pChar++ = ' ';
    }
    pChar += rozofs_eol(pChar);
   
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}
static void show_storage_device_status(char * argv[], uint32_t tcpRef, void *bufRef) {
    char                * pChar = uma_dbg_get_buffer();
    storage_t           * st=NULL;
    int                   device;
    storage_share_t     * share;
    uint32_t              period;
    
    while((st = storaged_next(st)) != NULL) {
      uint64_t sumfree=0;
      uint64_t sumsize=0;


      pChar += rozofs_string_append(pChar," ___ ___ ___ ________ ________ ________ ____ _____ ____ ______ _____ ______ _____ ________\n"); 
      pChar += rozofs_string_append(pChar,"| C | S | D | status |  free  |  max   |free| dev  |busy| rd  | Avg. | wr  | Avg. |  last  |\n");
      pChar += rozofs_string_append(pChar,"| I | I | E |        |  size  |  size  |  % | name |  % | /s  | rd   | /s  | wr   | access |\n");
      pChar += rozofs_string_append(pChar,"| D | D | V |        |        |        |    |      |    |     | usec |     | usec |  (sec) |\n");
        pChar += rozofs_string_append(pChar,"|___|___|___|________|________|________|____|______|____|_____|______|_____|______|________|\n");
      	
      /* 
      ** Resolve the share memory address|
      */
      share = storage_get_share(st);
      period = share->monitoring_period;
      if (period == 0) continue;     

      if (share != NULL) {
	for (device=0; device < st->device_number; device++) {
	  storage_device_info_t *pdev = &share->dev[device];
	  sumfree += pdev->free;
	  sumsize += pdev->size;
	  *pChar++ = '|';
	  pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, st->cid);
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, st->sid);
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, device);
	  pChar += rozofs_string_append(pChar," | ");
	  pChar += rozofs_string_padded_append(pChar, 7, rozofs_left_alignment, storage_device_status2string(pdev->status));
	  *pChar++ = '|';
	  pChar += rozofs_bytes_padded_append(pChar,7,pdev->free);
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_bytes_padded_append(pChar,7,pdev->size); 
	  pChar += rozofs_string_append(pChar," | ");
	  if (pdev->size==0) {
	    pChar += rozofs_string_append(pChar, "00");
	  }
	  else { 
	    pChar += rozofs_u32_padded_append(pChar, 2, rozofs_zero, pdev->free*100/pdev->size);
	  }	  
	  pChar += rozofs_string_append(pChar," |");
	  
	  if (pdev->devName==0) {
 	    pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, pdev->major);
	    *pChar++= '/';
	    pChar += rozofs_u32_padded_append(pChar, 3, rozofs_left_alignment, pdev->minor);
	  }
	  else {  
            pdev->devName[7] = 0,
            pChar += rozofs_string_padded_append(pChar, 6, rozofs_left_alignment, pdev->devName);
          }	  
	  pChar += rozofs_string_append(pChar,"|");	 
	  pChar += rozofs_u32_padded_append(pChar, 3, rozofs_right_alignment, pdev->usage);
	  pChar += rozofs_string_append(pChar," |");
	  uint32_t val = pdev->rdNb/period;
	  if (val<100) {
            pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, val);
	    *pChar++ = '.';
	    val = (pdev->rdNb-(val*period))*10/period;
            pChar += rozofs_u32_padded_append(pChar, 1, rozofs_left_alignment, val);
	  }
	  else {
  	    pChar += rozofs_u32_padded_append(pChar, 4, rozofs_right_alignment, val);
	  }  
	  
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_u32_padded_append(pChar, 5, rozofs_right_alignment, pdev->rdUs);
	  pChar += rozofs_string_append(pChar," |");
	  val = pdev->wrNb/period;
	  if (val<100) {
            pChar += rozofs_u32_padded_append(pChar, 2, rozofs_right_alignment, val);
	    *pChar++ = '.';
	    val = (pdev->wrNb-(val*period))*10/period;
            pChar += rozofs_u32_padded_append(pChar, 1, rozofs_left_alignment, val);
	  }
	  else {
  	    pChar += rozofs_u32_padded_append(pChar, 4, rozofs_right_alignment, val);
	  }
 	  
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_u32_padded_append(pChar, 5, rozofs_right_alignment, pdev->wrUs);
	  pChar += rozofs_string_append(pChar," |");
	  pChar += rozofs_u32_padded_append(pChar, 7, rozofs_right_alignment, pdev->lastActivityDelay);
	  pChar += rozofs_string_append(pChar," |");
	  
	    	  	   	  
	  if (pdev->diagnostic != DEV_DIAG_OK) {
            pChar += rozofs_string_append(pChar, storage_device_diagnostic2String(pdev->diagnostic));   
	  }  
            pChar += rozofs_eol(pChar);	 	  
	}
        pChar += rozofs_string_append(pChar,"|___|___|___|________|________|________|____|______|____|_____|______|_____|______|________|\n");
	

	pChar += rozofs_string_append(pChar,"                     |");
	pChar += rozofs_bytes_padded_append(pChar,7,sumfree);
	pChar += rozofs_string_append(pChar," |");
	pChar += rozofs_bytes_padded_append(pChar,7,sumsize);
	pChar += rozofs_string_append(pChar," | ");
	if (sumsize == 0) {	  	  
	  pChar += rozofs_string_append(pChar, "00");	
	} 
	else {
	  pChar += rozofs_u32_padded_append(pChar, 2, rozofs_zero, sumfree*100/sumsize);	  	  
	} 
	pChar += rozofs_string_append(pChar," |\n");
	

      } 
      pChar += rozofs_string_append(pChar,"                     |________|________|____|\n"); 
    }  
    uma_dbg_send(tcpRef, bufRef, TRUE, uma_dbg_get_buffer());
}
// For trace purpose
struct timeval Global_timeDay;
unsigned long long Global_timeBefore, Global_timeAfter;



/*
 **
 */

void fdl_debug_loop(int line) {
    int loop = 1;

    return;
    while (loop) {
        sleep(5);
        info("Fatal error on nb thread create (line %d) !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ", line);

    }


}

uint32_t ruc_init(uint32_t test, storaged_start_conf_param_t *arg_p) {
    int ret = RUC_OK;


    uint32_t mx_tcp_client = 2;
    uint32_t mx_tcp_server = 8;
    uint32_t mx_tcp_server_cnx = 10;
    uint32_t local_ip = INADDR_ANY;
    uint32_t        mx_af_unix_ctx = 1024;

    //#warning TCP configuration ressources is hardcoded!!
    /*
     ** init of the system ticker
     */
    rozofs_init_ticker();
    /*
     ** trace buffer initialization
     */
    ruc_traceBufInit();

    /*
     ** initialize the socket controller:
     **   for: NPS, Timer, Debug, etc...
     */
    //#warning set the number of contexts for socketCtrl to 1024
    ret = ruc_sockctl_init(1024);
    if (ret != RUC_OK) {
        fdl_debug_loop(__LINE__);
        fatal( " socket controller init failed" );
    }

    /*
     **  Timer management init
     */
    ruc_timer_moduleInit(FALSE);

    while (1) {
        /*
         **--------------------------------------
         **  configure the number of TCP connection
         **  supported
         **--------------------------------------   
         **  
         */
        ret = uma_tcp_init(mx_tcp_client + mx_tcp_server + mx_tcp_server_cnx);
        if (ret != RUC_OK) {
            fdl_debug_loop(__LINE__);
            break;
        }

        /*
         **--------------------------------------
         **  configure the number of TCP server
         **  context supported
         **--------------------------------------   
         **  
         */
        ret = ruc_tcp_server_init(mx_tcp_server);
        if (ret != RUC_OK) {
            fdl_debug_loop(__LINE__);
            break;
        }
        /*
        **--------------------------------------
        **  configure the number of AF_UNIX/AF_INET
        **  context supported
        **--------------------------------------   
        **  
        */    
        ret = af_unix_module_init(mx_af_unix_ctx,
                                  2,1024*1, // xmit(count,size)
                                  2,1024*1 // recv(count,size)
                                  );
        if (ret != RUC_OK) break;   
	        /*
         **--------------------------------------
         **   D E B U G   M O D U L E
         **--------------------------------------
         */

	if (pHostArray[0] != NULL) {
	  int idx=0;
	  while (pHostArray[idx] != NULL) {
	    rozofs_host2ip(pHostArray[idx], &local_ip);
	    uma_dbg_init(10, local_ip, arg_p->debug_port);	    
	    idx++;
	  }  
	}
	else {
	  local_ip = INADDR_ANY;
	  uma_dbg_init(10, local_ip, arg_p->debug_port);
	}  
        

        {
            char name[256];
	    if (pHostArray[0] == 0) {
	      sprintf(name, "storaged");
	    }
	    else {
              sprintf(name, "storaged %s", pHostArray[0]);
	    }  
            uma_dbg_set_name(name);
        }

        /*
        ** RPC SERVER MODULE INIT
        */
        ret = rozorpc_srv_module_init();
        if (ret != RUC_OK) break;         
        break;
    }

    //#warning Start of specific application initialization code


    return ret;
}

/*
 *_______________________________________________________________________
 */

/**
 *  This function is the entry point for setting rozofs in non-blocking mode

   @param args->ch: reference of the fuse channnel
   @param args->se: reference of the fuse session
   @param args->max_transactions: max number of transactions that can be handled in parallel
   
   @retval -1 on error
   @retval : no retval -> only on fatal error

 */
int storaged_start_nb_blocking_th(void *args) {
  int ret;
  storaged_start_conf_param_t *args_p = (storaged_start_conf_param_t*) args;

  ret = ruc_init(FALSE, args_p);
  if (ret != RUC_OK) {
    /*
     ** fatal error
     */
    fdl_debug_loop(__LINE__);
    fatal("can't initialize non blocking thread");
    return -1;
  }


  /*
  ** Init of the north interface (read/write request processing)
  */ 
  ret = storaged_north_interface_buffer_init(STORAGED_BUF_RECV_CNT, STORAGED_BUF_RECV_SZ);
  if (ret < 0) {
    fatal("Fatal error on storage_north_interface_buffer_init()\n");
    return -1;
  }
  ret = storaged_north_interface_init();
  if (ret < 0) {
    fatal("Fatal error on storage_north_interface_init()\n");
    return -1;
  }
   
  uma_dbg_addTopic_option("profiler", show_profile_storaged_master_display,UMA_DBG_OPTION_RESET);

    info("storaged non-blocking thread started "
            "(instance: %d, host: %s, port: %d).",
            args_p->instance_id, (pHostArray[0]==NULL)?"":pHostArray[0], args_p->debug_port);

    /*
     ** main loop
     */
    while (1) {
        ruc_sockCtrl_selectWait();
    }
    fatal("Exit from ruc_sockCtrl_selectWait()");
    fdl_debug_loop(__LINE__);
}

/*
 *_______________________________________________________________________
 */
/**
 *  This function is the entry point for setting rozofs in non-blocking mode

   @param args->ch: reference of the fuse channnel
   @param args->se: reference of the fuse session
   @param args->max_transactions: max number of transactions that can be handled in parallel
   
   @retval -1 on error
   @retval : no retval -> only on fatal error

 */
int storaged_start_nb_th(void *args) {
    int ret;
    storaged_start_conf_param_t *args_p = (storaged_start_conf_param_t*) args;

    ret = ruc_init(FALSE, args_p);
    if (ret != RUC_OK) {
        /*
         ** fatal error
         */
        fdl_debug_loop(__LINE__);
        fatal("ruc_init() can't initialize storaged non blocking thread");
        return -1;
    }


    /*
     ** Init of the north interface (read/write request processing)
     */
    ret = storaged_north_interface_buffer_init(STORAGED_BUF_RECV_CNT, STORAGED_BUF_RECV_SZ);
    if (ret < 0) {
        fatal("Fatal error on storaged_north_interface_buffer_init()\n");
        return -1;
    }
    ret = storaged_north_interface_init();
    if (ret < 0) {
        fatal("Fatal error on storaged_north_interface_init()\n");
        return -1;
    }

    /*
     ** add profiler subject 
     */
    uma_dbg_addTopic_option("profiler", show_profile_storaged_master_display,UMA_DBG_OPTION_RESET);
    
    storio_nb = args_p->nb_storio;
    uma_dbg_addTopic("storio_nb", show_storio_nb);
    uma_dbg_addTopic("device",show_storage_device_status);
    
    if (pHostArray[0] != NULL) {
        info("storaged non-blocking thread started (host: %s, dbg port: %d).",
                (pHostArray[0]==NULL)?"":pHostArray[0], args_p->debug_port);
    } else {
        info("storaged non-blocking thread started (dbg port: %d).", 
                args_p->debug_port);
    }

    /*
     ** main loop
     */
    while (1) {
        ruc_sockCtrl_selectWait();
    }
    fatal("Exit from ruc_sockCtrl_selectWait()");
    fdl_debug_loop(__LINE__);
}
