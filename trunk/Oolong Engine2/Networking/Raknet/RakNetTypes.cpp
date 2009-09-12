/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "RakNetTypes.h"
#include "RakAssert.h"
#include <string.h>
#include <stdio.h>

#if defined(_XBOX) || defined(X360)
#elif defined(_WIN32)
// IP_DONTFRAGMENT is different between winsock 1 and winsock 2.  Therefore, Winsock2.h must be linked againt Ws2_32.lib
// winsock.h must be linked against WSock32.lib.  If these two are mixed up the flag won't work correctly
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h> // strncasecmp
#include "Itoa.h"
#include "SocketLayer.h"
#include <stdlib.h>

bool NonNumericHostString( const char *host )
{
	if ( host[ 0 ] >= '0' && host[ 0 ] <= '9' )
		return false;

	if ( (host[ 0 ] == '-') && ( host[ 1 ] >= '0' && host[ 1 ] <= '9' ) )
		return false;

	return true;
}

SocketDescriptor::SocketDescriptor() {port=0; hostAddress[0]=0; remotePortRakNetWasStartedOn_PS3=0;}
SocketDescriptor::SocketDescriptor(unsigned short _port, const char *_hostAddress)
{
	remotePortRakNetWasStartedOn_PS3=0;
	port=_port;
	if (_hostAddress)
		strcpy(hostAddress, _hostAddress);
	else
		hostAddress[0]=0;
}

// Defaults to not in peer to peer mode for NetworkIDs.  This only sends the localSystemAddress portion in the BitStream class
// This is what you want for client/server, where the server assigns all NetworkIDs and it is unnecessary to transmit the full structure.
// For peer to peer, this will transmit the systemAddress of the system that created the object in addition to localSystemAddress.  This allows
// Any system to create unique ids locally.
// All systems must use the same value for this variable.
//bool RAK_DLL_EXPORT NetworkID::peerToPeerMode=false;

bool SystemAddress::operator==( const SystemAddress& right ) const
{
	return binaryAddress == right.binaryAddress && port == right.port;
}

bool SystemAddress::operator!=( const SystemAddress& right ) const
{
	return binaryAddress != right.binaryAddress || port != right.port;
}

bool SystemAddress::operator>( const SystemAddress& right ) const
{
	return ( ( binaryAddress > right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port > right.port ) ) );
}

