#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include "http_client.h"
#include "mistral_helpers.h"
#include "mistral_utils.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_MODEL "mistral-tiny"
#define DEFAULT_TEMPERATURE 0.7
#define DEFAULT_MAX_TOKENS 1024
#define DEFAULT_MAX_RETRIES 3
#define DEFAULT_RETRY_DELAY_MS 1000
#define DEFAULT_TIMEOUT_SEC 60

int execute_embeddings_http_request_with_retry(const mistral_config_t *config,
                                               const char *endpoint,
                                               const char *request_json,
                                               mistral_embeddings_response_t *response);

#ifdef DEBUG
static int g_debug_enabled = 0;
#define DEBUG_LOG(...)                                                         \
  do {                                                                         \
    fprintf(stderr, "[---DEBUG---] ");                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#else
#define DEBUG_LOG(...) ((void)0)
#define g_debug_enabled 0;
#endif

/*Init the Mistral lib include HTTP client*/
int mistral_init(void) { return http_client_init(); }

/*Cleanup all Mistral resources*/
void mistral_cleanup(void) { http_client_cleanup(); }

void mistral_set_debug(int enabled) {
#ifdef DEBUG
  g_debug_enabled = enabled;
  DEBUG_LOG("debug mode %s", enabled ? "enabled" : "disabled");
#else
  (void)enabled;
#endif
}

mistral_config_t *mistral_config_create(const char *api_key) {
  mistral_config_t *config = NULL;

  if (api_key == NULL) {
    fprintf(stderr, "API key cannot be NULL\n");
    return NULL;
  }

  config = (mistral_config_t *)malloc(sizeof(mistral_config_t));
  if (config == NULL) {
    fprintf(stderr, "failed to allocate memory for config\n");
    return NULL;
  }

  memset(config, 0, sizeof(mistral_config_t));

  config->api_key = strdup(api_key);
  if (config->api_key == NULL) {
    fprintf(stderr, "failed to duplicate API key\n");
    free(config);
    return NULL;
  }

  config->model = strdup(DEFAULT_MODEL);
  if (config->model == NULL) {
    fprintf(stderr, "failed to duplicate model name\n");
    free(config->api_key);
    free(config);
    return NULL;
  }

  config->temperature = DEFAULT_TEMPERATURE;
  config->max_tokens = DEFAULT_MAX_TOKENS;
  config->max_retries = DEFAULT_MAX_RETRIES;
  config->retry_delay_ms = DEFAULT_RETRY_DELAY_MS;
  config->timeout_sec = DEFAULT_TIMEOUT_SEC;
  config->debug_mode = 0;

  return config;
}

void mistral_config_free(mistral_config_t *config) {
  if (config != NULL) {
    if (config->api_key != NULL) {
      free(config->api_key);
    }
    if (config->model != NULL) {
      free(config->model);
    }
    free(config);
  }
}

int mistral_config_validate(const mistral_config_t *config) {
  if (config == NULL) {
    DEBUG_LOG("config is NULL");
    return -1;
  }

  if (config->api_key == NULL || strlen(config->api_key) == 0) {
    DEBUG_LOG("API key is empty");
    return -1;
  }

  if (config->model == NULL || strlen(config->model) == 0) {
    DEBUG_LOG("model is empty");
    return -1;
  }

  if (config->temperature < 0.0 || config->temperature > 2.0) {
    DEBUG_LOG("temperature out of range: %.2f", config->temperature);
    return -1;
  }

  if (config->max_tokens < 1 || config->max_tokens > 32000) {
    DEBUG_LOG("max tokens out of range: %d", config->max_tokens);
    return -1;
  }

  if (config->max_retries < 0 || config->max_retries > 10) {
    DEBUG_LOG("max retries out of range: %d", config->max_retries);
    return -1;
  }

  if (config->timeout_sec < 1 || config->timeout_sec > 300) {
    DEBUG_LOG("timeout out of range: %d", config->timeout_sec);
    return -1;
  }

  return 0;
}

void mistral_api_error_free(mistral_api_error_t *error) {
  if (error != NULL) {
    if (error->message != NULL) {
      free(error->message);
    }
    if (error->type != NULL) {
      free(error->type);
    }
    if (error->param != NULL) {
      free(error->param);
    }
    if (error->code != NULL) {
      free(error->code);
    }
    free(error);
  }
}

int mistral_embeddings(const mistral_config_t *config,
                        const mistral_embeddings_t *embeddings,
                        size_t input_count,
                        mistral_embeddings_response_t *response) {
  char *request_json = NULL;
  int ret = -1;
  int debug_was_enabled = 0;

  if (config == NULL || config->api_key == NULL || embeddings == NULL ||
      input_count == 0 || response == NULL) {
    fprintf(stderr, "invalid arguments to mistral_embeddings\n");
    if (response != NULL) {
      memset(response, 0, sizeof(mistral_embeddings_response_t));
      response->error_message = strdup("Invalid parameters");
      if (response->error_message == NULL) {
        return -1;
      }
      response->error_code = MISTRAL_ERR_INVALID_PARAM;
    }
    return -1;
  }

  memset(response, 0, sizeof(mistral_embeddings_response_t));

  if (validate_common_params(config, (mistral_response_t *)response) != 0) {
    return -1;
  }

  if (config->debug_mode) {
    mistral_set_debug(1);
    debug_was_enabled = 1;
#ifdef DEBUG
    DEBUG_LOG("debug mode enabled via config");
#endif
  }

  DEBUG_LOG("starting embeddings request");
  DEBUG_LOG("Model: %s", config->model);
  DEBUG_LOG("Input count: %zu", input_count);
  DEBUG_LOG("Max retries: %d, Timeout: %d seconds", config->max_retries,
            config->timeout_sec);

  request_json = create_embeddings_json(config, embeddings, input_count);
  if (request_json == NULL) {
    response->error_message = strdup("failed to create request JSON");
    if (response->error_message == NULL) {
      return -1;
    }
    response->error_code = MISTRAL_ERR_MEM;
    return -1;
  }

  DEBUG_LOG("request JSON created (length: %zu)", strlen(request_json));

  ret = execute_embeddings_http_request_with_retry(
      config, MISTRAL_BASE_API "/embeddings", request_json, response);

  if (request_json != NULL) {
    free(request_json);
  }

  if (debug_was_enabled) {
    mistral_set_debug(0);
#ifdef DEBUG
    DEBUG_LOG("debug mode disabled");
#endif
  }

  return ret;
}

int mistral_fim_completions(const mistral_config_t *config,
                            const mistral_fim_t *fim,
                            mistral_response_t *response) {
  char *request_json = NULL;
  int ret = -1;
  int debug_was_enabled = 0;

  if (config == NULL || config->api_key == NULL || fim == NULL ||
      fim->prompt == NULL || fim->suffix == NULL) {
    fprintf(stderr, "invalid arguments to mistral_fim_completions\n");
    if (response != NULL) {
      memset(response, 0, sizeof(mistral_response_t));
      if (set_error_message(response, "Invalid parameters") != 0) {
        return -1;
      }
      response->error_code = MISTRAL_ERR_INVALID_PARAM;
    }
    return -1;
  }

  memset(response, 0, sizeof(mistral_response_t));

  if (validate_common_params(config, response) != 0) {
    return -1;
  }

  if (config->debug_mode) {
    mistral_set_debug(1);
    debug_was_enabled = 1;
#ifdef DEBUG
    DEBUG_LOG("debug mode enabled via config");
#endif
  }

  DEBUG_LOG("starting FIM completion request");
  DEBUG_LOG("Model: %s, Temperature: %.2f, Max tokens: %d", config->model,
            config->temperature, config->max_tokens);
  DEBUG_LOG("Max retries: %d, Timeout: %d seconds", config->max_retries,
            config->timeout_sec);

  request_json = create_fim_request_json(config, fim);
  if (request_json == NULL) {
    if (set_error_message(response, "failed to create request JSON") != 0) {
      return -1;
    }
    response->error_code = MISTRAL_ERR_MEM;
    return -1;
  }

  DEBUG_LOG("request JSON created (length: %zu)", strlen(request_json));

  ret = execute_http_request_with_retry(
      config, MISTRAL_BASE_API "/fim/completions", request_json, response);

  if (request_json != NULL) {
    free(request_json);
  }

  if (debug_was_enabled) {
    mistral_set_debug(0);
#ifdef DEBUG
    DEBUG_LOG("debug mode disabled");
#endif
  }

  return ret;
}

int mistral_chat_completions(const mistral_config_t *config,
                             const mistral_message_t *messages,
                             size_t message_count,
                             mistral_response_t *response) {
  char *request_json = NULL;
  int ret = -1;
  int debug_was_enabled = 0;

  if (config == NULL || config->api_key == NULL || messages == NULL ||
      message_count == 0 || response == NULL) {
    fprintf(stderr, "invalid arguments to mistral_chat_completions\n");
    if (response != NULL) {
      memset(response, 0, sizeof(mistral_response_t));
      if (set_error_message(response, "Invalid parameters") != 0) {
        return -1;
      }
      response->error_code = MISTRAL_ERR_INVALID_PARAM;
    }
    return -1;
  }

  memset(response, 0, sizeof(mistral_response_t));

  if (validate_common_params(config, response) != 0) {
    return -1;
  }

  if (config->debug_mode) {
    mistral_set_debug(1);
    debug_was_enabled = 1;
#ifdef DEBUG
    DEBUG_LOG("debug mode enabled via config");
#endif
  }

  DEBUG_LOG("starting chat completion request");
  DEBUG_LOG("Model: %s, Temperature: %.2f, Max tokens: %d", config->model,
            config->temperature, config->max_tokens);
  DEBUG_LOG("Max retries: %d, Timeout: %d seconds", config->max_retries,
            config->timeout_sec);

  request_json = create_chat_request_json(config, messages, message_count);
  if (request_json == NULL) {
    if (set_error_message(response, "failed to create request JSON") != 0) {
      return -1;
    }
    response->error_code = MISTRAL_ERR_MEM;
    return -1;
  }

  DEBUG_LOG("request JSON created (length: %zu)", strlen(request_json));

  ret = execute_http_request_with_retry(
      config, MISTRAL_BASE_API "/chat/completions", request_json, response);

  if (request_json != NULL) {
    free(request_json);
  }

  if (debug_was_enabled) {
    mistral_set_debug(0);
#ifdef DEBUG
    DEBUG_LOG("debug mode disabled");
#endif
  }

  return ret;
}

void mistral_response_free(mistral_response_t *response) {
  if (response != NULL) {
    if (response->id != NULL) {
      free(response->id);
      response->id = NULL;
    }
    if (response->model != NULL) {
      free(response->model);
      response->model = NULL;
    }
    if (response->content != NULL) {
      free(response->content);
      response->content = NULL;
    }
    if (response->error_message != NULL) {
      free(response->error_message);
      response->error_message = NULL;
    }
    if (response->api_error != NULL) {
      mistral_api_error_free(response->api_error);
      response->api_error = NULL;
    }
    response->prompt_tokens = 0;
    response->completion_tokens = 0;
    response->total_tokens = 0;
    response->error_code = MISTRAL_OK;
    response->http_code = 0;
  }
}

void mistral_embeddings_response_free(mistral_embeddings_response_t *response) {
  if (response != NULL) {
    if (response->id != NULL) {
      free(response->id);
      response->id = NULL;
    }
    if (response->model != NULL) {
      free(response->model);
      response->model = NULL;
    }
    if (response->object != NULL) {
      free(response->object);
      response->object = NULL;
    }
    if (response->error_message != NULL) {
      free(response->error_message);
      response->error_message = NULL;
    }
    if (response->api_error != NULL) {
      mistral_api_error_free(response->api_error);
      response->api_error = NULL;
    }
    if (response->data != NULL) {
      size_t i = 0;
      while (i < 1000 && (response->data[i].embending != NULL || i == 0)) {
        if (response->data[i].embending != NULL) {
          free(response->data[i].embending);
        }
        if (response->data[i].object != NULL) {
          free(response->data[i].object);
        }
        i++;
      }
      free(response->data);
      response->data = NULL;
    }
    if (response->usage != NULL) {
      free(response->usage);
      response->usage = NULL;
    }
    response->error_code = MISTRAL_OK;
    response->http_code = 0;
  }
}
