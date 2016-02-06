/*
 *	debugnet library for PSP2 
 *	Copyright (C) 2010,2015 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 *  Repository https://github.com/psxdev/debugnet
 */
#ifndef _DEBUGNET_H_
#define _DEBUGNET_H_

#include "types.h"
#include "utils.h"

#define PSP2_NET_AF_INET        2
#define PSP2_NET_SOCK_DGRAM     2
#define PSP2_NET_IPPROTO_UDP    17
#define PSP2_NET_SOL_SOCKET     0xffff
#define PSP2_NET_SO_BROADCAST   0x00000020
#define PSP2_NET_ERROR_ENOTINIT 0x804101C8
#define PSP2_NETCTL_INFO_GET_IP_ADDRESS 15
#define PSP2_NETCTL_INFO_CONFIG_NAME_LEN_MAX 64
#define PSP2_NETCTL_INFO_SSID_LEN_MAX        32

typedef struct SceNetInitParam {
    void *memory;
    s32_t size;
    s32_t flags;
} SceNetInitParam;

typedef struct SceNetInAddr {
    u32_t s_addr;
} SceNetInAddr;

typedef struct SceNetSockaddrIn {
    u8_t sin_len;
    u8_t sin_family;
    u16_t sin_port;
    SceNetInAddr sin_addr;
    u16_t sin_vport;
    s8_t sin_zero[6];
} SceNetSockaddrIn;

typedef struct SceNetEtherAddr {
    u8_t data[6];
} SceNetEtherAddr;

typedef union SceNetCtlInfo {
    s8_t cnf_name[PSP2_NETCTL_INFO_CONFIG_NAME_LEN_MAX + 1];
    u32_t device;
    SceNetEtherAddr ether_addr;
    u32_t mtu;
    u32_t link;
    SceNetEtherAddr bssid;
    s8_t ssid[PSP2_NETCTL_INFO_SSID_LEN_MAX + 1];
    u32_t wifi_security;
    u32_t rssi_dbm;
    u32_t rssi_percentage;
    u32_t channel;
    u32_t ip_config;
    s8_t dhcp_hostname[256];
    s8_t pppoe_auth_name[128];
    s8_t ip_address[16];
    s8_t netmask[16];
    s8_t default_route[16];
    s8_t primary_dns[16];
    s8_t secondary_dns[16];
    u32_t http_proxy_config;
    s8_t http_proxy_server[256];
    u32_t http_proxy_port;
} SceNetCtlInfo;


#define NET_INIT_SIZE 1*1024*1024

int debugNetSetup();
int debugNetInit(const char *serverIp, u16_t port);
void debugNetFinish();
int debugNetSend(const char* line, u32_t size);
void debugNetUDPPrintf(const char* fmt, ...);
int uvl_debugnet_log(const char *line);

#endif
