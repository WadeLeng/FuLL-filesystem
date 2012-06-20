/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:11
# Filename: database.h
# Description: 封装的leveldb接口
============================================================================*/
#ifndef FULLFS_DATABASE
#define FULLFS_DATABASE

#include "leveldb/db.h"

leveldb::Status opendb();
leveldb::Status put(const char* key, const char* value, int value_size);
leveldb::Status get(const char* key, char** value, int* value_size);
leveldb::Status clear(const char* key);

#endif
