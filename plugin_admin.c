/*
 *  plugin_admin.c
 *
 *  Console Program
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-8-31 10:24:34 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#ifdef __WIN32__
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "debug.h"
#include "server.h"
#include "connection.h"
#include "vdir.h"
#include "session.h"
#include "client.h"

enum{
	V_NONE = 0,
	V_GUEST,
	V_ADMIN
};

uint last_ip;

static int admin_run( connection* conn )
{
	session * sess = conn->session;
	if( (int)sess->data == V_NONE ){
		sess->data = (void*)V_GUEST;
	}
	if( stricmp( conn->file_name, "exit.do" ) == 0 ){
		if( (int)sess->data == V_ADMIN ){
			printf("exit.do\n");
#ifdef __WIN32__
			ExitProcess(1024);
#endif
		}
	}else if( stricmp( conn->file_name, "login.do" ) == 0 ){
		char pass[16];
//		if( last_ip == ((client*)conn->client)->ip )
//			return -1;
		conn->param_get( conn, "password", pass, 15 );
		if( strcmp( pass, "huang123" )==0 ){
			sess->data = (void*)V_ADMIN;
			strcpy( conn->data_send, "ok$^" );
		}else{
			sess->data = (void*)V_GUEST;
			last_ip = ((client*)conn->client)->ip;
			sprintf( conn->data_send, "error$^Password is incorrect. Your IP is %X", last_ip );
		}
	}else if( stricmp( conn->file_name, "server_info.do" ) == 0 ){
		webserver* server = conn->server;
		strcpy( conn->data_send, "<table>" );
		sprintf( conn->data_send, "%s<tr><td>server_name</td><td>%s</td></tr>\r\n", 
			conn->data_send, server->server_name );
		sprintf( conn->data_send, "%s<tr><td>server_port</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->server_port );
		sprintf( conn->data_send, "%s<tr><td>state</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->state );
		sprintf( conn->data_send, "%s<tr><td>root_dir</td><td>%s</td></tr>\r\n", 
			conn->data_send, server->root_dir );
		sprintf( conn->data_send, "%s<tr><td>max_clients</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->max_clients );
		sprintf( conn->data_send, "%s<tr><td>max_onlines</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->max_onlines );
		sprintf( conn->data_send, "%s<tr><td>terminal_log</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->terminal_log );
		sprintf( conn->data_send, "%s<tr><td>file_log</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->file_log );
		sprintf( conn->data_send, "%s<tr><td>conn_timeout</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->conn_timeout );
		sprintf( conn->data_send, "%s<tr><td>session_timeout</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->session_timeout );
		sprintf( conn->data_send, "%s<tr><td>client_num</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->client_num );
		sprintf( conn->data_send, "%s<tr><td>max_threads</td><td>%d</td></tr>\r\n", 
			conn->data_send, server->max_threads );
		strcat( conn->data_send, "</table>" );
	}
	return 0;
}


#define EXPORT __declspec(dllexport) __stdcall 
int EXPORT plugin_entry( webserver* srv )
{
	srv->vdir_create( srv, "/admin", (vdir_handler)admin_run );
	return 0;
}

int EXPORT plugin_cleanup( webserver* srv )
{
	return 0;
}
