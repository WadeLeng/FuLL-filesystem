/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:11
# Filename: database.c
# Description: 
============================================================================*/
#include <iostream>
#include <pthread.h>
#include <stdlib.h>

#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "database.h"
#include "server.h"

using namespace std;

static pthread_mutex_t db_lock;

leveldb::Status opendb()
{
	pthread_mutex_init(&db_lock, NULL);
	full_options.create_if_missing = true;
	full_options.block_cache = leveldb::NewLRUCache(server_settings_cache * 1048576);//100MB cache
	leveldb::Status status = leveldb::DB::Open(full_options, server_settings_dataname, &db);
	if (!status.ok())
		cout << status.ToString() << endl;
	return status;
}

leveldb::Status put(const char* key, const char* value, int value_size)
{
	pthread_mutex_lock(&db_lock);
	char *info = (char*) calloc(value_size + 12, sizeof(char));
	sprintf(info, "%-11d#%s", value_size, value);
	leveldb::Status s = db->Put(leveldb::WriteOptions(), key, info);
	free(info);
	pthread_mutex_unlock(&db_lock);
	return s;
}

leveldb::Status get(const char* key, char** value, int* value_size)
{
	string value_str;
	const char* info;
	int i;

	leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value_str);
	if (!s.ok())
	{
		*value = NULL;
		*value_size = 0;
		return s;
	}
	info = value_str.c_str();
	sscanf(info, "%d #", value_size);
	*value = (char*) malloc((*value_size) * sizeof(char) + 1);
	for (i = 0; i < *value_size; i++)
		(*value)[i] = info[i + 12];
	(*value)[*value_size] = '\0';

	return s;
}

leveldb::Status clear(const char* key)
{
	pthread_mutex_lock(&db_lock);
	leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
	pthread_mutex_unlock(&db_lock);
	return s;
}

// TODO: other interface
