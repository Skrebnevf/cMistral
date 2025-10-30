#ifndef MISTRAL_UTILS_H
#define MISTRAL_UTILS_H

#include "../include/mistral.h"

#ifdef __cplusplus
extern "C" {
#endif

void sleep_ms(int milliseconds);

int set_error_message(mistral_response_t *response, const char *message);

int validate_common_params(const mistral_config_t *config,
                             mistral_response_t *response);

#ifdef __cplusplus
}
#endif

#endif 
