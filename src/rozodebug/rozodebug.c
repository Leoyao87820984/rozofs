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

#define SILENT 1
#define NOT_SILENT 0

#define FIRST_PORT  9000
#define LAST_PORT  10000

#define DEFAULT_TIMEOUT 4

#define         MX_BUF (32*1024)
typedef struct  msg_s {
   UMA_MSGHEADER_S header;
   char            buffer[MX_BUF];
} MSG_S;
MSG_S msg;

#define MAX_CMD 1024
#define MAX_TARGET  20
int                 nbCmd=0;
const char      *   cmd[MAX_CMD];
uint32_t            nbTarget=0;
uint32_t            ipAddr[MAX_TARGET];
uint16_t            serverPort[MAX_TARGET];
uint32_t            period;
int                 allCmd;
const char      *   prgName;  
int                 timeout=DEFAULT_TIMEOUT;
char prompt[64];
/**
**   lnkdebug <IPADDR> <PORT>
*/
void syntax() {
  printf("\n%s ([-i <hostname>] -p <port>)... [-c <cmd|all>]... [-f <cmd file>]... [-period <seconds>] [-t <seconds>]\n\n",prgName);
  printf("Several debug targets can be specified ([-i <hostname>] -p <port>)...\n");
  printf("  -i <hostname>  IP address or hostname of the debug target.\n");
  printf("                 When omitted previous -i value in the command line is taken as default\n");
  printf("                 or 127.0.0.1 when no previous -i option is set.\n");
  printf("  -p <port>      Port number of the debug target.\n");
  printf("                 At least one port value must be given.\n");
  printf("\nOptionnaly a list of command to run can be specified:\n");
  printf("  [-c <cmd|all>]...\n"); 
  printf("         Every word after -c is interpreted as a word of a command until end of line or new option.\n");
  printf("         Several -c options can be set.\n");                 
  printf("         \"all\" is used to run all the commands the target knows.\n");    
  printf("  [-f <cmd file>]...\n");  
  printf("         The list of commands can be specified through some files.\n");
  printf("  [-period <seconds>]\n");       
  printf("         Periodicity when running commands using -c or/and -f options.\n");  
  printf("\nMiscellaneous options:\n");
  printf("  -t <seconds>   Timeout value to wait for a response (default %d seconds).\n",DEFAULT_TIMEOUT);      
  printf("\ne.g\n%s -i 192.168.1.1 -p 50003 -p 50004 -p 50005 -c profiler reset\n",prgName) ;          
  printf("%s -i 192.168.1.1 -p 50003 -i 192.168.1.2 -p 50003 -c profiler -period 10\n",prgName) ;          
  exit(0);
}
int debug_receive(int socketId, int silent) {
  int             ret;
  unsigned int    recvLen;
 
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
      printf("%s", msg.buffer);
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
    printf("File %s not found\n",fileName);
    syntax();
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

  sent = send(socketId, &msg, len, 0);
  if (sent != len) {
    printf("send %s",strerror(errno));
    printf("%d sent upon %d\n", sent,len);
    return -1;
  }
    
  if (!debug_receive(socketId,silent)) {
    printf("Debug session abort\n");
    return -1;
  }  
  return 0;
  
}
#define SYSTEM_HEADER "system : "
void uma_dbg_read_prompt(int socketId, char * pr) {
  int i=strlen(SYSTEM_HEADER);
  char *c = pr;
    
  // Read the prompt
  if (debug_run_this_cmd(socketId, "who", SILENT) < 0)  return;
  
  if (strncmp(msg.buffer,SYSTEM_HEADER, strlen(SYSTEM_HEADER)) == 0) {

    while(msg.buffer[i] != '\n') {
      *c = msg.buffer[i];
      c++;
      i++;
    }
    *c++ = '>';
    *c++ = ' ';     
    *c = 0;
  }
  else {
    strcpy(pr,"rzdbg> ");
  }
}
#define LIST_COMMAND_HEADER "List of available topics :"
void uma_dbg_read_all_cmd_list(int socketId) {
  char * p, * begin;
  int len;
    
  // Read the command list
  if (debug_run_this_cmd(socketId, "", SILENT) < 0)  return;

  nbCmd = 0;
  p = msg.buffer;
    
  if (strncmp(p,LIST_COMMAND_HEADER, strlen(LIST_COMMAND_HEADER)) != 0) return;  
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
}
void debug_interactive_loop(int socketId) {
//  char mycmd[1024]; 
  char *mycmd = NULL; 
//  int len;
//  int fd;
  
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
      printf("Debug session end\n");
      break;
    }
    if (strcasecmp(mycmd,"q") == 0) {
      printf("Debug session end\n");
      break;
    }
    if (strcasecmp(mycmd,"quit") == 0) {
      printf("Debug session end\n");
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
//    printf("> %s", cmd[idx]);  
    if (debug_run_this_cmd(socketId, cmd[idx], NOT_SILENT) < 0)  break;
  }
} 


int expgw_host2ip(char *host,uint32_t *ipaddr_p)
{
    struct hostent *hp;    
    /*
    ** get the IP address of the storage node
    */
    if ((hp = gethostbyname(host)) == 0) {
        printf("gethostbyname failed for host : %s, %s", host,
                strerror(errno));
        return -1;
    }
    bcopy((char *) hp->h_addr, (char *) ipaddr_p, hp->h_length);
//    *ipaddr_p = ntohl(*ipaddr_p);
    return 0;
    
}

