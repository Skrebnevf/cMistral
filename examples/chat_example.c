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

  free(config->model);
  config->model = strdup("mistral-small-latest");

  printf("using model: %s\n", config->model);
  printf("temperature: %.2f\n", config->temperature);
  printf("max tokens: %d\n\n", config->max_tokens);

  mistral_message_t messages[] = {
      {.role = "system",
       .content = "you are helpful assistant for C developer"},
      {.role = "user", .content = "Tell me about malloc method"}};

  printf("sending request...\n\n");

  if (mistral_chat_completions(config, messages, 2, &response) != 0) {
    fprintf(stderr, "error: %s",
            response.error_message ? response.error_message : "unknown error");
    goto cleanup;
  }

  printf("response:\n");
  printf("---------\n");
  printf("%s\n\n", response.content);

  printf("usage statistics:\n");
  printf("-----------------\n");
  printf("prompt tokens:     %d\n", response.promt_tokens);
  printf("completion tokens: %d\n", response.completion_tokens);
  printf("total tokens:      %d\n", response.total_tokens);
  printf("\nresponse ID: %s\n", response.id ? response.id : "N/A");
  printf("model used:  %s\n", response.model ? response.model : "N/A");

  ret = 0;

cleanup:
  mistral_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();

  return ret;
}
