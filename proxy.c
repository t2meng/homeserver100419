/*
 *  proxy.c
 *
 *  Proxy Server
 *
 *  Copyright (C) 2009  Huang Guan
 *
 *  2009-10-06 11:32:50 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  Proxy Server
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "debug.h"
#include "memory.h"
#include "util.h"
#include "server.h"
#include "connection.h"
#include "proxy.h"

#ifndef __WIN32__
#define closesocket close 
#endif

static int proxy_recv( connection* conn, int sock_xy )
{
	char* buf;
	int ret;
	NEW( buf, KB(4) );
	if( !buf ){
		closesocket( sock_xy );
		return -1;
	}
	while( (ret=recv( sock_xy, buf, KB(4), 0 )) > 0 ){
		send( conn->socket, buf, ret, 0 );
	}
	DEL( buf );
	closesocket( sock_xy );
	conn->state = C_READY;
	return 0;
}

int proxy_request( connection* conn, const char* ip, int port )
{
	struct sockaddr_in addr;
	int sock_xy, i;
	char* header;
	if ( (sock_xy = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP )) <=0 ){
		perror("[proxy] Init Socket Error.");
		return -1;
	}
	memset( &addr, 0, sizeof(struct sockaddr_in) );
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr( ip );
	addr.sin_port = htons( port );
	if( connect( sock_xy, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0 )
	{
		perror("[proxy] connect failed.");
		return -2;
	}
	// prepare header
	NEW( header, KB(4) );
	if( !header ){
		closesocket( sock_xy );
		return -3;
	}
	sprintf( header, "%s %s %s\r\n", conn->request_method, conn->uri, conn->http_version );
	
	for( i=0; i<conn->header_num; i++ ){
		if( stricmp( conn->headers[i].name, "Connection" )==0 )
			sprintf( header, "%s%s: %s\r\n", header, 
				conn->headers[i].name, "Close" );
		else
			sprintf( header, "%s%s: %s\r\n", header, 
				conn->headers[i].name, conn->headers[i].value );
	}
	//Maybe this could be optional!!
	sprintf( header, "%s%s: %s\r\n", header, 
		"X-Forwarded-For", conn->client->ip_string );
	strcat( header, "\r\n" );
	if( send( sock_xy, header, strlen(header), 0 ) < 0 )
		printf("[proxy] send failed.\n");
	DEL( header );
	if( conn->form_size > 0 )
		send( sock_xy, conn->form_data, conn->form_size, 0 );
	return proxy_recv( conn, sock_xy );
}
