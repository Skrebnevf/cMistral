#include "http_client.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_valid_url(const char *url) {
  if (url == NULL || strlen(url) == 0)
    return 0;
  return strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total_size = size * nmemb;
  http_response_t *response = (http_response_t *)userdata;

  char *new_data = realloc(response->data, response->size + total_size + 1);
  if (new_data == NULL) {
    fprintf(stderr, "failed to allocate memory in cb\n");
    return 0;
  }

  response->data = new_data;

  memcpy(&(response->data[response->size]), ptr, total_size);
  response->size += total_size;
  response->data[response->size] = '\0';

  return total_size;
}

int http_client_init(void) {
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl global init failed: %s\n", curl_easy_strerror(res));
    return -1;
  }
  return 0;
}

void http_client_cleanup(void) { curl_global_cleanup(); }

int http_post(const char *url, const char **headers, const char *body,
              http_response_t *response) {
  if (!is_valid_url(url)) {
    fprintf(stderr, "incorrect URL\n");
  }

  CURL *curl = NULL;
  CURLcode res;
  struct curl_slist *header_list = NULL;
  int ret = -1;

  response->data = NULL;
  response->size = 0;
  response->http_code = 0;

  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "curl_easy_init failed\n");
    return -1;
  }

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    fprintf(stderr, "CURLOPT_URL failed: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_POST, 1L);
  if (res != CURLE_OK) {
    fprintf(stderr, "CURLOPT_POST failed: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  if (body != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    if (res != CURLE_OK) {
      fprintf(stderr, "CURLOPT_POSTFIELDS failed: %s\n",
              curl_easy_strerror(res));
      goto cleanup;
    }
  }

  if (headers != NULL) {
    int i = 0;
    while (headers[i] != NULL) {
      header_list = curl_slist_append(header_list, headers[i]);
      i++;
    }
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    if (res != CURLE_OK) {
      fprintf(stderr, "CURLOPT_HTTPHEADER failed: %s\n",
              curl_easy_strerror(res));
      goto cleanup;
    }
  }
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl perform failed: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_code);

  ret = 0;

cleanup:
  if (header_list != NULL) {
    curl_slist_free_all(header_list);
  }
  if (curl != NULL) {
    curl_easy_cleanup(curl);
  }

  if (ret != 0 && response->data != NULL) {
    free(response->data);
    response->data = NULL;
    response->size = 0;
  }

  return ret;
}

void http_response_free(http_response_t *response) {
  if (response != NULL && response->data != NULL) {
    free(response->data);
    response->data = NULL;
    response->size = 0;
    response->http_code = 0;
  }
}