void read_parameters(argc, argv)
int argc;
char *argv[];
{
  uint32_t            ret;
  uint32_t            idx;
  uint32_t            port32;
  int                 status;

  idx = 1;
  /* Scan parameters */
  while (idx < argc) {
    
    /* -i <ipAddr> */
    if (strcmp(argv[idx],"-i")==0) {
      idx++;
      if (idx == argc) {
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
//      ipAddr = inet_addr(argv[idx]);
      status = expgw_host2ip(argv[idx],&ipAddr[nbTarget]);
      if (status < 0) 
      {
        syntax();      
      }
      idx++;
      continue;
    }

    /* -p <portNumber> */
    if (strcmp(argv[idx],"-p")==0) {
      idx++;
      if (idx == argc) {
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
      ret = sscanf(argv[idx],"%u",&port32);
      if (ret != 1) {
	printf ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
        syntax();
      }	
      if ((port32<0) || (port32>0xFFFF)) {
	printf ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);
	syntax();
      }
      
      serverPort[nbTarget] = (uint16_t) port32;
      nbTarget++;
      /* Pre-initialize next IP address */ 
      ipAddr[nbTarget] = ipAddr[nbTarget-1];             
      idx++;
      continue;
    }
    
    /* -period <period> */
    if (strcmp(argv[idx],"-period")==0) {
      idx++;
      if (idx == argc) {
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
      ret = sscanf(argv[idx],"%u",&period);
      if (ret != 1) {
	printf ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);   
        syntax();
      }
      idx++;
      continue;
    }
    
    /* -t <seconds> */
    if (strcmp(argv[idx],"-t")==0) {
      idx++;
      if (idx == argc) {
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
      ret = sscanf(argv[idx],"%u",&timeout);
      if (ret != 1) {
	printf ("%s option with unexpected value \"%s\" !!!\n",argv[idx-1],argv[idx]);   
        syntax();
      }
      idx++;
      continue;
    }  
      
    /* -c <command> */
    if (strcmp(argv[idx],"-c")==0) {
      idx++;
      if (idx == argc) {
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
      if (strcmp(argv[idx],"all") == 0) {
        allCmd = 1;
	idx++;
      }
      else {
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
	printf ("%s option but missing value !!!\n",argv[idx-1]);
	syntax();
      }
      read_file(argv[idx]);
      idx++;
      continue;
    }

    printf("Unexpected option \"%s\" !!!\n", argv[idx]);
    syntax();
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
    exit(0);
  }  
  
  /* 
  ** change sizeof the buffer of socket for sending
  */
  if (setsockopt (socketId,SOL_SOCKET,
		  SO_SNDBUF,(char*)&sockSndSize,sizeof(int)) == -1)  {
    printf("Error on setsockopt SO_SNDBUF %d\n",sockSndSize);
    close(socketId);
    exit(0);
  }
  /* 
  ** change sizeof the buffer of socket for receiving
  */  
  if (setsockopt (socketId,SOL_SOCKET,
                  SO_RCVBUF,(char*)&sockRcvdSize,sizeof(int)) == -1)  {
    printf("Error on setsockopt SO_RCVBUF %d !!!\n",sockRcvdSize);
    close(socketId);
    exit(0);
  }
  

  /* Connect to the GG */
  vSckAddr.sin_family = AF_INET;
  vSckAddr.sin_port   = htons(serverPort);
  memcpy(&vSckAddr.sin_addr.s_addr, &ipAddr, 4); 
  if (connect(socketId,(struct sockaddr *)&vSckAddr,sizeof(struct sockaddr_in)) == -1) {
    printf("error on connect %s!!!", strerror(errno));
    exit(0);
  }
  return socketId;
}
int main(int argc, const char **argv) {
  int                 socketId; 
  int                 idx; 
  char              * p;
  uint32_t            ip;
   
  prgName = argv[0];

  /* Read parametres */
  memset(serverPort,0,sizeof(serverPort)); 
  memset(ipAddr,0,sizeof(ipAddr));
  nbTarget = 0;
  /* Pre-initialize 1rst IP address */ 
  ipAddr[nbTarget] = inet_addr("127.0.0.1");  
  period        = 0;
  nbCmd         = 0;
  allCmd        = 0;
  read_parameters(argc, argv);
  if (nbTarget == 0) syntax();


reloop:

  for (idx = 0; idx < nbTarget; idx++) {
   
    socketId = connect_to_server(ipAddr[idx],serverPort[idx]);

    
    p = prompt;
    ip = ntohl(ipAddr[idx]);
    p += sprintf(prompt,"[%u.%u.%u.%u:%d] ",(ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF,ip&0xFF, serverPort[idx]);
    uma_dbg_read_prompt(socketId,p);
      
    if (nbTarget > 1) {
      printf("%s\n",prompt);
    } 
     
    if (allCmd) uma_dbg_read_all_cmd_list(socketId);

    if (nbCmd == 0) {
      debug_interactive_loop(socketId);
    }  
    else {
      debug_run_command_list(socketId);
    }
    shutdown(socketId,SHUT_RDWR);   
    close(socketId);
  }  
  
  if (period != 0) {
    sleep(period);
    goto reloop;
  }

  exit(1);
}
