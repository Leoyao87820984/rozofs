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
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <arpa/inet.h>          
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <rozofs/core/uma_dbg_msgHeader.h>
#include <rozofs/core/rozofs_ip_utilities.h>
#include <rozofs/rozofs.h>

#define SILENT 1
#define NOT_SILENT 0

#define FIRST_PORT  9000
#define LAST_PORT  10000

#define DEFAULT_TIMEOUT 4

#define         MX_BUF (384*1024)
typedef struct  msg_s {
   UMA_MSGHEADER_S header;
   char            buffer[MX_BUF];
} MSG_S;
MSG_S msg;

#define MAX_CMD 1024
#define MAX_TARGET  128
int                 nbCmd=0;
const char      *   cmd[MAX_CMD];
uint32_t            nbTarget=0;
uint32_t            ipAddr[MAX_TARGET];
uint16_t            serverPort[MAX_TARGET];
int                 socketArray[MAX_TARGET];
uint32_t            period;
int                 allCmd;
const char      *   prgName;  
int                 timeout=DEFAULT_TIMEOUT;
char prompt[64];
/**
**   lnkdebug <IPADDR> <PORT>
*/
void syntax_display() {
  printf("\n%s - RozoFS %s\n\n", prgName, VERSION);
  printf("%s ([-i <nodes>] {-p <NPorts>|-T <LPorts>})... [-c <cmd|all>]... [-f <cmd file>]... [-period <seconds>] [-t <seconds>]\n\n",prgName);
  printf("Several diagnostic targets can be specified ( [-i <nodes>] {-p <NPorts>|-T <LPorts>} )...\n");
  printf("    -i <nodes>     IP address or hostname of the diagnostic targets.\n");
  printf("                   When omitted in a target definition, the -i value of the previous target is used.\n");
  printf("                   or 127.0.0.1 when no previous -i option is set.\n");
  printf("                   A range or list of hostnames or IP addresses can be specified this way:\n");
  printf("                   - range: localhost:1-4 or 172.20.10.:21-28\n");
  printf("                   - list: localhost:1,3 or 172.20.10.:21,22,26\n");
  printf("       <nodes> = { <hosts> | <IP@> }\n");
  printf("       <hosts> = { <hostname> | <hostname>:<N>-<P> | <hostname>:<N>,..,<P> }\n");
  printf("       <IP@>   = { <a>.<b>.<c>.<N> | <a>.<b>.<c>.:<N>-<P> | <a>.<b>.<c>.:<N>,..,<P> }\n");
  printf("\n    -p <NPorts>    Numeric port number or list or range of numeric port values of the diagnostic targets.\n");
  printf("       <NPorts> = { <port> | <port1>,..,<portN> | <port1>-<portN> }\n");
  printf(" or\n");
  printf("    -T <LPorts>    The logical ports are given in the format:\n");
  printf("       export or export:0                for master export\n");
  printf("       export:<i>                        for slave export <i>\n");
  printf("       export:<i>,..,<j>                 for a list of export\n");
  printf("       export:<i>-<j>                    for a range of export\n");
  printf("       stspare                           for spare restorer\n");
  printf("       storaged or storio:0              for storaged\n");
  printf("       storio:<i>    or storio:<i>,..,<j>       or storio:<i>-<j>       for storios\n");
  printf("       mount:<i>     or mount:<i>,..,<j>        or mount:<i>-<j>        for rozofsmounts\n");
  printf("       mount:<i>:<j> or mount:<i>:<j>,..,<k>    or mount:<i>:<j>-<k>    for storclis\n");;
  printf("       rebalancer[:<instance>]\n");    
  printf("       rcmd\n");    
  printf(" At least one -p or -T value must be given.\n");
  printf("\nOptionnaly a list of command to run can be specified in command line mode:\n");
  printf("  [-c <cmd|all>]...\n"); 
  printf("         Every word after -c is interpreted as a word of a command until end of line or new option.\n");
  printf("         Several -c options can be set.\n");                 
  printf("         \"all\" is used to run all the commands the target knows.\n");    
  printf("  [-f <cmd file>]...\n");  
  printf("         The list of commands can be specified through some files.\n");
  printf("  [-period <seconds>]\n");       
  printf("         Periodicity in seconds (floating value) when running commands using -c or/and -f options.\n");  
  printf("\nMiscellaneous options:\n");
  printf("  -t <seconds>     Timeout value to wait for a response (default %d seconds).\n",DEFAULT_TIMEOUT);
  printf("  -reserved_ports  Displays model for port reservation\n");        
  printf("\ne.g:\n");
  printf("  Get the profiler counters of the 4 instances of storcli of RozoFS mountpoint 2.\n");
  printf("    %s -i 192.168.1.1 -T mount:2:1-4 -c profiler\n",prgName) ;          
  printf("  Get the profiler counters of the export slave 1 every 10 seconds and reset them.\n");
  printf("    %s -T export:1 -c profiler reset -period 10\n",prgName) ;          
  printf("  Get the throughput history of the 3 local storio .\n");
  printf("    %s -T storio:1-3 -c throughput\n",prgName) ;          
  printf("  Get the device statuses of the storio of some nodes.\n");
  printf("    %s -i rozofs-node:1,3 -T storio:1-3 -c device\n",prgName) ;          
  printf("  Speed sampling of the profiler information on rozofsmount instance 2\n");
  printf("    %s -i 192.168.2.21 -T mount:2 -c prof reset -period 0.1\n",prgName) ; 
  exit(0);
}
/*__________________________________________________________________________________
** Shutdown and close a socket
**
** @param idx The index of the socket identifier within socketArray array 
**
*/
void socket_shutdown(int idx) {
  if (socketArray[idx] < 0) return;
  
  shutdown(socketArray[idx],SHUT_RDWR);   
  close(socketArray[idx]);  
  socketArray[idx] = -1;   
}
void stop_on_error(char *fmt, ... ) {
  va_list         vaList;

  if (fmt != NULL) {
    /* Format the string */
    va_start(vaList,fmt);
    vprintf(fmt, vaList);
    va_end(vaList);
  }

  printf("Check syntax with: %s -h\n",prgName);
  exit(1);  
}
int debug_receive(int socketId, int silent) {
  int             ret;
  unsigned int    recvLen;
  int             firstMsg=1;
 
//  printf("\n...............................................\n");
  
  /* 
  ** Do a select before reading to be sure that a response comes in time
  */
  {
    fd_set fd_read;
    struct timeval to;
    
    to.tv_sec  = timeout;
    to.tv_usec = 0;

    FD_ZERO(&fd_read);
    FD_SET(socketId,&fd_read);
    
    ret = select(socketId+1, &fd_read, NULL, NULL, &to);
    if (ret != 1) {
      printf("Timeout %d sec\n",timeout);
      return 0;
    }
  }
  

  while (1) {


    recvLen = 0;
    char * p = (char *)&msg;
    while (recvLen < sizeof(UMA_MSGHEADER_S)) {
      ret = recv(socketId,p,sizeof(UMA_MSGHEADER_S)-recvLen,0);
      if (ret <= 0) {
        if (errno != 0) printf("error on recv1 %s",strerror(errno));
	return 0;
      }
      recvLen += ret;
      p += ret;
    }
    
    msg.header.len = ntohl(msg.header.len);    
    if (msg.header.len >= MX_BUF) {
      printf("Receive too big %d\n", msg.header.len);
      return 0;
    }
//    printf("FDL length %u\n",msg.header.len);
    recvLen = 0;
    while (recvLen < msg.header.len) {
      ret = recv(socketId,&msg.buffer[recvLen],msg.header.len-recvLen,0);
      if (ret <= 0) {
	    printf("error on recv2 %s",strerror(errno));
	    return 0;
      }
      recvLen += ret;
    }
    if (silent == NOT_SILENT) {
      char * pMsg = msg.buffer;
      if (nbCmd == 0) {
        while ((*pMsg != 0) && (*pMsg != '\n')) pMsg++;
        if (*pMsg == '\n') pMsg++;
      }
      else if (firstMsg) {
        firstMsg = 0;
        printf("%s", prompt);
      }  
      printf("%s", pMsg);
    }  
    if (msg.header.end) return 1;
  }
}

