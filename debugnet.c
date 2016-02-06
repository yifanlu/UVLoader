/*
 *	debugnet library for PSP2 
 *	Copyright (C) 2010,2015 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 *  Repository https://github.com/psxdev/debugnet
 */

#include "uvloader.h"
#include "scefuncs.h"
#include "debugnet.h"

static int debugnet_initialized = 0;
static int SocketFD = -1;
static u8_t net_memory[NET_INIT_SIZE];
static SceNetInAddr vita_addr;
static struct SceNetSockaddrIn stSockAddr;

int debugNetSetup()
{
    uvl_scefuncs_resolve_debugnet();

    uvl_set_debug_log_func(&uvl_debugnet_log);

    return debugNetInit("255.255.255.255", 18194);
}

int uvl_debugnet_log(const char *line)
{
    debugNetUDPPrintf("%s\n", line);
	return 0;
}

int debugNetSend(const char* line, u32_t size)
{
    int res = sceNetSendto(SocketFD, line, size, 0, (struct SceNetSockaddr *)&stSockAddr, sizeof stSockAddr);
    if (res < 0)
        return res;

    return 0;
}

/**
 * UDP printf for debugnet library 
 *
 * @par Example:
 * @code
 * debugNetUDPPrintf("This is a test\n");
 * @endcode
 */
void debugNetUDPPrintf(const char* fmt, ...)
{
  char buffer[0x800];
  va_list arg;
  va_start(arg, fmt);
  sceClibVsnprintf(buffer, sizeof(buffer), fmt, arg);
  va_end(arg);
  
  debugNetSend(buffer, strlen(buffer));
}

/**
 * Init debugnet library 
 *
 * @par Example:
 * @code
 * int ret;
 * ret = debugNetInit("172.26.0.2", 18194);
 * @endcode
 *
 * @param serverIP - your pc/mac server ip
 * @param port - udp port server
 */
int debugNetInit(const char *serverIp, u16_t port)
{
    int ret;
    SceNetInitParam initparam;
    SceNetCtlInfo info;
    if (debugnet_initialized) {
        return debugnet_initialized;
    }

    /*net initialazation code from xerpi at https://github.com/xerpi/FTPVita/blob/master/ftp.c*/
    /* Init Net */
    if (sceNetShowNetstat() == PSP2_NET_ERROR_ENOTINIT) {
        initparam.memory = net_memory;
        initparam.size = NET_INIT_SIZE;
        initparam.flags = 0;

        ret = sceNetInit(&initparam);

        //printf("sceNetInit(): 0x%08X\n", ret);
    } else {
        //printf("Net is already initialized.\n");
    }

    /* Init NetCtl */
    ret = sceNetCtlInit();
    //printf("sceNetCtlInit(): 0x%08X\n", ret);

    /* Get IP address */
    ret = sceNetCtlInetGetInfo(PSP2_NETCTL_INFO_GET_IP_ADDRESS, &info);
    //printf("sceNetCtlInetGetInfo(): 0x%08X\n", ret);

    /* Save the IP of PSVita to a global variable */
    sceNetInetPton(PSP2_NET_AF_INET, info.ip_address, &vita_addr);

    uvl_unlock_mem();

	/* Create datagram udp socket*/
    SocketFD = sceNetSocket("debugnet_socket",
        PSP2_NET_AF_INET, PSP2_NET_SOCK_DGRAM, PSP2_NET_IPPROTO_UDP);

    memset(&stSockAddr, 0, sizeof stSockAddr);

	/*Populate SceNetSockaddrIn structure values*/
    stSockAddr.sin_family = PSP2_NET_AF_INET;
    stSockAddr.sin_port = sceNetHtons(port);

    int broadcast = 1;
    sceNetSetsockopt(SocketFD, PSP2_NET_SOL_SOCKET, PSP2_NET_SO_BROADCAST, &broadcast, sizeof(broadcast));

    sceNetInetPton(PSP2_NET_AF_INET, serverIp, &stSockAddr.sin_addr);

	/*Show log on pc/mac side*/
	/*debugNetUDPPrintf("debugnet initialized\n");
	debugNetUDPPrintf("Copyright (C) 2010,2015 Antonio Jose Ramos Marquez aka bigboss @psxdev\n");
	debugNetUDPPrintf("This Program is subject to the terms of the Mozilla Public\n"
		"License, v. 2.0. If a copy of the MPL was not distributed with this\n"
		"file, You can obtain one at http://mozilla.org/MPL/2.0/.\n");
    debugNetUDPPrintf("ready to have a lot of fun...\n");*/

    /*library debugnet initialized*/
    debugnet_initialized = 1;
    uvl_lock_mem();

    return debugnet_initialized;
}

/**
 * Finish debugnet library 
 *
 * @par Example:
 * @code
 * debugNetFinish();
 * @endcode
 */
void debugNetFinish()
{
    if (debugnet_initialized) {
       
        sceNetCtlTerm();
        sceNetTerm();

        uvl_unlock_mem();
        debugnet_initialized = 0;
        uvl_lock_mem();
    }
}
