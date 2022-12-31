/*
 *  plugin_home.cpp
 *
 *  Console Program
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-8-29 23:19:35 Created.
 *  2008-12-6 Updated
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include "debug.h"
#include "server.h"
#include "connection.h"
#include "vdir.h"
#include "CPrettyText.h"
#include "session.h"
#include "client.h"
using namespace PString;

static CPrettyText pt;
static pthread_mutex_t pt_mutex;
static char* bufReadPage = NULL;

enum{
	V_NONE = 0,
	V_GUEST,
	V_ADMIN
};

static char* toTime( const char* ts )
{
	static char buf[24];
	int t;
	t = atol( ts );
	if( t==-1 ) t= time(NULL);
	if( localtime((time_t*)&t) )
		strftime( buf, 64, "%Y-%m-%d %X", localtime((time_t*)&t) );
	else 
		return toTime( "-1" );
	return buf;
}

static char* mkTime( const char* s1 )
{
	static char s2[24];
	struct tm tm1;
	time_t t;
	strcpy( s2, s1 );
	sscanf( s2, "%4d-%2d-%2d %2d:%2d:%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, 
		&tm1.tm_hour, &tm1.tm_min, &tm1.tm_sec );
	tm1.tm_year-=1900;
	tm1.tm_mon --;
//	printf("year %d, mon: %d day: %d  hour: %d  min: %d\n", tm1.tm_year, tm1.tm_mon, tm1.tm_mday, 
//		tm1.tm_hour, tm1.tm_min );
	t = mktime( &tm1 );
	itoa( t, s2, 10 );
	return s2;
}

static int last_write_time = 0, write_count = 0;
static int last_backup_time = 0;
static int check_writable()
{
	if( time(NULL)-last_write_time > 3600 ){
		last_write_time = time(NULL);
		write_count = 0;
		return 1;
	}
	write_count ++;
	if( write_count < 36 ){
		return 1;
	}
	return 0;
}

static void backup()
{
	if( last_backup_time == 0 )
		last_backup_time = time(NULL);
	if( time(NULL) - last_backup_time > 3600*2 ){
		last_backup_time = time(NULL);
		pt.Save();
		printf("backup ok\n");
	}
}

static int home_run( connection* conn )
{
	session * sess = conn->session;
	pthread_mutex_lock( &pt_mutex );	//目前只允许一线程进入
	backup();
	if( (int)sess->data == V_NONE ){
		sess->data = (void*)V_GUEST;
		pt.Contents("/website/row/connects") = 
			ToString( atoi( pt.Contents("/website/row/connects").c_str() ) + 1 );
	}
//	printf("asp_run %s  session:%X u:%X", conn->file_name, (uint)conn->session, (uint)u );
	
	if( stricmp( conn->file_name, "connects.do" ) == 0 ){
		sprintf( conn->data_send, "ok$^%s", pt.Contents("/website/row/connects").c_str() );
	}else if( stricmp( conn->file_name, "login.do" ) == 0 ){
		char pass[16];
		conn->param_get( conn, "password", pass, 15 );
		if( pt.Contents("/website/row/password") == pass ){
			sess->data = (void*)V_ADMIN;
			strcpy( conn->data_send, "ok$^" );
		}else{
			sess->data = (void*)V_GUEST;
			strcpy( conn->data_send, "error$^Password is incorrect." );
		}
	}else if( stricmp( conn->file_name, "logout.do" ) == 0 ){
		sess->data = (void*)V_GUEST;
		strcpy( conn->data_send, "ok$^" );
	}else if( stricmp( conn->file_name, "topic.do" ) == 0 ){
		char s_type[6], s_count[6], s_pos[6], s_detail[6], 
			s_updateTime[24], s_dateTime[24];
		int type, count, pos, len, detail;
		conn->param_get( conn, "belong", s_type, 5 );	type = atoi( s_type );
		conn->param_get( conn, "count", s_count, 5 );	count = atoi( s_count );
		conn->param_get( conn, "pos", s_pos, 5 );	pos = atoi( s_pos );
		conn->param_get( conn, "detail", s_detail, 5 );	detail = atoi( s_detail );
		strcpy( conn->data_send, "ok" );
		pt.Enter("/forum");
		if( pt.Child() ){
			while( pos > 0 ){
				if( pt.Contents("belong") == s_type ){
					pos --;
				}
				if( !pt.Next() ){
					count = 0;
					break;
				}
			}
			while( count > 0 ){
				if( pt.Contents("belong") == s_type ){
					//潜在内存溢出问题
					strncpy( s_dateTime, toTime(pt.Contents("dateTime").c_str()), 23 );
					strncpy( s_updateTime, toTime(pt.Contents("updateTime").c_str()), 23 );
					if( detail==0 ){
						len = sprintf( conn->data_send, "%s$^%s$#%s$#%s$#%s$#%s", conn->data_send, 
							pt.Contents("id").c_str(), pt.Contents("title").c_str(), s_dateTime
							, pt.Contents("clicks").c_str(), pt.Contents("comments").c_str() );
					}else if( detail == 1 ){
						len = sprintf( conn->data_send, "%s$^%s$#%s$#%s$#%s$#%s$#%s$#%s$#%s", conn->data_send, 
							pt.Contents("id").c_str(), pt.Contents("name").c_str(), pt.Contents("title").c_str(), 
							pt.Contents("content").c_str(), s_dateTime, pt.Contents("clicks").c_str(), 
							s_updateTime, pt.Contents("comments").c_str() );
					}else if( detail == 2 ){
						len = sprintf( conn->data_send, "%s$^%s$#%s$#%s$#%s$#%s$#%s$#%s$#%s$#%s", conn->data_send, 
							pt.Contents("id").c_str(), pt.Contents("name").c_str(), pt.Contents("title").c_str(), 
							pt.Contents("content").c_str(), s_dateTime, pt.Contents("clicks").c_str(), 
							pt.Contents("belong").c_str(), s_updateTime, pt.Contents("comments").c_str() );
					}
					if( len + KB(16)> conn->data_size ){
						printf("len: %d is too large!!", len );
						break;
					}
					count --;
				}
				if( !pt.Next() )
					break;
			}
		}
	}else if( stricmp( conn->file_name, "view.do" ) == 0 ){
		char s_id[6], s_updateTime[24], s_dateTime[24];
		conn->param_get( conn, "id", s_id, 5 );	
		pt.Enter("/forum");
		if( !pt.FindFirst("id", s_id) ){
			strcpy( conn->data_send, "error$^No this topic." );
		}else{
			strncpy( s_dateTime, toTime(pt.Contents("dateTime").c_str()), 23 );
			strncpy( s_updateTime, toTime(pt.Contents("updateTime").c_str()), 23 );
			sprintf( conn->data_send, "ok$^%s$#%s$#%s$#%s$#%s$#%s$#%s$#%s$#%s",  
				pt.Contents("id").c_str(), pt.Contents("name").c_str(), pt.Contents("title").c_str(), 
				pt.Contents("content").c_str(), s_dateTime, pt.Contents("clicks").c_str(), 
				pt.Contents("belong").c_str(), s_updateTime, pt.Contents("comments").c_str() );
			pt.Contents("clicks") = 
				ToString( atoi( pt.Contents("clicks").c_str() ) + 1 );
		}
	}else if( stricmp( conn->file_name, "read.do" ) == 0 ){
		char s_id[6], s_updateTime[24], s_dateTime[24];
		conn->param_get( conn, "id", s_id, 5 );	
		pt.Enter("/forum");
		if( !pt.FindFirst("id", s_id) ){
			strcpy( conn->data_send, "error$^No this topic." );
		}else{
			strncpy( s_dateTime, toTime(pt.Contents("dateTime").c_str()), 23 );
			strncpy( s_updateTime, toTime(pt.Contents("updateTime").c_str()), 23 );
			//replace
			string buf = bufReadPage;
			Replace( buf, "[%title%]", pt.Contents("name") );
			Replace( buf, "[%id%]", pt.Contents("id") );
			Replace( buf, "[%updateTime%]", s_updateTime );
			Replace( buf, "[%dateTime%]", s_dateTime );
			Replace( buf, "[%clicks%]", pt.Contents("clicks") );
			Replace( buf, "[%belong%]", pt.Contents("belong") );
			Replace( buf, "[%comments%]", pt.Contents("comments") );
			Replace( buf, "[%title%]", pt.Contents("title") );
			Replace( buf, "[%isAdmin%]", ToString( (int)sess->data ) );
			string content = pt.Contents("content");
			Replace( content, "    ", "&nbsp;&nbsp;&nbsp;&nbsp;" );
			Replace( buf, "[%content%]", content );
			strncpy( conn->data_send, buf.c_str(), conn->data_size );
			pt.Contents("clicks") = 
				ToString( atoi( pt.Contents("clicks").c_str() ) + 1 );
		}
	}else if( stricmp( conn->file_name, "init_comment_count.do" ) == 0 ){	
		//add commentCount to all topics 08-12-06 by Huang Guan
		if( (int)sess->data != V_ADMIN ){
			strcpy( conn->data_send, "error$^you are not the master here." );
		}else{
			pt.Enter("/forum");
			pt.Child();
			while( 1 ){
				string s_id, s_belong;
				int id;
				int n = 0;
				s_id = pt.Contents("id");
				id = atoi( s_id.c_str() ) + 32;
				s_belong = PString::ToString( id );
				pt.Enter("/idea");
				pt.Child();
				while( pt.FindNext("belong", s_belong) ){
					n++;
				}
				pt.Enter("/forum");
				if( pt.FindFirst( "id", s_id ) ){
					pt.Contents("comments") = PString::ToString( n );
					sprintf( conn->data_send, "%s<p>%s: %d</p>", conn->data_send, pt.Contents("name").c_str(), n );
				}else{
					break;
				}
				if( !pt.Next() ){
					break;
				}
			}
			pt.Save();
		}
	}else if( stricmp( conn->file_name, "comment.do" ) == 0 ){
		char s_type[6], s_count[6], s_pos[6], s_detail[6];
		int type, count, pos, len, detail;
		conn->param_get( conn, "belong", s_type, 5 );	type = atoi( s_type );
		conn->param_get( conn, "count", s_count, 5 );	count = atoi( s_count );
		conn->param_get( conn, "pos", s_pos, 5 );	pos = atoi( s_pos );
		conn->param_get( conn, "detail", s_detail, 5 );	detail = atoi( s_detail );
		strcpy( conn->data_send, "ok" );
		pt.Enter("/idea");
		pt.Child();
		while( pos > 0 ){
			if( pt.Contents("belong") == s_type ){
				pos --;
			}
			if( !pt.Next() ){
				count=0;	//末尾
				break;
			}
		}
		while( count > 0 ){
			if( pt.Contents("belong") == s_type ){
				//潜在内存溢出问题
				if( detail==0 ){
					len = sprintf( conn->data_send, "%s$^%s$#%s$#%s$#%s", conn->data_send, 
						pt.Contents("name").c_str(), toTime(pt.Contents("updateTime").c_str()), 
						pt.Contents("mail").c_str(), pt.Contents("content").c_str() );
				}else if( detail == 1 ){
					len = sprintf( conn->data_send, "%s$^%s$#%s$#%s$#%s$#%s$#%s", conn->data_send, 
						pt.Contents("id").c_str(), pt.Contents("name").c_str(), 
						toTime(pt.Contents("updateTime").c_str()), pt.Contents("mail").c_str(), 
						pt.Contents("content").c_str(), pt.Contents("ip").c_str() );
				}
				if( len + KB(16)> conn->data_size ){
					printf("len: %d is too large!!", len );
					break;
				}
				count --;
			}
			if( !pt.Next() )
				break;
		}
	}else if( stricmp( conn->file_name, "write.do" ) == 0 ){
		char s_belong[6], s_name[32], *s_content, *s_mail, s_updateTime[24], s_id[6];
		s_content = (char*)malloc( KB(1) );
		s_mail = (char*)malloc( 128 );
		if( !s_content || !s_mail ){
			printf("memory low.");
		}else if( !check_writable() ){
			strcpy( conn->data_send, "error$^too many topics you've sent." );
		}else{
			int id;
			id = atoi(pt.Contents("/website/row/id").c_str());
			id ++;
			sprintf( s_id, "%d", id );
			pt.Contents("/website/row/id") = s_id;	//next id
			
			conn->form_get( conn, "belong", s_belong, 5 );	
			conn->form_get( conn, "name", s_name, 31 );
			conn->form_get( conn, "content", s_content, 1023 );
			conn->form_get( conn, "mail", s_mail, 127 );
			sprintf( s_updateTime, "%u", time(NULL) );
			pt.Enter("/idea");
			pt.Append("row");
			pt.Child();
			pt.Last();
			pt.Contents("id") = s_id;
			pt.Contents("name") = s_name;
			pt.Contents("mail") = s_mail;
			pt.Contents("content") = s_content;
			pt.Contents("ip") = ((client*)conn->client)->ip_string;
			pt.Contents("updateTime") = s_updateTime;
			pt.Contents("belong") = s_belong;
			pt.Enter("/forum");
			if( pt.FindFirst("id", PString::ToString( atoi(s_belong)-32 ) ) ){
				pt.Contents("comments") = 
					ToString( atoi( pt.Contents("comments").c_str() ) + 1 );
			} 
			pt.Enter("/idea");
			//以updateTime进行降序排序
			pt.SortBy( "updateTime", 1 );
			pt.Save();
			strcpy( conn->data_send, "ok" );
		}
		if( s_content ) free( (void*)s_content );
		if( s_mail ) free( (void*)s_mail );
	}else if( stricmp( conn->file_name, "edit.do" ) == 0 ){
		char s_id[6], s_belong[6], s_name[32], *s_title, *s_content, 
			s_dateTime[24], s_updateTime[24], s_clicks[6], s_comments[6];
		s_content = (char*)malloc( KB(400) );
		s_title = (char*)malloc( KB(4) );
		if( !s_content || !s_title ){
			printf("memory low.");
		}else if( !check_writable() ){
			strcpy( conn->data_send, "error$^too many topics you've sent." );
		}else if( (int)sess->data != V_ADMIN ){
			strcpy( conn->data_send, "error$^you are not the master here." );
		}else{
			conn->form_get( conn, "id", s_id, 5 );	
			conn->form_get( conn, "belong", s_belong, 5 );	
			conn->form_get( conn, "name", s_name, 31 );
			conn->form_get( conn, "title", s_title, KB(4)-1 );
			conn->form_get( conn, "content", s_content, KB(400)-1 );
			conn->form_get( conn, "dateTime", s_dateTime, 23 );
			conn->form_get( conn, "clicks", s_clicks, 5 );
			conn->form_get( conn, "comments", s_comments, 5 );
			sprintf( s_updateTime, "%u", time(NULL) );
			int id = atoi(s_id);
			pt.Enter("/forum");
			if( id==0 ){	//create
				id = atoi(pt.Contents("/website/row/id").c_str());
				id ++;
				sprintf( s_id, "%d", id );
				pt.Contents("/website/row/id") = s_id;	//next id
				pt.Enter("/forum");
				pt.Append("row");
				pt.Child();
				pt.Last();
				pt.Contents("id") = s_id;
			}
			pt.Enter("/forum");
			if( !pt.FindFirst("id", s_id) ){
				strcpy( conn->data_send, "error$^not found this topic." );
			}else{
				printf("edit topic ok! id=%s\n", s_id);
				pt.Contents("name") = s_name;
				pt.Contents("title") = s_title;
				pt.Contents("content") = s_content;
				pt.Contents("dateTime") = mkTime( s_dateTime );
				pt.Contents("clicks") = s_clicks;
				pt.Contents("comments") = s_comments;
				pt.Contents("belong") = s_belong;
				pt.Contents("updateTime") = s_updateTime;
				pt.Enter("/forum");
				//以dateTime进行降序排序
				pt.SortBy( "dateTime", 1 );
				pt.Save();
				strcpy( conn->data_send, "ok" );
			}
		}
		if(s_content) free( (void*)s_content );
		if(s_title) free( (void*)s_title );
	}else if( stricmp( conn->file_name, "deltopic.do" ) == 0 ){
		char s_id[6], s_belong[6];
		if( (int)sess->data != V_ADMIN ){
			strcpy( conn->data_send, "error$^you are not the master here." );
		}else{
			int id;
			conn->param_get( conn, "id", s_id, 6 );
			id = atoi( s_id );	
			sprintf( s_belong, "%d", id+32 );
			printf("delete topic %s\n", s_id );
			//delete its comments
			pt.Enter("/idea");
			while( pt.FindFirst("belong", s_belong ) ){
				pt.Delete();
				pt.Enter("/idea");
				printf("delete one comment.\n");
			}
			pt.Enter("/forum");
			if( pt.FindFirst( "id", s_id ) ){
				pt.Delete();
				strcpy( conn->data_send, "ok" );
			}else{
				strcpy( conn->data_send, "error$^not found this topic." );
			}
		}
	}else if( stricmp( conn->file_name, "delcomment.do" ) == 0 ){
		char s_id[6];
		if( (int)sess->data != V_ADMIN ){
			strcpy( conn->data_send, "error$^you are not the master here." );
		}else{
			conn->param_get( conn, "id", s_id, 6 );	
			pt.Enter("/idea");
			if( pt.FindFirst( "id", s_id ) ){
				pt.Delete();
				strcpy( conn->data_send, "ok" );
			}else{
				strcpy( conn->data_send, "error$^not found this comment." );
			}
		}
	}else{
		strcpy( conn->data_send, "error$^wrong script name." );;
	}
	pthread_mutex_unlock( &pt_mutex );
	
//	sprintf( conn->data_send, "<h1>This is a test!! </h1><p>Request Script Name: %s", conn->file_name );
	return 0;
}

static int home_init()
{
	pthread_mutex_init( &pt_mutex, NULL );
	if( pt.Open("./web/home/#data.txt" ) != ERR_OK )
		return -1;
	pt.Enter("/forum");
	//以dateTime进行降序排序
	pt.SortBy( "dateTime", 1 );
	pt.Enter("/idea");
	//以updateTime进行降序排序
	pt.SortBy( "updateTime", 1 );
	//加载阅读页面代码
	FILE* fp = fopen("./web/home/read.htm" , "rb" );
	if( fp == NULL )
		return -2;
	int file_size = filelength( fileno(fp) );
	bufReadPage = (char*)malloc( file_size+1 );
	if( fread( bufReadPage, file_size, 1, fp ) < 1 )
		printf("read file error: %s\n", "./web/home/read.htm" );
	bufReadPage[file_size] = '\0';
	fclose( fp );
	return 0;
}

static void home_end()
{
	pthread_mutex_lock( &pt_mutex );
	pt.Save();
	pt.Clear();
	if( bufReadPage )
		free( bufReadPage );
	pthread_mutex_unlock( &pt_mutex );
}
#define EXPORT __declspec(dllexport) __stdcall 
extern "C" int EXPORT plugin_entry( webserver* srv )
{
	if( home_init()<0 )
		return -1;	//will init config file
	srv->vdir_create( srv, "/home", (vdir_handler)home_run );
	return 0;
}

extern "C" int EXPORT plugin_cleanup( webserver* srv )
{
	home_end();
	return 0;
}
