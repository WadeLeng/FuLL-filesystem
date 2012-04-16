#!/usr/bin/env  python
#15th APR. 2012

importsys,urllib2

file1=open("dolce.html","w")
ifnotfile1:
    print"no file"

req=urllib2.Request(sys.argv[1]) 
try:
    fd=urllib2.urlopen(req)
#打开连接并得到一个文件类对象
excepturllib2.URLError,e:
    print"Error retrieving data:",e
    sys.exit(1)
print"Retrieved",fd.geturl()
info=fd.info()
#http header info
forkey,valueininfo.items():
    print"%s = %s"% (key,value)

#把下载的数据写入文件
while1:
    data=fd.read(1024)
    ifnotlen(data):
        break
    file1.write(data)
print'work done!'


#Get请求：
importsys,urllib,urllib2

defaddGETdata(url,data):
    """Adds data to url. Data should be a list or tuple consisting of 2-itemlists or tuples of the form: (key, value).
    Items that have no key should have key set to None.
    A given key may occur more than once.
    """
    returnurl+'?'+urllib.urlencode(data)

zipcode=sys.argv[1]
url=addGETdata('http://www.dolce.com',[('query',zipcode)])

url=url+'&wuSelect=INFOMATION'
#生成最终的url请求地址
print"Using URL",url
req=urllib2.Request(url)
fd=urllib2.urlopen(req)
while1:
    data=fd.read(1024)
    ifnotlen(data):
        break
    sys.stdout.write(data)

#POST请求：
importsys,urllib,urllib2

zipcode=sys.argv[1]

url='http://www.dolce.com'
data=urllib.urlencode([('query',zipcode),('wuSelect','INFOMATION')])#对表单中键和值进行编码
printdata
req=urllib2.Request(url)
fd=urllib2.urlopen(req,data)  #附加数据通过第二个参数传入
while1:
    data=fd.read(1024)
    ifnotlen(data):
        break
    sys.stdout.write(data)
