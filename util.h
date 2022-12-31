#ifndef _UTIL_H
#define _UTIL_H

int parse_path( char* path );
int get_line( char* buf, int size, int* pos, char* line, int max );
char* http_ext_desc( char* ext );
char* http_code_string(int no);
void format_time(time_t t, char* buf);
int is_dir( const char* path );
int mkdir_recursive( char* path );

#endif
