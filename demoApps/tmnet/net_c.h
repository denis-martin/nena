/*
 * net_c.h
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#ifndef TMNET_C_H_
#define TMNET_C_H_

#include <stddef.h>

#include "net_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tmnet_nhandle;
typedef struct tmnet_nhandle* tmnet_pnhandle;
typedef char* tmnet_req_t;

int tmnet_init();

int tmnet_bind(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
int tmnet_wait(tmnet_pnhandle handle);
int tmnet_accept(tmnet_pnhandle handle, tmnet_pnhandle* inc);
int tmnet_get_property(tmnet_pnhandle handle, const char* key, char** value);
int tmnet_free_property(char* key);

int tmnet_connect(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);

int tmnet_read(tmnet_pnhandle handle, char* buf, size_t* size);
int tmnet_write(tmnet_pnhandle handle, const char* buf, size_t size);

int tmnet_readmsg(tmnet_pnhandle handle, char* buf, size_t* size);
int tmnet_writemsg(tmnet_pnhandle handle, const char* buf, size_t size);

int tmnet_close(tmnet_pnhandle handle);

int tmnet_isEndOfStream(tmnet_pnhandle handle);

// optional

int tmnet_set_option(tmnet_pnhandle handle, const char* name, const char* value);
int tmnet_set_plugin_option(const char* plugin, const char* name, const char* value);

int tmnet_get(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
int tmnet_put(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* TMNET_C_H_ */
