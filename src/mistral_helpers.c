#define _POSIX_C_SOURCE 200809L

#include "mistral_helpers.h"
#include "mistral_utils.h"
#include "http_client.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG_LOG(...) ((void)0)

mistral_api_error_t *parse_api_error(cJSON *error_obj) {
  mistral_api_error_t *api_error = NULL;
  cJSON *item = NULL;

  if (error_obj == NULL || !cJSON_IsObject(error_obj)) {
    return NULL;
  }

  api_error = (mistral_api_error_t *)malloc(sizeof(mistral_api_error_t));
  if (api_error == NULL) {
    return NULL;
  }

  memset(api_error, 0, sizeof(mistral_api_error_t));

  item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
  if (item != NULL && cJSON_IsString(item)) {
    api_error->message = strdup(item->valuestring);
    if (api_error->message == NULL) {
      mistral_api_error_free(api_error);
      return NULL;
    }
  }

  item = cJSON_GetObjectItemCaseSensitive(error_obj, "type");
  if (item != NULL && cJSON_IsString(item)) {
    api_error->type = strdup(item->valuestring);
    if (api_error->type == NULL) {
      mistral_api_error_free(api_error);
      return NULL;
    }
  }

  item = cJSON_GetObjectItemCaseSensitive(error_obj, "param");
  if (item != NULL && cJSON_IsString(item)) {
    api_error->param = strdup(item->valuestring);
    if (api_error->param == NULL) {
      mistral_api_error_free(api_error);
      return NULL;
    }
  }

  item = cJSON_GetObjectItemCaseSensitive(error_obj, "code");
  if (item != NULL) {
    if (cJSON_IsString(item)) {
      api_error->code = strdup(item->valuestring);
      if (api_error->code == NULL) {
        mistral_api_error_free(api_error);
        return NULL;
      }
    } else if (cJSON_IsNumber(item)) {
      char code_buf[32];
      snprintf(code_buf, sizeof(code_buf), "%d", item->valueint);
      api_error->code = strdup(code_buf);
      if (api_error->code == NULL) {
        mistral_api_error_free(api_error);
        return NULL;
      }
    }
  }

  return api_error;
}

mistral_error_code_t determine_error_code(long http_code,
                                           const char *error_type) {
  if (http_code == 200) {
    return MISTRAL_OK;
  }

  if (http_code == 401 || http_code == 403) {
    return MISTRAL_ERR_AUTH;
  }

  if (http_code == 429) {
    return MISTRAL_ERR_RATE_LIMIT;
  }

  if (http_code >= 500) {
    return MISTRAL_ERR_SERVER;
  }

  if (http_code == 408) {
    return MISTRAL_ERR_TIMEOUT;
  }

  if (error_type != NULL) {
    if (strstr(error_type, "authentication") != NULL) {
      return MISTRAL_ERR_AUTH;
    }
    if (strstr(error_type, "rate_limit") != NULL) {
      return MISTRAL_ERR_RATE_LIMIT;
    }
  }

  return MISTRAL_ERR_NETWORK;
}

