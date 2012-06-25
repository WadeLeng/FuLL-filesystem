#!/usr/bin/python

'This is a test program: put some key-value to FuLL filesystem'

import httplib
import urllib
import threading

key = [x for x in range(500)]
value = [x for x in range(500)]

def main():
	headers = {'Content-type' : 'text/plain'}

	conn = httplib.HTTPConnection("127.0.0.1 :11211")

	for i in range(500):
		url = '/?opt=put&key=' + str(key[i])
		conn.request('POST', url, str(value[i]), headers)

		response = conn.getresponse()
		print response.status, response.reason

		data = response.read()
		print data

	conn.close

if __name__ == "__main__":
	main()
