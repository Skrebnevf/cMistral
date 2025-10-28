#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include "http_client.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MODEL "mistral-tiny"
#define DEFAULT_TEMPERATURE 0.7
#define DEFAULT_MAX_TOKENS 1024

int mistral_init(void) { return http_client_init(); }

void mistral_cleanup(void) { http_client_cleanup(); }

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

static char *create_fim_request_json(const mistral_config_t *config,
                                     const mistral_fim_t *fim) {
  cJSON *root = NULL;
  char *json_string = NULL;

  root = cJSON_CreateObject();
  if (root == NULL) {
    fprintf(stderr, "failed to create JSON object\n");
    return NULL;
  }

  if (cJSON_AddStringToObject(root, "model", config->model) == NULL) {
    fprintf(stderr, "failed to add model to JSON\n");
    goto error;
  }

  if (cJSON_AddStringToObject(root, "prompt", fim->prompt) == NULL) {
    fprintf(stderr, "failed to add prompt to JSON");
    goto error;
  }

  if (cJSON_AddStringToObject(root, "suffix", fim->suffix) == NULL) {
    fprintf(stderr, "failed to add suffix to JSON");
    goto error;
  }

  if (cJSON_AddNumberToObject(root, "temperature", config->temperature) ==
      NULL) {
    fprintf(stderr, "failed to add temperature to JSON\n");
    goto error;
  }

  if (cJSON_AddNumberToObject(root, "max_tokens", config->max_tokens) == NULL) {
    fprintf(stderr, "filed to add max_tokens to JSON\n");
    goto error;
  }

  json_string = cJSON_PrintUnformatted(root);
  if (json_string == NULL) {
    fprintf(stderr, "failed to printed JSON\n");
    goto error;
  }

  cJSON_Delete(root);
  return json_string;

error:
  if (root != NULL) {
    cJSON_Delete(root);
  }
  return NULL;
}

static char *create_chat_request_json(const mistral_config_t *config,
                                      const mistral_message_t *messages,
                                      size_t message_count) {
  cJSON *root = NULL;
  cJSON *messages_array = NULL;
  cJSON *message_obj = NULL;
  char *json_string = NULL;
  size_t i;

  root = cJSON_CreateObject();
  if (root == NULL) {
    fprintf(stderr, "failed to create JSON object\n");
    return NULL;
  }

  if (cJSON_AddStringToObject(root, "model", config->model) == NULL) {
    fprintf(stderr, "failed to add model to JSON\n");
    goto error;
  }

  messages_array = cJSON_CreateArray();
  if (messages_array == NULL) {
    fprintf(stderr, "failed to create messages array\n");
    goto error;
  }

  for (i = 0; i < message_count; i++) {
    message_obj = cJSON_CreateObject();
    if (message_obj == NULL) {
      fprintf(stderr, "failed to create message object\n");
      goto error;
    }

    if (cJSON_AddStringToObject(message_obj, "role", messages[i].role) ==
        NULL) {
      cJSON_Delete(message_obj);
      fprintf(stderr, "failed to add role to message\n");
      goto error;
    }

    if (cJSON_AddStringToObject(message_obj, "content", messages[i].content) ==
        NULL) {
      cJSON_Delete(message_obj);
      fprintf(stderr, "failed to add content to message\n");
      goto error;
    }

    cJSON_AddItemToArray(messages_array, message_obj);
  }

  cJSON_AddItemToObject(root, "messages", messages_array);

  if (cJSON_AddNumberToObject(root, "temperature", config->temperature) ==
      NULL) {
    fprintf(stderr, "failed to add temperature to JSON\n");
    goto error;
  }

  if (cJSON_AddNumberToObject(root, "max_tokens", config->max_tokens) == NULL) {
    fprintf(stderr, "failed to add max_tokens to JSON\n");
    goto error;
  }

  json_string = cJSON_PrintUnformatted(root);
  if (json_string == NULL) {
    fprintf(stderr, "failed to print JSON\n");
    goto error;
  }

  cJSON_Delete(root);
  return json_string;

error:
  if (root != NULL) {
    cJSON_Delete(root);
  }
  return NULL;
}

