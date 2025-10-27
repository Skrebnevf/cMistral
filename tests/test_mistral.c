#define _POSIX_C_SOURCE 200809L

#include "../include/mistral.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_mistral_init_cleanup(void) {
  printf("TEST - Init cleanup mistral\n");

  int result = mistral_init();
  assert(result == 0);
  printf("...init - ok\n");

  mistral_cleanup();
  printf("...clenup - ok\n");

  printf("TEST PASSED\n\n");

  return 0;
}

int test_config_create_valid(void) {
  printf("TEST - Init cleanup mistral\n");

  const char *api_key = "test-key";

  mistral_config_t *config = mistral_config_create(api_key);

  printf("...init - ok\n");

  assert(config != NULL);
  assert(config->api_key != NULL);
  assert(strcmp(config->api_key, api_key) == 0);
  assert(config->model != NULL);
  assert(config->max_tokens == 1024);
  assert(config->temperature == 0.7);

  mistral_config_free(config);
  printf("...cleanup - ok\n");

  printf("TEST PASSED\n\n");
  return 0;
}

int test_config_create_null_ptr(void) {
  printf("TEST - Init config null ptr\n");

  mistral_config_t *config = mistral_config_create(NULL);

  assert(config == NULL);

  printf("...init with null - ok\n");

  printf("TEST PASSED\n\n");

  return 0;
}

int test_config_free_null(void) {
  printf("TEST - Free null config\n");

  mistral_config_free(NULL);

  printf("...after null free - ok\n");

  printf("TEST PASSED\n\n");

  return 0;
}

int test_config_modify(void) {
  printf("TEST - Modify config parametrs\n");

  const char *api_key = "test-key";
  const char *model = "mistral-small-latest";

  mistral_config_t *config = mistral_config_create(api_key);
  assert(config != NULL);
  printf("...init - ok\n");

  config->temperature = 0.1;
  config->max_tokens = 512;

  free(config->model);
  config->model = strdup(model);
  printf("...change model - ok\n");
  printf("Model: %s\n", config->model);

  assert(config->temperature == 0.1);
  assert(config->max_tokens == 512);
  assert(!strcmp(config->model, model));

  mistral_config_free(config);
  printf("...cleanup - ok\n");

  printf("TEST PASSED\n\n");
  return 0;
}

int test_response_init(void) {
  printf("TEST - Response init\n");

  mistral_response_t response = {0};
  printf("...init - ok\n");

  assert(response.id == NULL);
  assert(response.model == NULL);
  assert(response.content == NULL);
  assert(response.prompt_tokens == 0);
  assert(response.completion_tokens == 0);
  assert(response.total_tokens == 0);
  assert(response.error_message == NULL);

  printf("TEST PASSED\n\n");

  return 0;
}

int test_response_free_empty(void) {
  printf("TEST - Free empty response\n");

  mistral_response_t response = {0};
  printf("...init - ok\n");

  mistral_response_free(&response);
  assert(response.id == NULL);
  assert(response.content == NULL);
  printf("...free - ok\n");

  printf("TEST PASSED\n\n");
  return 0;
}

int test_response_with_free_data(void) {
  printf("TEST - Free response with data\n");

  mistral_response_t response = {0};

  response.id = strdup("cmpl-123");
  response.model = strdup("mistral-tiny");
  response.content = strdup("Test response");
  response.prompt_tokens = 10;
  response.completion_tokens = 20;
  response.total_tokens = 30;

  printf("...init - ok\n");
  assert(response.content != NULL);

  mistral_response_free(&response);

  assert(response.id == NULL);
  assert(response.model == NULL);
  assert(response.content == NULL);
  assert(response.prompt_tokens == 0);
  assert(response.completion_tokens == 0);
  assert(response.total_tokens == 0);

  printf("...cleanup - ok\n");

  printf("TEST PASSED\n\n");

  return 0;
}

int test_real_api_request(void) {
  printf("TEST - Real API request\n");

  const char *api_key = getenv("MISTRAL_API_KEY");

  if (api_key == NULL) {
    printf("SKIPPED - MISTRAL_API_KEY not set\n");
    return 0;
  }

  printf("...get env - ok\n");

  if (mistral_init() != 0) {
    printf("SKIPPED - Mistral init failed\n");
    return 0;
  }

  mistral_config_t *config = mistral_config_create(api_key);
  assert(config != NULL);

  config->max_tokens = 50;

  mistral_message_t msgs[] = {
      {.role = "user", .content = "Say 'hello' in one word"}};

  mistral_response_t response = {0};

  printf("...send request\n");
  int result = mistral_chat_completions(config, msgs, 1, &response);

  if (result == 0) {
    assert(response.content != NULL);
    assert(response.total_tokens > 0);
    assert(response.error_message == NULL);

    printf("...request - ok, tokens: %d\n", response.total_tokens);
  } else {
    if (response.error_message != NULL) {
      printf("...TEST FAILED - error message from response: %s\n",
             response.error_message);
    } else {
      printf("TEST FAILED - unknown error\n");
    }
    mistral_response_free(&response);
    mistral_config_free(config);
    mistral_cleanup();

    return 1;
  }

  mistral_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();

  printf("TEST PASSED\n\n");
  return 0;
}

int test_chat_completions_invalid_params(void) {
  printf("TEST - Chat completions with invalid params\n");

  mistral_config_t *config = mistral_config_create("test");
  mistral_message_t messages[] = {{.role = "user", .content = "test"}};
  mistral_response_t response = {0};

  printf("...init - ok\n");

  int result = mistral_chat_completions(NULL, messages, 1, &response);
  assert(result != 0);

  result = mistral_chat_completions(config, NULL, 1, &response);
  assert(result != 0);

  result = mistral_chat_completions(config, messages, 0, &response);
  assert(result != 0);

  result = mistral_chat_completions(config, messages, 1, NULL);
  assert(result != 0);

  mistral_config_free(config);

  printf("TEST PASSED\n\n");
  return 0;
}

int test_multiple_messages(void) {
  printf("TEST - Multiple messages in conversation\n");

  mistral_config_t *config = mistral_config_create("test");
  assert(config != NULL);

  printf("...init - ok\n");

  mistral_message_t messages[] = {
      {.role = "system", .content = "You are a helpful assistant"},
      {.role = "user", .content = "Hello"},
      {.role = "assistant", .content = "Hi there!"},
      {.role = "user", .content = "How are you?"}};

  assert(messages[0].role != NULL);
  assert(messages[3].content != NULL);

  mistral_config_free(config);

  printf("...cleanup - ok\n");

  printf("TEST PASSED\n\n");
  return 0;
}

int main(void) {
  int failed = 0;

  printf("===========================================\n");
  printf("Mistral API Unit Tests\n");
  printf("===========================================\n\n");

  failed += test_mistral_init_cleanup();
  failed += test_config_create_valid();
  failed += test_config_create_null_ptr();
  failed += test_config_free_null();
  failed += test_config_modify();
  failed += test_response_init();
  failed += test_response_free_empty();
  failed += test_response_with_free_data();
  failed += test_chat_completions_invalid_params();
  failed += test_multiple_messages();

  printf("\n--- Network-dependent tests ---\n");

  failed += test_real_api_request();

  printf("\n===========================================\n");
  if (failed == 0) {
    printf("[+] All Mistral API tests passed!\n");
  } else {
    printf("[-] %d test(s) failed\n", failed);
  }
  printf("===========================================\n");

  return failed;
}
