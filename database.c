#include <iostream>
#include <pthread.h>
#include "leveldb/db.h"
#include "leveldb/comparator.h"
#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "server.h"
#include "utils.h"
using namespace std;

static pthread_mutex_t db_lock;

leveldb::Status opendb()
{
	pthread_mutex_init(&db_lock, NULL);
	options.create_if_missing = true;
	options.block_cache = leveldb::NewLRUCache(server_settings_cache * 1048576);//100MB cache
	leveldb::Status status = leveldb::DB::Open(options, server_settings_dataname, &db);
	if (!status.ok())
		cout << status.ToString() << endl;
	return status;
}

leveldb::Status put(const char* key, char* value, int value_size)
{
	pthread_mutex_lock(&db_lock);
	char *info = (char*) calloc(value_size + 11, sizeof(char));
	sprintf(info, "%d#%s", value_size, value);
	leveldb::Status s = db->Put(leveldb::WriteOptions(), key, info);
	free(info);
	pthread_mutex_unlock(&db_lock);
	return s;
}

leveldb::Status get(const char* key, char* value, int* value_size)
{
	string value;
	leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
	return s;
}

leveldb::Status delete(const char* key)
{
	pthread_mutex_lock(&db_lock);
	leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
	pthread_mutex_unlock(&db_lock);
	return s;
}

leveldb::Status deleteall()
{
	pthread_mutex_lock(&db_lock);
	leveldb::Status s = DestroyDB(server_settings_dataname, options);
	pthread_mutex_unlock(&db_lock);
	return opendb();
}

