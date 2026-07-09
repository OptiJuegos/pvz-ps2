/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2005, ps2dev - http://www.ps2dev.org
# Licenced under GNU Library General Public License version 2
# Review ps2sdk README & LICENSE files for further details.
#
# PS2_EEIP_DRIVER
*/

#ifndef PS2_EEIP_DRIVER
#define PS2_EEIP_DRIVER

#include <stdbool.h>
#include <ps2ip.h>

#ifdef __cplusplus
extern "C" {
#endif

enum EEIP_INIT_STATUS {
    EEIP_INIT_STATUS_DEPENDENCY_IRX_ERROR = -2,
    EEIP_INIT_STATUS_UNKNOWN = -1,
    EEIP_INIT_STATUS_OK = 0,
};

enum EEIP_INIT_STATUS init_eeip_driver(bool init_dependencies);
void deinit_eeip_driver(bool deinit_dependencies);

/* ------------------------------------------------------------------------- */
/* Network configuration helpers                                             */
/* ------------------------------------------------------------------------- */

enum EEIP_PROGRESS_EVENT {
    EEIP_PROGRESS_SETTING_LINK_MODE,
    EEIP_PROGRESS_TCPIP_INIT,
    EEIP_PROGRESS_APPLYING_IP_CONFIG,
    EEIP_PROGRESS_WAITING_LINK_UP,
    EEIP_PROGRESS_LINK_UP,
    EEIP_PROGRESS_WAITING_DHCP,
    EEIP_PROGRESS_DHCP_BOUND,
    EEIP_PROGRESS_READY,
};

enum EEIP_NET_STATUS {
    EEIP_NET_STATUS_OK              =  0,
    EEIP_NET_STATUS_LINK_MODE_ERROR = -1,
    EEIP_NET_STATUS_LINK_TIMEOUT    = -2,
    EEIP_NET_STATUS_DHCP_TIMEOUT    = -3,
    EEIP_NET_STATUS_CONFIG_ERROR    = -4,
};

typedef void (*eeip_progress_cb)(enum EEIP_PROGRESS_EVENT ev, void *user);

typedef struct {
    bool             use_dhcp;
    struct ip4_addr  ip;        /* ignored when use_dhcp == true */
    struct ip4_addr  netmask;   /* ignored when use_dhcp == true */
    struct ip4_addr  gateway;   /* ignored when use_dhcp == true */
    int              link_mode;        /* NETMAN_NETIF_ETH_LINK_MODE_* */
    int              timeout_seconds;  /* per phase; 0 -> default 10 */
    eeip_progress_cb on_progress;      /* may be NULL */
    void            *user;             /* opaque, passed to callback */
} eeip_network_config_t;

/* Build a struct ip4_addr from octets. Equivalent to lwIP's IP4_ADDR but
   returns the value, so it can be used in struct initializers / assignments. */
static inline struct ip4_addr eeip_make_addr(unsigned char a,
                                             unsigned char b,
                                             unsigned char c,
                                             unsigned char d) {
    struct ip4_addr ip;
    IP4_ADDR(&ip, a, b, c, d);
    return ip;
}
#define EEIP_ADDR(a, b, c, d) eeip_make_addr((a), (b), (c), (d))

void eeip_network_config_default_dhcp(eeip_network_config_t *cfg);
enum EEIP_NET_STATUS configure_eeip_network(const eeip_network_config_t *cfg);
int  eeip_get_current_config(struct ip4_addr *ip,
                             struct ip4_addr *nm,
                             struct ip4_addr *gw);

#ifdef __cplusplus
}
#endif

#endif /* PS2_EEIP_DRIVER */