char *create_fim_request_json(const mistral_config_t *config,
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
    fprintf(stderr, "failed to add max_tokens to JSON\n");
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

char *create_chat_request_json(const mistral_config_t *config,
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

int parse_response(const char *json_data, long http_code,
                    mistral_response_t *response) {
  cJSON *root = NULL;
  cJSON *choices = NULL;
  cJSON *first_choice = NULL;
  cJSON *message = NULL;
  cJSON *content = NULL;
  cJSON *usage = NULL;
  cJSON *item = NULL;

  memset(response, 0, sizeof(mistral_response_t));
  response->http_code = http_code;

  root = cJSON_Parse(json_data);
  if (root == NULL) {
    response->error_message = strdup("failed to parse JSON response");
    if (response->error_message == NULL) {
      response->error_code = MISTRAL_ERR_MEM;
      return -1;
    }
    response->error_code = MISTRAL_ERR_PARSE;
    return -1;
  }

  item = cJSON_GetObjectItemCaseSensitive(root, "error");
  if (item != NULL && cJSON_IsObject(item)) {
    response->api_error = parse_api_error(item);

    if (response->api_error != NULL && response->api_error->message != NULL) {
      response->error_message = strdup(response->api_error->message);
    } else {
      response->error_message = strdup("unknown API error");
    }

    response->error_code = determine_error_code(
        http_code, response->api_error ? response->api_error->type : NULL);

    cJSON_Delete(root);
    return -1;
  }

  if (http_code != 200) {
    char msg[256];
    snprintf(msg, sizeof(msg), "HTTP error %ld: ", http_code);

    item = cJSON_GetObjectItemCaseSensitive(root, "message");
    if (item != NULL && cJSON_IsString(item)) {
      strncat(msg, item->valuestring, sizeof(msg) - strlen(msg) - 1);
    } else {
      strncat(msg, "no error details provided", sizeof(msg) - strlen(msg) - 1);
    }

    response->error_message = strdup(msg);
    response->error_code = determine_error_code(http_code, NULL);

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
    response->error_message = strdup("no content in successful response");
    response->error_code = MISTRAL_ERR_PARSE;
    return -1;
  }

  response->error_code = MISTRAL_OK;
  return 0;
}

int execute_http_request_with_retry(const mistral_config_t *config,
                                    const char *endpoint,
                                    const char *request_json,
                                    mistral_response_t *response) {
  http_response_t http_resp = {0};
  char auth_header[512];
  const char *headers[3];
  int ret = -1;
  int attempt = 0;
  int retry_delay = config->retry_delay_ms;

  int written = snprintf(auth_header, sizeof(auth_header),
                         "authorization: Bearer %s", config->api_key);
  if (written >= (int)sizeof(auth_header)) {
    fprintf(stderr, "authorization header too long\n");
    if (set_error_message(response, "authorization header too long") != 0) {
      return -1;
    }
    response->error_code = MISTRAL_ERR_INVALID_PARAM;
    return -1;
  }

  headers[0] = "Content-Type: application/json";
  headers[1] = auth_header;
  headers[2] = NULL;

  for (attempt = 0; attempt <= config->max_retries; attempt++) {
    if (attempt > 0) {
      DEBUG_LOG("retry attempt %d/%d after %d ms delay", attempt,
                config->max_retries, retry_delay);
      sleep_ms(retry_delay);

      retry_delay *= 2;
      if (retry_delay > 30000) {
        retry_delay = 30000;
      }
    }

    DEBUG_LOG("Sending HTTP POST request attempt %d", attempt + 1);

    http_response_free(&http_resp);
    memset(&http_resp, 0, sizeof(http_resp));

    if (http_post(endpoint, headers, request_json, &http_resp) != 0) {
      DEBUG_LOG("HTTP request failed");

      if (attempt < config->max_retries) {
        continue;
      }

      if (set_error_message(response, "HTTP request failed") != 0) {
        return -1;
      }
      response->error_code = MISTRAL_ERR_NETWORK;
      goto cleanup;
    }

    DEBUG_LOG("received HTTP response: code=%ld, size=%zu", http_resp.http_code,
              http_resp.size);

    if (http_resp.http_code == 200) {
      DEBUG_LOG("request successful");

      if (parse_response(http_resp.data, http_resp.http_code, response) != 0) {
        DEBUG_LOG("failed to parse response");
        goto cleanup;
      }

      ret = 0;
      goto cleanup;

    } else if (http_resp.http_code == 429) {
      DEBUG_LOG("rate limit hit (429), will retry");

      parse_response(http_resp.data, http_resp.http_code, response);

      retry_delay = retry_delay * 2;
      if (retry_delay < 5000) {
        retry_delay = 5000;
      }

      if (attempt < config->max_retries) {
        mistral_response_free(response);
        memset(response, 0, sizeof(mistral_response_t));
        continue;
      }

      if (response->error_message == NULL) {
        if (set_error_message(response, "rate limit exceeded") != 0) {
          return -1;
        }
      }
      response->error_code = MISTRAL_ERR_RATE_LIMIT;
      goto cleanup;

    } else if (http_resp.http_code >= 500 && http_resp.http_code < 600) {
      DEBUG_LOG("server error (%ld), will retry", http_resp.http_code);

      parse_response(http_resp.data, http_resp.http_code, response);

      if (attempt < config->max_retries) {
        mistral_response_free(response);
        memset(response, 0, sizeof(mistral_response_t));
        continue;
      }

      if (response->error_message == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "server error: %ld", http_resp.http_code);
        char *error_msg = strdup(msg);
        if (error_msg == NULL) {
          response->error_code = MISTRAL_ERR_MEM;
          return -1;
        }
        response->error_message = error_msg;
      }
      response->error_code = MISTRAL_ERR_SERVER;
      goto cleanup;

    } else {
      DEBUG_LOG("Non-retryable error: %ld", http_resp.http_code);

      if (attempt > 0) {
        mistral_response_free(response);
        memset(response, 0, sizeof(mistral_response_t));
      }

      if (parse_response(http_resp.data, http_resp.http_code, response) != 0) {
        DEBUG_LOG("Error parsed from response");
      } else {
        if (response->error_message == NULL) {
          char msg[128];
          snprintf(msg, sizeof(msg), "HTTP error: %ld", http_resp.http_code);
          char *error_msg = strdup(msg);
          if (error_msg == NULL) {
            response->error_code = MISTRAL_ERR_MEM;
            return -1;
          }
          response->error_message = error_msg;
        }
        response->error_code = determine_error_code(http_resp.http_code, NULL);
      }

      goto cleanup;
    }
  }

  DEBUG_LOG("exhausted all retry attempts");
  if (response->error_message == NULL) {
    if (set_error_message(response, "all retry attempts failed") != 0) {
      return ret;
    }
    response->error_code = MISTRAL_ERR_NETWORK;
  }

cleanup:
  http_response_free(&http_resp);
  return ret;
}