uint32_t readln(int fd,char *pbuf,uint32_t buflen)
{
   int len,lenCur = 0;
   for(;;)
   {
       len = read (fd, &pbuf[lenCur],1);
       if (len == 0)
       {
         return -1;
       }
       if (pbuf[lenCur] == '\n')
       {
         break;
       }
       lenCur++;
       if (lenCur == buflen)
       {
         lenCur--;
         break;
       }       
   }
   pbuf[lenCur] = 0;
   return lenCur+1;
}

void add_cmd_in_list(char * new_cmd, int len) {
  char * p;
  
  p = malloc(len+1);
  memcpy(p,new_cmd,len);
  p[len] = 0;
  cmd[nbCmd++] = (const char *) p;
}

void read_file(const char * fileName ) {
  uint32_t len;  
  int fd;
    
  fd = open(fileName, O_RDONLY); 
  if (fd < 0) {
    stop_on_error("File %s not found\n",fileName);
  } 
  
  while (nbCmd < MAX_CMD) {

    len = readln (fd, msg.buffer,sizeof(msg.buffer));
    if (len == (uint32_t)-1) break;

    add_cmd_in_list(msg.buffer, len);
  }
  
  close(fd);
} 
int debug_run_this_cmd(int socketId, const char * cmd, int silent) {
  uint32_t len,sent; 
   
  len = strlen(cmd)+1; 

  memcpy(msg.buffer ,cmd,len);  
  msg.header.len = htonl(len);
  msg.header.end = 1;
  msg.header.lupsId = 0;
  msg.header.precedence = 0;
  len += sizeof(UMA_MSGHEADER_S);

  sent = send(socketArray[socketId], &msg, len, 0);
  if (sent != len) {
    printf("send %s",strerror(errno));
    printf("%d sent upon %d\n", sent,len);
    socket_shutdown(socketId);
    return -1;
  }
    
  if (!debug_receive(socketArray[socketId],silent)) {
    printf("Diagnostic session abort\n");
    socket_shutdown(socketId);
    return -1;
  }  
  return 0;
  
}
#define SYSTEM_HEADER "system : "
int uma_dbg_read_prompt(int socketId, char * pr) {
  char *c = pr;
  char *pt = msg.buffer; 
    
  // Read the prompt
  if (debug_run_this_cmd(socketId, "who", SILENT) < 0)  return -1;
  
  // skip 1rst line
  while ((*pt != 0)&&(*pt != '\n')) pt++;
  
  if (*pt != 0) {

    pt++;
  
    if (strncmp(pt,SYSTEM_HEADER, strlen(SYSTEM_HEADER)) == 0) {

      pt += strlen(SYSTEM_HEADER);
      *c++ = ' ';
      while((*pt != '\n')&&(*pt != 0)) {
	*c++ = *pt++;
      }
    }
  }  
  *c++ = '>';
  *c++ = ' ';     
  *c = 0;
  return 0;
}
#define LIST_COMMAND_HEADER "List of available topics :"
int uma_dbg_read_all_cmd_list(int socketId) {
  char * p, * begin;
  int len;
    
  // Read the command list
  if (debug_run_this_cmd(socketId, "", SILENT) < 0)  return -1;

  nbCmd = 0;
  p = msg.buffer;
    
  if (strncmp(p,LIST_COMMAND_HEADER, strlen(LIST_COMMAND_HEADER)) != 0) return -1;  
  while(*p != '\n') p++;    
  p++;
    
  while (p) {
  
    // Skip ' '
    while (*p == ' ') p++;  
    
    // Is it the end
    if (strncmp(p,"exit", 4) == 0) break;
    
    // Read command in list
    begin = p;
    len = 0;
    while(*p != '\n') {
      len++;  
      p++;
    }
    add_cmd_in_list(begin, len);
    p++;
  }
  return 0;
}
void debug_interactive_loop(int socketId) {
//  char mycmd[1024]; 
  char *mycmd = NULL; 
//  int len;
//  int fd;

  uma_dbg_read_prompt(socketId,prompt + strlen(prompt)); 
   
//  fd = open("/dev/stdin", O_RDONLY);
  using_history();
  rl_bind_key('\t',rl_complete);   
  while (1) {

    printf("_________________________________________________________\n");
//    len = readln (fd, mycmd,sizeof(mycmd));
//    if (len == (uint32_t)-1) break;
    mycmd = readline (prompt);
    if (mycmd == NULL) break;
    if (strcasecmp(mycmd,"exit") == 0) {
      printf("Diagnostic session end\n");
      break;
    }
    if (strcasecmp(mycmd,"q") == 0) {
      printf("Diagnostic session end\n");
      break;
    }
    if (strcasecmp(mycmd,"quit") == 0) {
      printf("Diagnostic session end\n");
      break;
    }
    if ((mycmd[0] != 0) && (strcasecmp(mycmd,"!!") != 0)) {
       add_history(mycmd);
    }
    if (debug_run_this_cmd(socketId, mycmd, NOT_SILENT) < 0)  break;
    free(mycmd);
  }
  if (mycmd != NULL) free(mycmd);
//  close(fd);  
} 
void debug_run_command_list(int socketId) {
  int idx;  

  for (idx=0; idx < nbCmd; idx++) {
//    printf("_________________________________________________________\n");
//    printf("< %s >\n", cmd[idx]);  
    if (debug_run_this_cmd(socketId, cmd[idx], NOT_SILENT) < 0)  break;
  }
} 
/*
**_______________________________________________________________________
** Scan for a a host in the formmat 
**
** <hostname>
** or @IP
** or <hostname>:N-P 
** or <hostname>:N,...,P
** or x.y.z.:N-P
** or x.y.z.:N,...,P
**
** @param str      The string that contains the host
** @param ip       The returned array of IP addresses
**
** @retval         The number of IP addresses
*/
static inline int scan_host(char * hostStr, uint32_t * ip) {
  uint32_t  val1,val2;
  int       ret;
  char      hostname[128];
  int       nbHost = 0;   
  char    * str = hostStr;
  int       idx;
  
   
  while ((*str != 0)&&(*str != ':')) str++;
  
  /*
  ** Only one host 
  */
  if (*str == 0) {
    if (rozofs_host2ip_netw(hostStr,ip)<0) {
      return 0;
    }
    return 1;
  }
  
  *str = 0;
  str++;
  if (*str == 0) return 0;
    
  ret = sscanf(str,"%u",&val1);
  if (ret != 1) return 0;

  while ((*str != '-')&&(*str != ',')&&(*str != 0)) str++;
  if (*str == 0) return 0;
  
  /*
  ** List
  */
  if (*str == ',') {

    sprintf(hostname,"%s%d",hostStr,val1);
    if (rozofs_host2ip_netw(hostname,ip)<0) {
      return 0;
    }
    nbHost++;
    ip++;
  
    while (*str != 0) {
    
      str++;
      if (*str == 0) return 0;
    
      ret = sscanf(str,"%u",&val1);
      if (ret != 1) return 0;
    
      sprintf(hostname,"%s%d",hostStr,val1);
      if (rozofs_host2ip_netw(hostname,ip)<0) {
        return 0;
      }
      nbHost++;
      ip++;
      
      while ((*str != ',')&&(*str != 0)) str++;
    }
    return nbHost;
  }
  
  /*
  ** Range
  */  
  str++;
  if (*str == 0) return 0;
  ret = sscanf(str,"%u",&val2);
  if (ret != 1) return 0;
  if (val2 < val1) return -1;
  
  nbHost = (val2-val1) + 1;
  if (nbHost > MAX_TARGET) return 0;
  
  for (idx=0; idx <nbHost; idx++) {
  
    sprintf(hostname,"%s%d",hostStr,idx+val1);
    if (rozofs_host2ip_netw(hostname,ip)<0) {
      return 0;
    }
    ip++;    
  }
  return nbHost;
  
}
/*
** Scan for either x  x-y x,y,..
*/
static inline int scan_ports(char * str, uint32_t * values) {
  int      nbValues = 0;
  uint32_t val2;
  int      ret;
  int      idx;
  
  while (*str == ' ') str++;
  
  ret = sscanf(str,"%u",values);
  if (ret != 1) return 0;
  nbValues = 1;

  while ((*str != 0) && (*str != ',') && (*str != '-') && (*str != ':')) str++;
  if (*str == 0) return 1;
  if (*str == ':') return nbValues;

  if (*str == '-') {
    str++;
    if (*str == 0) return 0;
    
    ret = sscanf(str,"%u",&val2);
    if (ret != 1) return 0;
    
    if (val2 <= *values) return 0;  
    nbValues = (val2-*values) + 1;
    if (nbValues > MAX_TARGET) return 0;
    
    for (idx=1; idx<nbValues; idx++) {
      *(values+1) = (*values)+1;
      values++;
    }
    return nbValues;
  }
 
  values++;
  while (*str == ',') {
    str++;
    if (*str == 0) return nbValues;
  
    ret = sscanf(str,"%u",values);
    if (ret != 1) return 0;
    nbValues++;
    values++;
    while ((*str != 0) && (*str != ',')) str++;        
  }
  return nbValues;
  
}
void read_parameters(argc, argv)
int argc;
char *argv[];
{
  uint32_t            ret;
  uint32_t            idx;
  uint32_t            port32;
  uint32_t            val32;
  char              * pt;
  uint32_t            ports[MAX_TARGET];
  int                 nbPorts = 0;
  int                 localPort;
  int                 hostNb;
  int                 localIP;
  uint32_t            IPs[MAX_TARGET];

  /* Pre-initialize 1rst IP address */ 
  hostNb = scan_host("127.0.0.1",IPs); 
  
  /*
  ** Change local directory to "/"
  */
  if (chdir("/")!=0) {}
    
  idx = 1;
  if (1 == argc) syntax_display();
  
  /* Scan parameters */
  while (idx < argc) {

    /* -h */
    if (strcmp(argv[idx],"-h")==0) {
      syntax_display();
    }    

    /* -i <ipAddr> */
    if (strcmp(argv[idx],"-i")==0) {
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      hostNb = scan_host(argv[idx],IPs); 
      if (hostNb == 0) {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
      }
      idx++;
      continue;
    }
    
    /* -reserved_ports */
    if (strcmp(argv[idx],"-reserved_ports")==0) {
      int ret;
      char message[1024*4];
      show_ip_local_reserved_ports(message);
      printf("%s\n",message);
      printf("grep ip_local_reserved_ports /etc/sysctl.conf\n");
      ret = system("grep ip_local_reserved_ports /etc/sysctl.conf");
      printf("\ncat /proc/sys/net/ipv4/ip_local_reserved_ports\n");
      ret += system("cat /proc/sys/net/ipv4/ip_local_reserved_ports"); 
      exit(0);
    }
    
    
    /* -p <portNumber> */
    if (strcmp(argv[idx],"-p")==0) {
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      nbPorts = scan_ports(argv[idx],ports);
      if (nbPorts <= 0) {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
      }	
      for (localIP=0;localIP<hostNb; localIP++) {
        for (localPort=0; localPort < nbPorts; localPort++) {
          ipAddr[nbTarget]     = IPs[localIP];        
          serverPort[nbTarget] = (uint16_t) ports[localPort];
          nbTarget++;
        }
      }  
      idx++;
      continue;
    }
    
    /* 
    ** storaged               : -f storaged
    ** storio                 : -f storio[:<instance>]
    ** rozofsmount            : -f mount[:<mount instance>]
    ** storcli of rozofsmount : -f mount[:<mount instance>[:<1|2>]]
    ** geomgr                 : -f geomgr
    ** geocli                 : -f geocli[:<geocli instance>]
    ** storcli of geocli      : -f geocli[:<geocli instance>[:<1|2>]]
    */
    if (strcmp(argv[idx],"-T")==0) {
    
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      pt = argv[idx];
      
      /*
      ** storio:<idx>
      */
      if (strncasecmp(pt,"storio",strlen("storio"))==0) {
        port32 = 0;
	pt += strlen("storio");
	if (*pt == ':') {
          pt++;
          nbPorts = scan_ports(pt,ports);
          if (nbPorts <= 0) {
	    stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
          }	
          for (localIP=0;localIP<hostNb; localIP++) {
            for (localPort=0; localPort < nbPorts; localPort++) {
              ipAddr[nbTarget]     = IPs[localIP];        
              serverPort[nbTarget] = (uint16_t) rozofs_get_service_port_storio_diag(ports[localPort]);;
              nbTarget++;
            }
          }       
          idx++;
          continue;   
	}
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
      }
      
      /*
      ** mount:
      */
      if (strncasecmp(pt,"mount",strlen("mount"))==0) {
	pt += strlen("mount");
        if (*pt != ':') { 
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
	}  
	pt++;
        nbPorts = scan_ports(pt,ports);
        if (nbPorts <= 0) {
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
        }
        while ((*pt!=0) && (*pt!=':')) pt++;
        if (*pt == 0) {
          for (localIP=0;localIP<hostNb; localIP++) {
            for (localPort=0; localPort < nbPorts; localPort++) {
              ipAddr[nbTarget]     = IPs[localIP];        
              serverPort[nbTarget] = (uint16_t) rozofs_get_service_port_fsmount_diag(ports[localPort]);
              nbTarget++;
            }
          }   
          idx++;
          continue;                       
        } 
        
        if (nbPorts != 1) {   
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
        } 
        port32 = ports[0];               
	pt++;
        nbPorts = scan_ports(pt,ports);
        if (nbPorts <= 0) {
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
        }	
        for (localIP=0;localIP<hostNb; localIP++) {
          for (localPort=0; localPort < nbPorts; localPort++) {
            ipAddr[nbTarget]     = IPs[localIP];        
            serverPort[nbTarget] = (uint16_t)rozofs_get_service_port_fsmount_storcli_diag(port32,ports[localPort]);
            nbTarget++;
          }
        }             
        idx++;
        continue;            
      }
              
      /*
      ** storaged 
      */
      if (strncasecmp(pt,"storaged",strlen("storaged"))==0) {
        port32 = rozofs_get_service_port_storaged_diag();  
        nbPorts = 1;             
      }
      
      /*
      ** stspare
      */
      else if (strncasecmp(pt,"stspare",strlen("stspare"))==0) {
        port32 = rozofs_get_service_port_stspare_diag();
        nbPorts = 1;                     
      }     
      /*
      ** export
      */ 
      else if (strncasecmp(pt,"export",strlen("export"))==0) {
        pt += strlen("export");
	if (*pt == ':') {
          pt++;
          nbPorts = scan_ports(pt,ports);
          if (nbPorts <= 0) {
	    stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
          }
          for (localIP=0;localIP<hostNb; localIP++) {
            for (localPort=0; localPort < nbPorts; localPort++) {
              ipAddr[nbTarget]     = IPs[localIP];        
              serverPort[nbTarget] = (uint16_t)rozofs_get_service_port_export_slave_diag(ports[localPort]);
              nbTarget++;
            }
          }           	        
          idx++;
          continue;   
	}
	else {
          port32 = rozofs_get_service_port_export_master_diag();
          nbPorts = 1;             
        }	  	
      }    
      else if (strncasecmp(pt,"geomgr",strlen("geomgr"))==0) {
	port32 = rozofs_get_service_port_geomgr_diag();	
        nbPorts = 1;                       	
      }  
      else if (strncasecmp(pt,"geocli",strlen("geocli"))==0) {
	pt += strlen("geocli");  	
        if (*pt != ':') { 
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
	}  
	pt++;
	ret = sscanf(pt,"%u",&port32);
	if (ret != 1) {
	  stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
        }
	while ((*pt != 0)&&(*pt != ':')) pt++;
	if (*pt == 0) { 
	  port32 = rozofs_get_service_port_geocli_diag(port32);
	}    
	else { // geocli:x: ...
	  pt++;
	  ret = sscanf(pt,"%u",&val32);
	  if (ret != 1) {
	    stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
          }	
	  // storcli:x:y 
	  port32 = rozofs_get_service_port_geocli_storcli_diag(port32,val32);
          nbPorts = 1;                       	
	}
      }  

      else if (strncasecmp(pt,"rebalancer",strlen("rebalancer"))==0) {
      
	pt += strlen("rebalancer");
        
        if (*pt == 0) { 
	  port32 = rozofs_get_service_port_rebalancing_diag(0);
	}  
        else {
          if (*pt != ':') { 
	    stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
	  }  
	  pt++;
	  ret = sscanf(pt,"%u",&port32);
	  if (ret != 1) {
	    stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
          }
          port32 = rozofs_get_service_port_rebalancing_diag(port32);
          nbPorts = 1;                       	
	}
      }                
      else if (strncasecmp(pt,"rcmd",strlen("rcmd"))==0) {
        port32 = rozofs_get_service_port_export_rcmd_diag();
        nbPorts = 1;                       	
      }    
      else {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);       
      }

      if ((port32<0) || (port32>0xFFFF)) {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
      }

      for (localIP=0;localIP<hostNb; localIP++) {
        for (localPort=0; localPort < nbPorts; localPort++) {
          ipAddr[nbTarget]     = IPs[localIP];        
          serverPort[nbTarget] = (uint16_t)port32;;
          nbTarget++;
        }
      }           
      idx++;
      continue;
    }    
    
    /* -period <period> */
    if (strcmp(argv[idx],"-period")==0) {
      idx++;
      float fl;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      ret = sscanf(argv[idx],"%f",&fl);
      if (ret < 1) {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);   
      }
      period = fl * 1000000;
      idx++;
      continue;
    }
    
    /* -t <seconds> */
    if (strcmp(argv[idx],"-t")==0) {
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      ret = sscanf(argv[idx],"%u",&timeout);
      if (ret != 1) {
	stop_on_error ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);           
      }
      idx++;
      continue;
    }  
          
    /* -c <command> */
    if (strcmp(argv[idx],"-c")==0) {
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      if (strcmp(argv[idx],"all") == 0) {
        allCmd = 1;
	idx++;
        continue;
      }
      if (strcmp(argv[idx],"system") == 0) {
        int start = idx;
	int size  = 0;
        while (idx < argc) {
	  size += strlen(argv[idx])+1;
	  idx++;
	}
	if (size > 1) {
	  char * cmd = malloc(size+1);
	  char * p = cmd;
	  for  (;start<idx; start++) p += sprintf(p,"%s ", argv[start]);
	  *p = 0;
	  add_cmd_in_list(cmd,size);
	  free(cmd);
	}
        continue; 	
      }
      {
        int start = idx;
	int size  = 0;
        while (idx < argc) {
	  if (argv[idx][0] == '-') break;
	  size += strlen(argv[idx])+1;
	  idx++;
	}
	if (size > 1) {
	  char * cmd = malloc(size+1);
	  char * p = cmd;
	  for  (;start<idx; start++) p += sprintf(p,"%s ", argv[start]);
	  *p = 0;
	  add_cmd_in_list(cmd,size);
	  free(cmd);
	} 	  
      }	
      continue;
    }
        
    /* -f <fileName> */
    if (strcmp(argv[idx],"-f")==0) {
      idx++;
      if (idx == argc) {
	stop_on_error ("%s option but missing value !!!\n",argv[idx-1]);
      }
      read_file(argv[idx]);
      idx++;
      continue;
    }

    stop_on_error("Unexpected option \"%s\" !!!\n", argv[idx]);
  }  
  
}
int connect_to_server(uint32_t   ipAddr, uint16_t  serverPort) {
  int                 socketId;  
  struct  sockaddr_in vSckAddr;
  int                 sockSndSize = 256;
  int                 sockRcvdSize = 2*MX_BUF;
  /*
  ** now create the socket for TCP
  */
  if ((socketId = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Unable to create a socket !!!\n");
    exit(2);
  }  
  
  /* 
  ** change sizeof the buffer of socket for sending
  */
  if (setsockopt (socketId,SOL_SOCKET,
		  SO_SNDBUF,(char*)&sockSndSize,sizeof(int)) == -1)  {
    printf("Error on setsockopt SO_SNDBUF %d\n",sockSndSize);
    close(socketId);
    exit(2);
  }
  /* 
  ** change sizeof the buffer of socket for receiving
  */  
  if (setsockopt (socketId,SOL_SOCKET,
                  SO_RCVBUF,(char*)&sockRcvdSize,sizeof(int)) == -1)  {
    printf("Error on setsockopt SO_RCVBUF %d !!!\n",sockRcvdSize);
    close(socketId);
    exit(2);
  }
  

  /* Connect to the GG */
  vSckAddr.sin_family = AF_INET;
  vSckAddr.sin_port   = htons(serverPort);
  memcpy(&vSckAddr.sin_addr.s_addr, &ipAddr, 4); 
  if (connect(socketId,(struct sockaddr *)&vSckAddr,sizeof(struct sockaddr_in)) == -1) {
    printf("____[%u.%u.%u.%u:%u] error on connect %s!!!\n", 
            (unsigned int)ipAddr&0xFF, 
            (unsigned int)(ipAddr>>8)&0xFF, 
            (unsigned int) (ipAddr>>16)&0xFF, 
            (unsigned int) (ipAddr>>24)&0xFF, 
            (unsigned int)serverPort, 
            strerror(errno));
    return-1;
  }
  return socketId;
}
int main(int argc, const char **argv) {
  int                 socketId; 
  uint32_t            ip;
   
  prgName = argv[0];

  /* Read parametres */
  memset(serverPort,0,sizeof(serverPort)); 
  memset(ipAddr,0,sizeof(ipAddr));
  nbTarget = 0;
  period        = 0;
  nbCmd         = 0;
  allCmd        = 0;
  read_parameters(argc, argv);
  if (nbTarget == 0) stop_on_error("No target defined");

  memset(socketArray, -1, sizeof(socketArray)); 
reloop:

  for (socketId = 0; socketId < nbTarget; socketId++) {
   
    if (socketArray[socketId]  < 0) {
      socketArray[socketId] = connect_to_server(ipAddr[socketId],serverPort[socketId]);
    }
    if (socketArray[socketId]  < 0) {  
      continue;
    }
     
    ip = ntohl(ipAddr[socketId]);
    sprintf(prompt,"____[%u.%u.%u.%u:%d]",(ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF,ip&0xFF, serverPort[socketId]);
    //uma_dbg_read_prompt(socketId,p);
     
    if (allCmd) uma_dbg_read_all_cmd_list(socketId);

    if (nbCmd == 0) {
      debug_interactive_loop(socketId);
    }  
    else {
      debug_run_command_list(socketId);
    }
  }  
  
  if (period != 0) {
    usleep(period);
    goto reloop;
  }
  
  for (socketId = 0; socketId < nbTarget; socketId++) {
    socket_shutdown(socketId);
  }  
  exit(0);
}
