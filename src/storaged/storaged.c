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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/poll.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <libconfig.h>
#include <getopt.h>
#include <inttypes.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <netdb.h>

#include <rozofs/rozofs_srv.h>
#include <rozofs/common/log.h>
#include <rozofs/common/xmalloc.h>
#include <rozofs/common/list.h>
#include <rozofs/common/daemon.h>
#include <rozofs/common/profile.h>
#include <rozofs/common/common_config.h>
#include <rozofs/rpc/mproto.h>
#include <rozofs/rpc/sproto.h>
#include <rozofs/rpc/spproto.h>
#include <rozofs/core/rozofs_core_files.h>
#include <rozofs/core/rozofs_ip_utilities.h>
#include <rozofs/core/af_unix_socket_generic.h>
#include <rozofs/core/rozo_launcher.h>
#include <rozofs/core/rozofs_share_memory.h>
#include <rozofs/core/uma_dbg_api.h>
#include <rozofs/rozofs_timer_conf.h>

#include "storio_crc32.h"

#include "config.h"
#include "sconfig.h"
#include "storage.h"
#include "storaged.h"
#include "sconfig.h"
#include "storaged_nblock_init.h"

#define STORAGED_PID_FILE "storaged"

static char storaged_config_file[PATH_MAX] = STORAGED_DEFAULT_CONFIG;

sconfig_t storaged_config;

static storage_t storaged_storages[STORAGES_MAX_BY_STORAGE_NODE] = { { 0 } };

#define MAX_STORAGED_HOSTNAMES 32
static char   storaged_hostname_buffer[512];
char *        pHostArray[MAX_STORAGED_HOSTNAMES]={0};

void parse_host_name(char * host) {
  int    nb_names=0;
  char * pHost;
  char * pNext;

  if (host == NULL) return;
  
  strcpy(storaged_hostname_buffer,host);
  pHost = storaged_hostname_buffer;
  while (*pHost=='/') pHost++;
  
  while (*pHost != 0) {
  
    pHostArray[nb_names++] = pHost;
    
    pNext = pHost;
    
    while ((*pNext != 0) && (*pNext != '/')) pNext++;
    if (*pNext == '/') {
      *pNext = 0;
      pNext++;
    }  

    pHost = pNext;
  }
  pHostArray[nb_names++] = NULL;  
}


static uint16_t storaged_nrstorages = 0;

static SVCXPRT *storaged_monitoring_svc = 0;

uint8_t storaged_nb_ports = 0;
uint8_t storaged_nb_io_processes = 0;

DEFINE_PROFILING(spp_profiler_t) = {0};


static int storaged_initialize() {
    int status = -1;
    list_t *p = NULL;
    DEBUG_FUNCTION;

    /* Initialize rozofs constants (redundancy) */
    rozofs_layout_initialize();

    storaged_nrstorages = 0;

    storaged_nb_io_processes = 1;
    
    storaged_nb_ports = storaged_config.io_addr_nb;

    /* For each storage on configuration file */
    list_for_each_forward(p, &storaged_config.storages) {
        storage_config_t *sc = list_entry(p, storage_config_t, list);
        /* Initialize the storage */
        if (storage_initialize(storaged_storages + storaged_nrstorages++,
                sc->cid, sc->sid, sc->root,
		sc->device.total,
		sc->device.mapper,
		sc->device.redundancy,
		-1,NULL) != 0) {
            severe("can't initialize storage (cid:%d : sid:%d) with path %s",
                    sc->cid, sc->sid, sc->root);
            goto out;
        }
    }

    status = 0;
out:
    return status;
}
/*
**____________________________________________________
*/
/*
  Allocate a device for a file
  
   @param st: storage context
*/
uint32_t storio_device_mapping_allocate_device(storage_t * st) {
  severe("storaged should not call storio_device_mapping_allocate_device");
  return -1;
}
#if 0
static void storaged_release() {
    DEBUG_FUNCTION;
    int i;
    list_t *p, *q;

    for (i = 0; i < storaged_nrstorages; i++) {
        storage_release(&storaged_storages[i]);
    }
    storaged_nrstorages = 0;

    // Free config

    list_for_each_forward_safe(p, q, &storaged_config.storages) {

        storage_config_t *s = list_entry(p, storage_config_t, list);
        free(s);
    }
}
#endif
storage_t *storaged_lookup(cid_t cid, sid_t sid) {
    storage_t *st = 0;
    DEBUG_FUNCTION;

    st = storaged_storages;
    do {
        if ((st->cid == cid) && (st->sid == sid))
            goto out;
    } while (st++ != storaged_storages + storaged_nrstorages);
    errno = EINVAL;
    st = 0;
out:
    return st;
}
storage_t *storaged_next(storage_t * st) {
    DEBUG_FUNCTION;

    if (storaged_nrstorages == 0) return NULL;
    if (st == NULL) return storaged_storages;

    st++;
    if (st < storaged_storages + storaged_nrstorages) return st;
    return NULL;
}


