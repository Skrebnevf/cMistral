#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  mistral_config_t *config = NULL;
  mistral_embeddings_response_t response = {0};
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

  printf("Mistral Embeddings Example\n");
  printf("==========================\n\n");

  config = mistral_config_create(api_key);
  if (config == NULL) {
    fprintf(stderr, "failed to create config\n");
    goto cleanup;
  }

  free(config->model);
  config->model = strdup("mistral-embed");
  config->debug_mode = 1;

  printf("Configuration:\n");
  printf("  Model: %s\n", config->model);
  printf("  Debug mode: %s\n\n", config->debug_mode ? "enabled" : "disabled");

  mistral_embeddings_t embeddings = {
    .input = "Hello, world!"
  };

  printf("sending request...\n\n");

  if (mistral_embeddings(config, &embeddings, 1, &response) != 0) {
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
  printf("ID: %s\n", response.id ? response.id : "N/A");
  printf("Model: %s\n", response.model ? response.model : "N/A");
  printf("Object: %s\n", response.object ? response.object : "N/A");
  
  if (response.data != NULL) {
    printf("\nEmbedding:\n");
    printf("-----------\n");
    printf("Index: %d\n", response.data[0].index);
    if (response.data[0].object != NULL) {
      printf("Object: %s\n", response.data[0].object);
    }
    if (response.data[0].embending != NULL) {
      printf("First 5 values: [%.6f, %.6f, %.6f, %.6f, %.6f]\n",
             response.data[0].embending[0], response.data[0].embending[1],
             response.data[0].embending[2], response.data[0].embending[3],
             response.data[0].embending[4]);
    }
  }

  if (response.usage != NULL) {
    printf("\nUsage Statistics:\n");
    printf("-----------------\n");
    printf("Prompt tokens:     %d\n", response.usage->prompt_tokens);
    printf("Completion tokens: %d\n", response.usage->completion_tokens);
    printf("Total tokens:      %d\n", response.usage->total_tokens);
  }

  printf("\nMetadata:\n");
  printf("---------\n");
  printf("HTTP code:   %ld\n", response.http_code);
  printf("Error code:  %d (%s)\n", response.error_code,
         mistral_error_string(response.error_code));

  ret = 0;

cleanup:
  mistral_embeddings_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();

  return ret;
}