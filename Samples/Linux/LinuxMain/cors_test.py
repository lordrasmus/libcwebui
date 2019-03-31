#!/usr/bin/python
import httplib, urllib

from pprint import pprint

print("\n Teste CORS Header\n");

conn = httplib.HTTPConnection("localhost:8081")
#conn.putheader("test","a")

params = urllib.urlencode({'spam': 1, 'eggs': 2, 'bacon': 0})
headers = {"Origin":"www.google.de"}
conn.request("GET","/index.html", params, headers)

res = conn.getresponse()
#print res.status, res.reason

cors_header_ok = False
for a in  res.getheaders():
	if a[0] == "access-control-allow-origin":
		#print( a[1] ) 
		#if a[1] == "www.google.de":
		cors_header_ok = True



if cors_header_ok:
	print( "CORS OK")
else:
	print( "CORS Error")

print("")

for a in  res.getheaders():
	if ( a[0].startswith("access-control-") ):
		pprint( a )
