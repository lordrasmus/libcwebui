#!/usr/bin/python3

import os, sys, socket, struct, json, traceback


from pprint import pprint



def main_loop( ip ):

	print("\nwnfs client\n")

	print("connectiong to : " + ip + " ... ")

	sock = socket.socket(   socket.AF_INET, socket.SOCK_STREAM)
	sock.connect((ip, 4444))

	print("connected")

	while True:

		lb = sock.recv( 4 )
		l = struct.unpack('I', lb )[0]
		data = sock.recv( l )
		j_data = json.loads( data.decode("utf-8")  )


		if j_data["op"] == "get_file":
			file_name = j_data["file"]
			file_path = "data/" + file_name

			if not os.path.exists( file_path ):
				print("file not found : " + file_name + "  " )
				sock.send( struct.pack('I', 0 ) )

			else:
				size = os.path.getsize( file_path )
				#print("get_file : " + file_name  + " " + str(size) + " Bytes ")
				sock.send( struct.pack('I', size ) )

				f = open( file_path , "rb" )
				f_data = f.read(size)

				l = sock.send(f_data)
				while True:
					if not ( l == len( f_data ) ):
						print("l1 : " + str( l ) + " l2: " + str( size ) )
						l += sock.send(f_data[l:])
						continue

					break

				f.close()

			continue


		if j_data["op"] == "put_file":

			file_name = j_data["file"]
			file_path = "data/" + file_name

			print("store new file : " + file_name )

			lb = sock.recv( 4 )
			l = struct.unpack('I', lb )[0]

			data = sock.recv( l )
			while not ( l == len( data ) ):
				print("l1 : " + str( l ) + " l2: " + str( len( data ) ) )
				data += sock.recv( l - len( data ) )

			if not os.path.exists( os.path.dirname ( file_path ) ):
				os.system("mkdir -p " + os.path.dirname ( file_path ))

			if not ( l == len( data ) ):
				 raise NameError('File incomplete')

			f = open( file_path , "wb" )
			f.write( data )
			f.close()

			continue

		print( j_data )


		sock.send( struct.pack('I', 0 ) )

def main_start(ip ):

	if not os.path.exists("data"):
		os.system("mkdir data")

	while True:
		try:
			main_loop( ip )
		except KeyboardInterrupt:
			sys.exit(0)
		except:
			traceback.print_exc()


main_start( "192.168.100.1" )
