#!/usr/bin/python

'This is a test program: get some key-value from FuLL filesystem'

import httplib
import urllib
import threading

key = [x for x in range(500)]
value = [x for x in range(500)]

def main():
	headers = {'Content-type' : 'text/plain'}

	conn = httplib.HTTPConnection("127.0.0.1 :11211")

	for i in range(500):
		url = 'http://127.0.0.1 :11211/?opt=get&key=' + str(key[i])
		handle = urllib.urlopen(url)
		print 'key: %s  value: %s' % (str(key[i]), handle.read())

	conn.close

if __name__ == "__main__":
	main()
