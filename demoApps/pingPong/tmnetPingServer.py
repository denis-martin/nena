import os, sys
lib_path = os.path.abspath('../tmnet')
sys.path.append(lib_path)
import tmnet

tmnet.init()

#tmnet_set_plugin_option("tmnet::nena", "ipcsocket", socketPath.c_str());

print("set option")
tmnet.set_plugin_option("tmnet::nena", "ipcsocket", "/tmp/nena_socket_node02")

print("bind to uri")
listen = tmnet.bind("app://localhost.node02/ping_tmnet", "")

while(True):
    print("wait on handle")
    listen.wait()

    print("accept new handle")
    con = listen.accept()

    print("read data from handle")
    data = con.read(4)

    print(data)

    print("write data to handle")
    con.write(data)