pid_t session_id=0;

static void on_stop() {
    DEBUG_FUNCTION;

    svc_exit();

    if (storaged_monitoring_svc) {
        svc_unregister(MONITOR_PROGRAM, MONITOR_VERSION);
        pmap_unset(MONITOR_PROGRAM, MONITOR_VERSION);
        svc_destroy(storaged_monitoring_svc);
        storaged_monitoring_svc = NULL;
    }

    /*
    ** now kill every sub process
    */
    rozofs_session_leader_killer(300000);
}

char storage_process_filename[NAME_MAX];

static void on_start() {
    char cmd[256];
    char pidfile[128];
    char * p;
    storaged_start_conf_param_t conf;
    int ret = -1;
    list_t   *l;
          
    DEBUG_FUNCTION;

    session_id = setsid();

    af_unix_socket_set_datagram_socket_len(128);
    storage_process_filename[0] = 0;

    // Initialization of the storage configuration
    if (storaged_initialize() != 0) {
        fatal("can't initialize storaged: %s.", strerror(errno));
        return;
    }

//    SET_PROBE_VALUE(nb_rb_processes, 0);

    SET_PROBE_VALUE(uptime, time(0));
    strncpy((char*) gprofiler.vers, VERSION, 20);
    SET_PROBE_VALUE(nb_io_processes, common_config.nb_disk_thread);
    
    // Create storio process(es)

    // Set monitoring values just for the master process
    //SET_PROBE_VALUE(io_process_ports[i],(uint16_t) storaged_storage_ports[i] + 1000);
    
    
    /*
    ** Create a share memory to get the device status for every SID
    */
    l = NULL;
    list_for_each_forward(l, &storaged_config.storages) {

      storage_config_t *sc = list_entry(l, storage_config_t, list);
      
      /*
      ** Allocate share memory to report device status
      */
      void * p = rozofs_share_memory_allocate_from_name(sc->root, STORAGE_MAX_DEVICE_NB*sizeof(storage_device_info_t));
      if (p) {
        memset(p,0,STORAGE_MAX_DEVICE_NB*sizeof(storage_device_info_t));
      }
    }
    
    
    conf.nb_storio = 0;
    /*
    ** Then start storio
    */
    if (common_config.storio_multiple_mode==0) {
      p = cmd;
      p += rozofs_string_append(p, "storio -i 0 -c ");
      p += rozofs_string_append(p,storaged_config_file);
      if (pHostArray[0] != NULL) {
        p += rozofs_string_append (p, " -H ");
	p += rozofs_string_append (p, pHostArray[0]);
        int idx=1;
	while (pHostArray[idx] != NULL) {
	  *p++ ='/';
	  p += rozofs_string_append(p , pHostArray[idx++]);
	}  
      }	
      
      storio_pid_file(pidfile, pHostArray[0], 0);
      
      // Launch storio
      ret = rozo_launcher_start(pidfile, cmd);
      if (ret !=0) {
        severe("rozo_launcher_start(%s,%s) %s",pidfile, cmd, strerror(errno));
      }

      conf.nb_storio++;
    }
    else {
      uint64_t  bitmask[4] = {0};
      uint8_t   cid,rank,bit; 
         
      /* For each storage on configuration file */
      l = NULL;
      list_for_each_forward(l, &storaged_config.storages) {
      
        storage_config_t *sc = list_entry(l, storage_config_t, list);
	cid = sc->cid;
	
        /* Is this storage already started */
	rank = (cid-1)/64;
	bit  = (cid-1)%64; 
	if (bitmask[rank] & (1ULL<<bit)) {
	  continue;
	}
	
	bitmask[rank] |= (1ULL<<bit);
	
	p = cmd;
	p += rozofs_string_append(p, "storio -i ");
	p += rozofs_u32_append(p,cid);
	p += rozofs_string_append(p, " -c ");
	p += rozofs_string_append(p,storaged_config_file);
	if (pHostArray[0] != NULL) {

          p += rozofs_string_append (p, " -H ");
	  p += rozofs_string_append (p, pHostArray[0]);

          int idx=1;
	  while (pHostArray[idx] != NULL) {
	    *p++ ='/';
	    p += rozofs_string_append(p , pHostArray[idx++]);
          }	
	}		
      
        storio_pid_file(pidfile, pHostArray[0], cid); 
	
        // Launch storio
	ret = rozo_launcher_start(pidfile, cmd);
	if (ret !=0) {
          severe("rozo_launcher_start(%s,%s) %s",pidfile, cmd, strerror(errno));
	}
        conf.nb_storio++;
      }
    }

    // Create the debug thread of the parent
    conf.instance_id = 0;
    /* Try to get debug port from /etc/services */    
    conf.debug_port = rozofs_get_service_port_storaged_diag();
    
    storaged_start_nb_th(&conf);
}

