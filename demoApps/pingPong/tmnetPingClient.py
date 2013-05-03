import os, sys
lib_path = os.path.abspath('../tmnet')
sys.path.append(lib_path)
import tmnet

tmnet.init()

con = tmnet.connect("app://localhost.node02/ping_tmnet", "")
con.write("PI")
print(con.read(4))