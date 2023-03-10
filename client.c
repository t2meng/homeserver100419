/*
 *  client.c
 *
 *  Client
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-9 22:49:22 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  Client
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <wininet.h>
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "debug.h"
#include "memory.h"
#include "server.h"
#include "connection.h"
#include "client.h"
#include "http.h"

client* client_create( struct cweb_server* server, uint ip )
{
	client* c;
	if( server->client_num >= server->max_clients )
	{
		DBG("failed server->client_num: %d", server->client_num );
		return NULL;
	}
	NEW( c, sizeof(client) );
	LINK_APPEND( c, server->first_client );
	server->client_num ++;
	c->server = server;
	c->ip = ip;
	struct in_addr addr;
	addr.s_addr = htonl( c->ip );
	strncpy( c->ip_string, inet_ntoa( addr ), 31 );
	
	c->time_create = time( NULL );
	pthread_mutex_init( &c->mutex_session, NULL );
	pthread_mutex_init( &c->mutex_conn, NULL );
	return c;
}

int client_live( client* c )
{
	//close all connections
	pthread_mutex_lock( &c->mutex_conn );
	connection* conn;
	time_t time_now = time(NULL);
	for( conn=c->first_conn; conn;  ){
		connection* next = conn->next;
		if( time_now - conn->time_alive > c->server->conn_timeout )
		{	//if timeout, shutdown it, that will let the thread return
			shutdown( conn->socket, SHUT_RDWR );
#ifdef __WIN32__
			closesocket( conn->socket );
#else
			close( conn->socket );
#endif
			conn->state = C_END;
			DBG("# close timeout conn %s:%d", conn->script_name, conn->socket );
		}
		conn = next;
	}
	//close all sessions
	pthread_mutex_lock( &c->mutex_session );
	session* s;
	for( s=c->first_session; s; ){
		session* next = s->next;
		if( s->reference <= 0 && time_now - s->time_alive > c->server->session_timeout ){
			LINK_DELETE( s, c->first_session );
			DBG("# delete session %.16s  ref: %d timeout: %d", s->key, s->reference, 
				time_now - s->time_alive );
			DEL( s );
			c->session_num --;
		}
		s = next;
	}
	pthread_mutex_unlock( &c->mutex_session );
	pthread_mutex_unlock( &c->mutex_conn );
	if( c->first_conn==NULL && c->first_session==NULL ){
		pthread_mutex_destroy( &c->mutex_conn );
		pthread_mutex_destroy( &c->mutex_session);
		LINK_DELETE( c, c->server->first_client );
	//	struct in_addr addr;
	//	addr.s_addr = htonl( c->ip );
	//	DBG("# delete unused client %s", inet_ntoa( addr ) );
		c->server->client_num --;
		DEL( c );
	}
	return 0;
}


int client_end( client* c )
{
	//close all connections
	pthread_mutex_lock( &c->mutex_conn );
	connection* conn;
	for( conn=c->first_conn; conn; ){
		connection* next = conn->next;
		shutdown( conn->socket, SHUT_RDWR );
#ifdef __WIN32__
		closesocket( conn->socket );
#else
		close( conn->socket );
#endif
		conn->state = C_END;
		conn = next;
	}
	//close all sessions
	pthread_mutex_lock( &c->mutex_session );
	session* s;
	for( s=c->first_session; s; ){
		session* next = s->next;
		if( s->reference > 0 ){
			DBG("##Fatal Error: session->reference = %d", s->reference );
		}
		LINK_DELETE( s, c->first_session );
		DEL( s );
		s = next;
	}
	pthread_mutex_unlock( &c->mutex_session );
	pthread_mutex_unlock( &c->mutex_conn );
	//is it safe to do things below? 091205 by HG
	pthread_mutex_destroy( &c->mutex_session );
	pthread_mutex_destroy( &c->mutex_conn );
	DEL( c );
	return 0;
}

void client_print( client* c )
{
	char timestr[64];
	int n;
	format_time( c->time_create, timestr );
	DBG("  IP: %s", c->ip_string );
	DBG("  Create time: %s", timestr );
	DBG("  Connection count: %d", c->conn_num );
	
	pthread_mutex_lock( &c->mutex_conn );
	pthread_mutex_lock( &c->mutex_session );
	connection* conn;
	for( conn=c->first_conn, n=0 ; conn; conn=conn->next, n++ ){
		DBG("    Connecion [%d]", n );
		format_time( conn->time_create, timestr );
		DBG("      Create time: %s", timestr );
		format_time( conn->time_alive, timestr );
		DBG("      Alive time: %s", timestr );
		DBG("      Requests: %d", conn->requests );
		DBG("      Current dir: %s", conn->current_dir );
		DBG("      Full path: %s", conn->full_path );
		DBG("      State: %d", conn->state );
	}
	DBG("\tSession count: %d", c->session_num );
	session* s;
	for( s=c->first_session; s; s=s->next ){
		DBG("    Session [%.16s]", s->key );
		format_time( s->time_create, timestr );
		DBG("      Create time: %s", timestr );
		format_time( s->time_alive, timestr );
		DBG("      Alive time: %s", timestr );
		DBG("      Reference: %d", s->reference );
	}
	pthread_mutex_unlock( &c->mutex_session );
	pthread_mutex_unlock( &c->mutex_conn );
}
