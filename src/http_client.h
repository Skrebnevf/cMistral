#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char *data;
  size_t size;
  long http_code;
} http_response_t;

/*
* Global libcurl init client. Call once.
*/
int http_client_init(void);

/*
* libcurl cleanup
*/
void http_client_cleanup(void);

/*
* Return 0 if ok, -1 if error
*/
int http_post(const char *url, const char **headers, const char *body, http_response_t *response);

/*
* libcurl free
*/
void http_response_free(http_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
