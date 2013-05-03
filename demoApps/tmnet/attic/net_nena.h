/*
 * ani_nena.h
 *
 *  Created on: 7 Jun 2011
 *      Author: denis
 */

#ifndef TMNET_NENA_H_
#define TMNET_NENA_H_

#include "net.h"

class INenai;

namespace kit  {
namespace tm  {
namespace net {
namespace nena {

typedef enum {ipctype_socket, ipctype_memory} ipctype;

void set_ipctype(ipctype type);
void set_ipcsocket(const std::string& filename);

nstream* get(const std::string& uri, void* req = NULL);

nstream* put(const std::string& uri, void* req = NULL);

nstream* connect(const std::string& uri, void* req = NULL);

nhandle publish(const std::string& uri, void* req = NULL);

}
}
}
}

#endif /* TMNET_NENA_H_ */