void usage() {

    printf("RozoFS storage daemon - %s\n", VERSION);
    printf("Usage: storaged [OPTIONS]\n\n");
    printf("   -h, --help\t\t\tprint this message.\n");
    printf("   -H, --host=storaged-host\tspecify the hostname to use for build pid name (default: none).\n");
    printf("   -c, --config=config-file\tspecify config file to use (default: %s).\n",
            STORAGED_DEFAULT_CONFIG);
    printf("   -C, --check\tthe storaged just checks the configuration and returns an exit status (0 when OK).\n");
}

int main(int argc, char *argv[]) {
    int c;
    int  justCheck=0; /* Run storaged. Not just a configuration check. */
    char pid_name[256];
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h'},
        { "config", required_argument, 0, 'c'},
        { "host", required_argument, 0, 'H'},
	{ "check", no_argument, 0, 'C'},
        { 0, 0, 0, 0}
    };

    /*
    ** Change local directory to "/"
    */
    if (chdir("/")!= 0) {}

    uma_dbg_thread_add_self("Main");
    
    uma_dbg_record_syslog_name("storaged");

    // Init of the timer configuration
    rozofs_tmr_init_configuration();

    while (1) {

        int option_index = 0;
        c = getopt_long(argc, argv, "hmCc:H:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {

            case 'h':
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'C':
                justCheck = 1;
                break;				
            case 'c':
                if (!realpath(optarg, storaged_config_file)) {
                    severe("storaged failed: %s %s\n", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;
            case 'H':
		parse_host_name(optarg);
                break;
            case '?':
                usage();
                exit(EXIT_SUCCESS);
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
                break;
        }
    }

    // Initialize the list of storage config
    sconfig_initialize(&storaged_config);
    
    // Read the configuration file
    if (sconfig_read(&storaged_config, storaged_config_file,0) != 0) {
        severe("Failed to parse storage configuration file: %s.\n",strerror(errno));
        goto error;
    }
    // Check the configuration
    if (sconfig_validate(&storaged_config) != 0) {
        severe("Inconsistent storage configuration file: %s.\n",strerror(errno));
        goto error;
    }
    
    /*
    ** read common config file
    */
    common_config_read(NULL);    

    /*
    ** init of the crc32c
    */
    crc32c_init(common_config.crc32c_generate,
                common_config.crc32c_check,
                common_config.crc32c_hw_forced);


    if (justCheck) {
      closelog();
      exit(EXIT_SUCCESS);      
    }

    char *pid_name_p = pid_name;
    if (pHostArray[0] != NULL) {
        char * pChar = pid_name_p;
	pChar += rozofs_string_append(pChar,STORAGED_PID_FILE);
	*pChar++ = '_'; 
	pChar += rozofs_string_append(pChar,pHostArray[0]);
	pChar += rozofs_string_append(pChar,".pid");	
    } else {
        char * pChar = pid_name_p;
	pChar += rozofs_string_append(pChar,STORAGED_PID_FILE);
	pChar += rozofs_string_append(pChar,".pid");	
    }
    
    no_daemon_start("storaged", common_config.nb_core_file, pid_name, on_start,
            on_stop, NULL);

    exit(EXIT_SUCCESS);
error:
    closelog();
    exit(EXIT_FAILURE);
}