static int parse_response(const char *json_data, mistral_response_t *response) {
  cJSON *root = NULL;
  cJSON *choices = NULL;
  cJSON *first_choice = NULL;
  cJSON *message = NULL;
  cJSON *content = NULL;
  cJSON *usage = NULL;
  cJSON *item = NULL;

  memset(response, 0, sizeof(mistral_response_t));

  root = cJSON_Parse(json_data);
  if (root == NULL) {
    response->error_message = strdup("failed to parse JSON response");
    return -1;
  }

  item = cJSON_GetObjectItemCaseSensitive(root, "error");
  if (item != NULL) {
    cJSON *error_message = cJSON_GetObjectItemCaseSensitive(item, "message");
    if (error_message != NULL && cJSON_IsString(error_message)) {
      response->error_message = strdup(error_message->valuestring);
    } else {
      response->error_message = strdup("unknown API error");
    }
    cJSON_Delete(root);
    return -1;
  }

  item = cJSON_GetObjectItemCaseSensitive(root, "id");
  if (item != NULL && cJSON_IsString(item)) {
    response->id = strdup(item->valuestring);
  }

  item = cJSON_GetObjectItemCaseSensitive(root, "model");
  if (item != NULL && cJSON_IsString(item)) {
    response->model = strdup(item->valuestring);
  }

  choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
  if (choices != NULL && cJSON_IsArray(choices) &&
      cJSON_GetArraySize(choices) > 0) {
    first_choice = cJSON_GetArrayItem(choices, 0);
    if (first_choice != NULL) {
      message = cJSON_GetObjectItemCaseSensitive(first_choice, "message");
      if (message != NULL) {
        content = cJSON_GetObjectItemCaseSensitive(message, "content");
        if (content != NULL && cJSON_IsString(content)) {
          response->content = strdup(content->valuestring);
        }
      }
    }
  }

  usage = cJSON_GetObjectItemCaseSensitive(root, "usage");
  if (usage != NULL) {
    item = cJSON_GetObjectItemCaseSensitive(usage, "prompt_tokens");
    if (item != NULL && cJSON_IsNumber(item)) {
      response->prompt_tokens = item->valueint;
    }

    item = cJSON_GetObjectItemCaseSensitive(usage, "completion_tokens");
    if (item != NULL && cJSON_IsNumber(item)) {
      response->completion_tokens = item->valueint;
    }

    item = cJSON_GetObjectItemCaseSensitive(usage, "total_tokens");
    if (item != NULL && cJSON_IsNumber(item)) {
      response->total_tokens = item->valueint;
    }
  }

  cJSON_Delete(root);

  if (response->content == NULL) {
    response->error_message = strdup("no content in response");
    return -1;
  }

  return 0;
}

int mistral_fim_completions(const mistral_config_t *config,
                            const mistral_fim_t *fim,
                            mistral_response_t *response) {
  char *request_json = NULL;
  http_response_t http_resp = {0};
  char auth_headers[512];
  const char *headers[3];
  int ret = -1;

  if (config == NULL || config->api_key == NULL || fim == NULL ||
      fim->prompt == NULL || fim->suffix == NULL) {
    fprintf(stderr, "invalid arguments to mistral_fim_completions\n");
    return -1;
  }

  memset(response, 0, sizeof(mistral_response_t));

  request_json = create_fim_request_json(config, fim);
  if (request_json == NULL) {
    response->error_message = strdup("failed to create request JSON");
    return -1;
  }

  snprintf(auth_headers, sizeof(auth_headers), "authorization: Bearer %s",
           config->api_key);

  headers[0] = "Content-Type: application/json";
  headers[1] = auth_headers;
  headers[2] = NULL;

  if (http_post(MISTRAL_BASE_API "/fim/completions", headers, request_json,
                &http_resp) != 0) {
    response->error_message = strdup("HTTP request failed");
    goto cleanup;
  }

  if (http_resp.http_code != 200) {
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "HTTP error: %ld %s",
             http_resp.http_code, http_resp.data);
    response->error_message = strdup(error_msg);
    goto cleanup;
  }

  if (parse_response(http_resp.data, response) != 0) {
    goto cleanup;
  }

  ret = 0;

  return ret;

cleanup:
  if (request_json != NULL) {
    free(request_json);
  }
  http_response_free(&http_resp);

  return ret;
}

int mistral_chat_completions(const mistral_config_t *config,
                             const mistral_message_t *messages,
                             size_t message_count,
                             mistral_response_t *response) {
  char *request_json = NULL;
  http_response_t http_resp = {0};
  char auth_header[512];
  const char *headers[3];
  int ret = -1;

  if (config == NULL || config->api_key == NULL || messages == NULL ||
      message_count == 0 || response == NULL) {
    fprintf(stderr, "invalid arguments to mistral_chat_completions\n");
    return -1;
  }

  memset(response, 0, sizeof(mistral_response_t));

  request_json = create_chat_request_json(config, messages, message_count);
  if (request_json == NULL) {
    response->error_message = strdup("failed to create request JSON");
    return -1;
  }

  snprintf(auth_header, sizeof(auth_header), "authorization: Bearer %s",
           config->api_key);
  headers[0] = "Content-Type: application/json";
  headers[1] = auth_header;
  headers[2] = NULL;

  if (http_post(MISTRAL_BASE_API "/chat/completions", headers, request_json,
                &http_resp) != 0) {
    response->error_message = strdup("HTTP request failed");
    goto cleanup;
  }

  if (http_resp.http_code != 200) {
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "HTTP error: %ld %s",
             http_resp.http_code, http_resp.data);
    response->error_message = strdup(error_msg);
    goto cleanup;
  }

  if (parse_response(http_resp.data, response) != 0) {
    goto cleanup;
  }

  ret = 0;

  return ret;

cleanup:
  if (request_json != NULL) {
    free(request_json);
  }
  http_response_free(&http_resp);

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

    response->prompt_tokens = 0;
    response->completion_tokens = 0;
    response->total_tokens = 0;
  }
}
