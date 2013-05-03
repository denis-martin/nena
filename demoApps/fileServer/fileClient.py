import os, sys
lib_path = os.path.abspath('../tmnet')
sys.path.append(lib_path)
import tmnet

tmnet.init()

con = tmnet.get("app://localhost.node02/file/testFile", "")
#con.write("GET")
#print(con.read(4))
print(con.read(1024*1024).decode("ascii"))
con.close()