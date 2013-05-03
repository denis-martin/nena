/*
 * test.cpp
 *
 *  Created on: 7 Jun 2011
 *      Author: denis
 */

#include "net.h"
#include "net_nena.h"

#include <iostream>

using namespace std;
using namespace kit::tm;

int main(int argc, char** argv)
{
	string s;
	net::nstream* ns;

	// IPC via shared memory currently broken (stalls)
	//net::nena::set_ipctype(net::nena::ipctype_memory);

	ns = net::get("nena://localhost/info");
	while (!ns->eof()) {
		char buf[256];
		ns->read(buf, 10);
		s += string(buf, ns->gcount());
	}
	cout << s << endl;
	delete ns;


	ns = net::get("nena://localhost/netlets");
	while (!ns->eof()) {
		char buf[256];
		ns->getline(buf, 256);
		cout << buf << endl;
	}
	delete ns;


	ns = net::put("nena://localhost/logmessage");
	*ns << "Hello World!";
	ns->flush();
	delete ns;


	ns = net::put("node://edu.kit.tm/itm/node01");
	delete ns;


	ns = net::connect("node://edu.kit.tm/itm/node02");
	delete ns;


//	net::nhandle nh = net::publish("video://bigbuckbunny.org/BigBuckBunny");


	cout << endl;
	return 0;
}
