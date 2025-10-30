#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include "mistral_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
void sleep_ms(int milliseconds) { Sleep(milliseconds); }
#else
#include <unistd.h>
void sleep_ms(int ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}
#endif

const char *mistral_error_string(mistral_error_code_t code) {
  switch (code) {
  case MISTRAL_OK:
    return "success";
  case MISTRAL_ERR_INVALID_PARAM:
    return "invalid parameter";
  case MISTRAL_ERR_NETWORK:
    return "network error";
  case MISTRAL_ERR_AUTH:
    return "authentication error";
  case MISTRAL_ERR_RATE_LIMIT:
    return "rate limit exceeded";
  case MISTRAL_ERR_SERVER:
    return "server error";
  case MISTRAL_ERR_PARSE:
    return "JSON parse error";
  case MISTRAL_ERR_TIMEOUT:
    return "timeout";
  case MISTRAL_ERR_MEM:
    return "memory allocation error";
  default:
    return "unknown error";
  }
}

int set_error_message(mistral_response_t *response, const char *message) {
  char *msg = strdup(message);
  if (msg == NULL) {
    response->error_code = MISTRAL_ERR_MEM;
    return -1;
  }
  response->error_message = msg;
  return 0;
}

int validate_common_params(const mistral_config_t *config,
                             mistral_response_t *response) {
  if (config == NULL || config->api_key == NULL || response == NULL) {
    fprintf(stderr, "invalid arguments: config, api_key or response is NULL\n");
    if (response != NULL) {
      memset(response, 0, sizeof(mistral_response_t));
      if (set_error_message(response, "Invalid parameters") != 0) {
        return -1;
      }
      response->error_code = MISTRAL_ERR_INVALID_PARAM;
    }
    return -1;
  }

  if (mistral_config_validate(config) != 0) {
    if (set_error_message(response, "invalid configuration") != 0) {
      return -1;
    }
    response->error_code = MISTRAL_ERR_INVALID_PARAM;
    return -1;
  }

  return 0;
}