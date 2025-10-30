#ifndef MISTRAL_HELPERS_H
#define MISTRAL_HELPERS_H

#include "../include/mistral.h"
#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

mistral_api_error_t *parse_api_error(cJSON *error_obj);

mistral_error_code_t determine_error_code(long http_code,
                                           const char *error_type);

char *create_fim_request_json(const mistral_config_t *config,
                             const mistral_fim_t *fim);

char *create_chat_request_json(const mistral_config_t *config,
                              const mistral_message_t *messages,
                              size_t message_count);

int parse_response(const char *json_data, long http_code,
                    mistral_response_t *response);

int execute_http_request_with_retry(const mistral_config_t *config,
                                    const char *endpoint,
                                    const char *request_json,
                                    mistral_response_t *response);

#ifdef __cplusplus
}
#endif

#endif /* MISTRAL_HELPERS_H */