//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2001 J�nis Legzdi��
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#include "gamedefs.h"
#include "net_loc.h"

#include "net_loop.h"
#include "net_null.h"
#include "net_dgrm.h"
#include "net_ser.h"

net_driver_t net_drivers[MAX_NET_DRIVERS] =
{
	{
	"Loopback",
	false,
	Loop_Init,
	Loop_Listen,
	Loop_SearchForHosts,
	Loop_Connect,
	Loop_CheckNewConnections,
	Loop_GetMessage,
	Loop_SendMessage,
	Loop_SendUnreliableMessage,
	Loop_CanSendMessage,
	Loop_CanSendUnreliableMessage,
	Loop_Close,
	Loop_Shutdown
	}
	,
	{
	"Null",
	false,
	NetNull_Init,
	NetNull_Listen,
	NetNull_SearchForHosts,
	NetNull_Connect,
	NetNull_CheckNewConnections,
	NetNull_GetMessage,
	NetNull_SendMessage,
	NetNull_SendUnreliableMessage,
	NetNull_CanSendMessage,
	NetNull_CanSendUnreliableMessage,
	NetNull_Close,
	NetNull_Shutdown
	}
	,
	{
	"Datagram",
	false,
	Datagram_Init,
	Datagram_Listen,
	Datagram_SearchForHosts,
	Datagram_Connect,
	Datagram_CheckNewConnections,
	Datagram_GetMessage,
	Datagram_SendMessage,
	Datagram_SendUnreliableMessage,
	Datagram_CanSendMessage,
	Datagram_CanSendUnreliableMessage,
	Datagram_Close,
	Datagram_Shutdown
	}
	,
	{
	"Serial",
	false,
	NULL,//Serial_Init,
	NULL,//Serial_Listen,
	NULL,//Serial_SearchForHosts,
	NULL,//Serial_Connect,
	NULL,//Serial_CheckNewConnections,
	NULL,//Serial_GetMessage,
	NULL,//Serial_SendMessage,
	NULL,//Serial_SendUnreliableMessage,
	NULL,//Serial_CanSendMessage,
	NULL,//Serial_CanSendUnreliableMessage,
	NULL,//Serial_Close,
	NULL,//Serial_Shutdown
	}
};

int net_numdrivers = 3;//4;

#include "net_bw.h"
#include "net_ipx.h"
#include "net_mp.h"

net_landriver_t	net_landrivers[MAX_NET_DRIVERS] =
{
	{
	"Beame & Whiteside TCP/IP",
	false,
	0,
	BW_Init,
	BW_Shutdown,
	BW_Listen,
	BW_OpenSocket,
	BW_CloseSocket,
	BW_Connect,
	BW_CheckNewConnections,
	BW_Read,
	BW_Write,
	BW_Broadcast,
	BW_AddrToString,
	BW_StringToAddr,
	BW_GetSocketAddr,
	BW_GetNameFromAddr,
	BW_GetAddrFromName,
	BW_AddrCompare,
	BW_GetSocketPort,
	BW_SetSocketPort
	}
    ,
	{
	"IPX",
	false,
	0,
	IPX_Init,
	IPX_Shutdown,
	IPX_Listen,
	IPX_OpenSocket,
	IPX_CloseSocket,
	IPX_Connect,
	IPX_CheckNewConnections,
	IPX_Read,
	IPX_Write,
	IPX_Broadcast,
	IPX_AddrToString,
	IPX_StringToAddr,
	IPX_GetSocketAddr,
	IPX_GetNameFromAddr,
	IPX_GetAddrFromName,
	IPX_AddrCompare,
	IPX_GetSocketPort,
	IPX_SetSocketPort
	}
	,
	{
	"Win95 TCP/IP",
	false,
	0,
	MPATH_Init,
	MPATH_Shutdown,
	MPATH_Listen,
	MPATH_OpenSocket,
	MPATH_CloseSocket,
	MPATH_Connect,
	MPATH_CheckNewConnections,
	MPATH_Read,
	MPATH_Write,
	MPATH_Broadcast,
	MPATH_AddrToString,
	MPATH_StringToAddr,
	MPATH_GetSocketAddr,
	MPATH_GetNameFromAddr,
	MPATH_GetAddrFromName,
	MPATH_AddrCompare,
	MPATH_GetSocketPort,
	MPATH_SetSocketPort
	}
};

int net_numlandrivers = 3;

//**************************************************************************
//
//	$Log$
//	Revision 1.4  2001/12/01 17:40:41  dj_jl
//	Added support for bots
//
//	Revision 1.3  2001/07/31 17:16:31  dj_jl
//	Just moved Log to the end of file
//	
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