bool SystemAddress::operator<( const SystemAddress& right ) const
{
	return ( ( binaryAddress < right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port < right.port ) ) );
}
const char *SystemAddress::ToString(bool writePort) const
{
	static unsigned char strIndex=0;
	static char str[8][22];

	unsigned char lastStrIndex=strIndex;
	strIndex++;
	ToString(writePort, str[lastStrIndex&7]);
	return (char*) str[lastStrIndex&7];
}
void SystemAddress::ToString(bool writePort, char *dest) const
{
	if (*this==UNASSIGNED_SYSTEM_ADDRESS)
	{
		strcpy(dest, "UNASSIGNED_SYSTEM_ADDRESS");
		return;
	}

#if defined(_XBOX) || defined(X360)
	// Don't do this, can't use ToString() to call Connect()
//	if (writePort)
//		sprintf( dest, "%016I64X:%d", binaryAddress, port );
//	else
//		sprintf( dest, "%016I64X", binaryAddress );

	Itoa(binaryAddress, dest, 10);
	if (writePort)
	{
		strcat(dest, ":");
		Itoa(port, dest+strlen(dest), 10);
	}

#else

	
	in_addr in;
	in.s_addr = binaryAddress;
//	cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
//	sys_net_initialize_network();
	const char *ntoaStr = inet_ntoa( in );
	strcpy(dest, ntoaStr);
	if (writePort)
	{
		strcat(dest, ":");
		Itoa(port, dest+strlen(dest), 10);
	}
#endif
}
SystemAddress::SystemAddress() {*this=UNASSIGNED_SYSTEM_ADDRESS;}
SystemAddress::SystemAddress(const char *a, unsigned short b) {SetBinaryAddress(a); port=b;};
SystemAddress::SystemAddress(unsigned int a, unsigned short b) {binaryAddress=a; port=b;};
#ifdef _MSC_VER
#pragma warning( disable : 4996 )  // The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strnicmp. See online help for details.
#endif
void SystemAddress::SetBinaryAddress(const char *str)
{
	if ( NonNumericHostString( str ) )
	{

#if defined(_XBOX) || defined(X360)
		return;
#else
	#if defined(_WIN32)
		if (_strnicmp(str,"localhost", 9)==0)
	#else
		if (strncasecmp(str,"localhost", 9)==0)
	#endif
		{
			binaryAddress=inet_addr("127.0.0.1");
			if (str[9])
				port=(unsigned short) atoi(str+9);
			return;
		}

		const char *ip = ( char* ) SocketLayer::Instance()->DomainNameToIP( str );
		if (ip)
		{
			binaryAddress=inet_addr(ip);
		}
#endif
	}
	else
	{
		//#ifdef _XBOX
		//	binaryAddress=UNASSIGNED_SYSTEM_ADDRESS.binaryAddress;
		//#else
		// Split the string into the first part, and the : part
		int index, portIndex;
		char IPPart[22];
		char portPart[10];
		// Only write the valid parts, don't change existing if invalid
		//	binaryAddress=UNASSIGNED_SYSTEM_ADDRESS.binaryAddress;
		//	port=UNASSIGNED_SYSTEM_ADDRESS.port;
		for (index=0; str[index] && str[index]!=':' && index<22; index++)
		{
			IPPart[index]=str[index];
		}
		IPPart[index]=0;
		portPart[0]=0;
		if (str[index] && str[index+1])
		{
			index++;
			for (portIndex=0; portIndex<10 && str[index] && index < 22+10; index++, portIndex++)
				portPart[portIndex]=str[index];
			portPart[portIndex]=0;
		}

#if defined(_XBOX) || defined(X360)
		int dotCount=0;
		for (index=0; str[index] && str[index]!=':' && index<22; index++)
		{
			if (str[index]=='.')
				dotCount++;
		}
		if (IPPart[0] && dotCount==3)
			binaryAddress=inet_addr(IPPart);
		else
			binaryAddress=atoi(IPPart);
#else
		if (IPPart[0])
			binaryAddress=inet_addr(IPPart);

#endif

		if (portPart[0])
			port=(unsigned short) atoi(portPart);
		//#endif
	}

}

NetworkID& NetworkID::operator = ( const NetworkID& input )
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
	systemAddress = input.systemAddress;
	guid = input.guid;
#endif
	localSystemAddress = input.localSystemAddress;
	return *this;
}

bool NetworkID::operator==( const NetworkID& right ) const
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return guid == right.guid && localSystemAddress == right.localSystemAddress;
		else
			return systemAddress == right.systemAddress && localSystemAddress == right.localSystemAddress;
	}
#else
//	else
		return localSystemAddress==right.localSystemAddress;
#endif
}

bool NetworkID::operator!=( const NetworkID& right ) const
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return guid != right.guid || localSystemAddress != right.localSystemAddress;
		else
			return systemAddress != right.systemAddress || localSystemAddress != right.localSystemAddress;
	}
#else
//	else
		return localSystemAddress!=right.localSystemAddress;
#endif
}

bool NetworkID::operator>( const NetworkID& right ) const
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return ( ( guid > right.guid ) || ( ( guid == right.guid ) && ( localSystemAddress > right.localSystemAddress ) ) );
		else
			return ( ( systemAddress > right.systemAddress ) || ( ( systemAddress == right.systemAddress ) && ( localSystemAddress > right.localSystemAddress ) ) );
	}
#else
//	else
		return localSystemAddress>right.localSystemAddress;
#endif
}

bool NetworkID::operator<( const NetworkID& right ) const
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return ( ( guid < right.guid ) || ( ( guid == right.guid ) && ( localSystemAddress < right.localSystemAddress ) ) );
		else
			return ( ( systemAddress < right.systemAddress ) || ( ( systemAddress == right.systemAddress ) && ( localSystemAddress < right.localSystemAddress ) ) );
	}
#else
	//else
		return localSystemAddress<right.localSystemAddress;
