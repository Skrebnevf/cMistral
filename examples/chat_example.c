#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  mistral_config_t *config = NULL;
  mistral_response_t response = {0};
  const char *api_key = NULL;
  int ret = 1;

  api_key = getenv("MISTRAL_API_KEY");
  if (api_key == NULL) {
    fprintf(stderr, "error: MISTRAL_API_KEY environment variable not set\n");
    fprintf(stderr, "usage: export MISTRAL_API_KEY='your-api-key'\n");
    return 1;
  }

  if (mistral_init() != 0) {
    fprintf(stderr, "failed to initialize Mistral library\n");
    return 1;
  }

  printf("Mistral Chat Example\n");
  printf("====================\n\n");

  config = mistral_config_create(api_key);
  if (config == NULL) {
    fprintf(stderr, "failed to create config\n");
    goto cleanup;
  }

  config->temperature = 0.7;
  config->max_tokens = 200;

  config->max_retries = 3;
  config->retry_delay_ms = 1000;
  config->timeout_sec = 60;
  config->debug_mode = 1;

  free(config->model);
  config->model = strdup("mistral-small-latest");

  printf("Configuration:\n");
  printf("  Model: %s\n", config->model);
  printf("  Temperature: %.2f\n", config->temperature);
  printf("  Max tokens: %d\n", config->max_tokens);
  printf("  Max retries: %d\n", config->max_retries);
  printf("  Retry delay: %d ms\n", config->retry_delay_ms);
  printf("  Timeout: %d seconds\n", config->timeout_sec);
  printf("  Debug mode: %s\n\n", config->debug_mode ? "enabled" : "disabled");

  mistral_message_t messages[] = {
      {.role = "system",
       .content = "You are a helpful assistant that provides concise answers."},
      {.role = "user", .content = "Whai is C lang?"}};

  printf("sending request...\n\n");

  if (mistral_chat_completions(config, messages, 2, &response) != 0) {
    printf("\n=== ERROR ===\n");
    printf("error code: %d (%s)\n", response.error_code,
           mistral_error_string(response.error_code));
    printf("HTTP code: %ld\n", response.http_code);

    if (response.error_message != NULL) {
      printf("message: %s\n", response.error_message);
    }

    if (response.api_error != NULL) {
      printf("\ndetailed API Error:\n");

      if (response.api_error->type != NULL) {
        printf("  type: %s\n", response.api_error->type);
      }

      if (response.api_error->message != NULL) {
        printf("  message: %s\n", response.api_error->message);
      }

      if (response.api_error->code != NULL) {
        printf("  code: %s\n", response.api_error->code);
      }

      if (response.api_error->param != NULL) {
        printf("  parameter: %s\n", response.api_error->param);
      }
    }

    printf("=============\n");
    goto cleanup;
  }

  printf("\n=== RESPONSE ===\n");
  printf("%s\n", response.content);
  printf("================\n\n");

  printf("Usage Statistics:\n");
  printf("-----------------\n");
  printf("Prompt tokens:     %d\n", response.prompt_tokens);
  printf("Completion tokens: %d\n", response.completion_tokens);
  printf("Total tokens:      %d\n\n", response.total_tokens);

  printf("Metadata:\n");
  printf("---------\n");
  if (response.id != NULL) {
    printf("Response ID: %s\n", response.id);
  }
  if (response.model != NULL) {
    printf("Model used:  %s\n", response.model);
  }
  printf("HTTP code:   %ld\n", response.http_code);
  printf("Error code:  %d (%s)\n", response.error_code,
         mistral_error_string(response.error_code));

  ret = 0;

  return ret;

cleanup:
  mistral_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();

  return ret;
}
