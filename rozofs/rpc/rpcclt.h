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

#ifndef _RPCCLT_H
#define _RPCCLT_H

#include <rpc/rpc.h>

typedef struct rpcclt {
    int sock;
    CLIENT *client;
    int lbg_id; /**< reference of the load balancing group */
} rpcclt_t;

static inline void init_rpcctl_ctx(rpcclt_t * clt) {
  clt->sock   = -1;
  clt->lbg_id = -1;
  clt->client = NULL;
}

int rpcclt_initialize(rpcclt_t * client, const char *host, unsigned long prog,
        unsigned long vers, unsigned int sendsz, unsigned int recvsz,
        uint32_t port_num, struct timeval timeout);

void rpcclt_release(rpcclt_t * client);

#endif