#endif
}
bool NetworkID::IsPeerToPeerMode(void)
{
#if defined NETWORK_ID_SUPPORTS_PEER_TO_PEER
	return true;
#else
	return false;
#endif
//	return peerToPeerMode;
}
void NetworkID::SetPeerToPeerMode(bool isPeerToPeer)
{
	(void) isPeerToPeer;

	// deprecated, define NETWORK_ID_SUPPORTS_PEER_TO_PEER instead for true, comment out for false
	// This is in RakNetDefines.h
	RakAssert(0);

//	peerToPeerMode=isPeerToPeer;
}
bool RakNetGUID::operator==( const RakNetGUID& right ) const
{
	//return memcmp(g, right.g, sizeof(g))==0;
	return g==right.g;
//	for (unsigned int i=0; i < sizeof(RakNetGUID) / sizeof(RakNetGUID::g[0]); i++)
//		if (g[i]!=right.g[i])
//			return false;
//	return true;
}
bool RakNetGUID::operator!=( const RakNetGUID& right ) const
{
	return g!=right.g;
	// return memcmp(g, right.g, sizeof(g))!=0;
//	for (unsigned int i=0; i < sizeof(RakNetGUID) / sizeof(RakNetGUID::g[0]); i++)
//		if (g[i]!=right.g[i])
//			return true;
//	return false;
}
bool RakNetGUID::operator > ( const RakNetGUID& right ) const
{
	// memcmp returns the wrong result!
//	return memcmp(g, right.g, sizeof(g))>0;
	return g > right.g;
	/*
	for (unsigned int i=0; i < sizeof(RakNetGUID) / sizeof(RakNetGUID::g[0]); i++)
	{
		if (g[i]<right.g[i])
			return false;
		if (g[i]>right.g[i])
			return true;
	}
	return false;
	*/
}
bool RakNetGUID::operator < ( const RakNetGUID& right ) const
{
	return g < right.g;
	// memcmp returns the wrong result!
//	return memcmp(g, right.g, sizeof(g))<0;
	/*
	for (unsigned int i=0; i < sizeof(RakNetGUID) / sizeof(RakNetGUID::g[0]); i++)
	{
		if (g[i]>right.g[i])
			return false;
		if (g[i]<right.g[i])
			return true;
	}
	return false;
	*/
}
const char *RakNetGUID::ToString(void) const
{
	static unsigned char strIndex=0;
	static char str[8][22];

	unsigned char lastStrIndex=strIndex;
	strIndex++;
	ToString(str[lastStrIndex&7]);
	return (char*) str[lastStrIndex&7];
}
void RakNetGUID::ToString(char *dest) const
{
	if (*this==UNASSIGNED_RAKNET_GUID)
		strcpy(dest, "UNASSIGNED_RAKNET_GUID");

	//sprintf(dest, "%u.%u.%u.%u.%u.%u", g[0], g[1], g[2], g[3], g[4], g[5]);
	sprintf(dest, "%" PRINTF_TIME_MODIFIER "u", g);
	// sprintf(dest, "%u.%u.%u.%u.%u.%u", g[0], g[1], g[2], g[3], g[4], g[5]);
}
bool RakNetGUID::FromString(const char *source)
{
	if (source==0)
		return false;

#if defined(WIN32) || defined(_XBOX) || defined(_X360)
	g=_atoi64(source);
#else
	g=atoll(source);
#endif
	return true;

	/*
	char intPart[128];
	unsigned int destIndex, sourceIndex=0, partIndex;
	for (partIndex=0; partIndex<sizeof(RakNetGUID) / sizeof(RakNetGUID::g[0]); partIndex++)
	{
		for (destIndex=0; source[sourceIndex] && source[sourceIndex]!='.' && destIndex<sizeof(intPart)-1; destIndex++, sourceIndex++)
		{
			if (source[sourceIndex]<'0' || source[sourceIndex]>'9')
				return false;
			intPart[destIndex]=source[sourceIndex];
		}
		if (source[sourceIndex]!='.' && partIndex<3)
		{
			// Failed
			return false;
		}
		if (destIndex==0)
		{
			// Failed
			return false;
		}
		intPart[destIndex]=0;
		g[partIndex]=strtoul(intPart,0,0);
		sourceIndex++;
	}

	return true;
	*/
}
