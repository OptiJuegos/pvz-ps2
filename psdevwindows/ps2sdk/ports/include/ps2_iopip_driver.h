/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2005, ps2dev - http://www.ps2dev.org
# Licenced under GNU Library General Public License version 2
# Review ps2sdk README & LICENSE files for further details.
#
# PS2_IOPIP_DRIVER
*/

#ifndef PS2_IOPIP_DRIVER
#define PS2_IOPIP_DRIVER

#include <stdbool.h>
#include <ps2ips.h>     /* IOP-side socket RPC + transitively struct ip4_addr */

#ifdef __cplusplus
extern "C" {
#endif

enum IOPIP_INIT_STATUS {
    IOPIP_INIT_STATUS_DEPENDENCY_IRX_ERROR = -5,
    IOPIP_INIT_STATUS_PS2IP_NM_IRX_ERROR = -4,
    IOPIP_INIT_STATUS_PS2IPS_IRX_ERROR = -3,
    IOPIP_INIT_STATUS_PS2IPS_ERROR = -2,
    IOPIP_INIT_STATUS_UNKNOWN = -1,
    IOPIP_INIT_STATUS_OK = 0,
};

enum IOPIP_INIT_STATUS init_iopip_driver(bool init_dependencies);
void deinit_iopip_driver(bool deinit_dependencies);

/* ------------------------------------------------------------------------- */
/* Network configuration helpers (static IP only).                           */
/*                                                                           */
/* The IOP-side path runs lwIP inside ps2ip-nm.irx; the EE talks to it via   */
/* SIF RPC. DHCP is intentionally not supported here — use the EE-side       */
/* eeip driver if you need DHCP.                                             */
/* ------------------------------------------------------------------------- */

enum IOPIP_PROGRESS_EVENT {
    IOPIP_PROGRESS_SETTING_LINK_MODE,
    IOPIP_PROGRESS_APPLYING_IP_CONFIG,
    IOPIP_PROGRESS_WAITING_LINK_UP,
    IOPIP_PROGRESS_LINK_UP,
    IOPIP_PROGRESS_READY,
};

enum IOPIP_NET_STATUS {
    IOPIP_NET_STATUS_OK              =  0,
    IOPIP_NET_STATUS_LINK_MODE_ERROR = -1,
    IOPIP_NET_STATUS_LINK_TIMEOUT    = -2,
    IOPIP_NET_STATUS_CONFIG_ERROR    = -3,
};

typedef void (*iopip_progress_cb)(enum IOPIP_PROGRESS_EVENT ev, void *user);

typedef struct {
    struct ip4_addr  ip;
    struct ip4_addr  netmask;
    struct ip4_addr  gateway;
    int              link_mode;        /* NETMAN_NETIF_ETH_LINK_MODE_* */
    int              timeout_seconds;  /* per phase; 0 -> default 10 */
    iopip_progress_cb on_progress;     /* may be NULL */
    void            *user;             /* opaque, passed to callback */
} iopip_network_config_t;

/* IOPIP_ADDR(a,b,c,d) builds a struct ip4_addr from octets — same helper as
   the EE side, declared here for parity so callers using only the iopip
   driver header still get it. */
#ifndef EEIP_ADDR
static inline struct ip4_addr iopip_make_addr(unsigned char a,
                                              unsigned char b,
                                              unsigned char c,
                                              unsigned char d) {
    struct ip4_addr ip;
    IP4_ADDR(&ip, a, b, c, d);
    return ip;
}
#define IOPIP_ADDR(a, b, c, d) iopip_make_addr((a), (b), (c), (d))
#else
#define IOPIP_ADDR(a, b, c, d) EEIP_ADDR((a), (b), (c), (d))
#endif

/* Fill cfg with sane defaults: zeroed addrs (caller MUST set ip/netmask/
   gateway before configure_iopip_network), AUTO link mode, 10s timeout,
   no callback. */
void iopip_network_config_default(iopip_network_config_t *cfg);

/* Apply the static IP config: optional NetManSetLinkMode, ps2ip_setconfig,
   then poll the link until it's up (or timeout). Reports progress through
   cfg->on_progress if non-NULL. Requires init_iopip_driver(true) first. */
enum IOPIP_NET_STATUS configure_iopip_network(const iopip_network_config_t *cfg);

/* Read the currently bound IP/netmask/gateway from sm0. Returns 0 on success. */
int iopip_get_current_config(struct ip4_addr *ip,
                             struct ip4_addr *nm,
                             struct ip4_addr *gw);

#ifdef __cplusplus
}
#endif

#endif /* PS2_IOPIP_DRIVER */
