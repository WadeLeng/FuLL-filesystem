/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:25
# Filename: server.h
# Description: 
============================================================================*/
#ifndef FULLFS_SERVER
#define FULLFS_SERVER

#include "leveldb/db.h"
#include "leveldb/options.h"

extern int server_settings_cache;
extern char server_settings_dataname[1024];
extern leveldb::DB* db;
extern leveldb::Options full_options;

#endif
