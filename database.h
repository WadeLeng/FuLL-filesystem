#ifndef FULLFS_DATABASE
#define FULLFS_DATABASE

#include "leveldb/status.h"

leveldb::Status opendb();
leveldb::Status put(const char* key, const char* value, int value_size);
leveldb::Status get(const char* key, char** value, int* value_size);
leveldb::Status clear(const char* key);

#endif
