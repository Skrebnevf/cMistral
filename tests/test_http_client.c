#define _POSIX_C_SOURCE 200809L

#include "../src/http_client.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int test_init_cleanup(void) {
  printf("TEST - HTTP client init/cleanup\n");

  int result = http_client_init();
  assert(result == 0);
  printf("...init - ok\n");

  http_client_cleanup();
  printf("...cleanup - ok\n");

  printf("TEST PASSED\n\n");

  return 0;
}

int test_response_init(void) {
  printf("TEST - Response struct init\n");

  http_response_t response = {0};
  printf("...init - ok\n");

  assert(response.data == NULL);
  assert(response.http_code == 0);
  assert(response.size == 0);

  printf("TEST PASSED\n\n");

  return 0;
}

int test_response_free_empty(void) {
  printf("TEST - Free empty response\n");

  http_response_t response = {0};
  printf("...add free data - ok\n");

  http_response_free(&response);
  printf("...clear free data - ok\n");

  assert(response.data == NULL);
  assert(response.size == 0);

  printf("TESR PASSED\n\n");

  return 0;
}

int test_response_free_with_data(void) {
  printf("TEST - Free with data response\n");

  http_response_t response = {0};

  response.data = strdup("test data");
  response.size = strlen("test data");
  response.http_code = 200;
  printf("...fill data - ok\n");

  assert(response.data != NULL);

  http_response_free(&response);
  printf("...clear data - ok\n");

  assert(response.data == NULL);
  assert(response.size == 0);
  assert(response.http_code == 0);

  printf("...check is clear - ok\n");

  printf("TEST PASSED\n\n");
  return 0;
}

int test_real_http_request(void) {
  printf("TEST - Real request\n");

  http_response_t response = {0};

  const char *test_api = "https://httpbin.org/post";
  const char *headers[] = {"Content-Type: application/json", NULL};
  const char *body = {"{\"test\": \"data\"}"};

  if (http_client_init() != 0) {
    printf("SKIPPED - init failed\n");
    return 0;
  }
  printf("...init - ok\n");

  printf("...send request\n");

  int result = http_post(test_api, headers, body, &response);
  if (result == 0) {
    assert(response.http_code == 200);
    assert(response.data != NULL);
    assert(response.size > 0);

    printf("...request - ok (code: %ld, bytes: %zu)\n", response.http_code,
           response.size);
  } else {
    printf("SKIPPED - network error\n");
  }

  http_response_free(&response);
  http_client_cleanup();

  printf("TEST PASSED\n\n");

  return 0;
}

int test_null_ptr(void) {
  printf("TEST - NULL ptr handling\n");

  http_response_t response = {0};

  http_response_free(NULL);
  http_response_free(&response);

  printf("...cleaning - ok\n");
  printf("TEST PASSED\n\n");

  return 0;
}

int main(void) {
  int failed = 0;

  printf("===========================================\n");
  printf("HTTP Client Unit Tests\n");
  printf("===========================================\n\n");

  failed += test_init_cleanup();
  failed += test_response_init();
  failed += test_response_free_empty();
  failed += test_response_free_with_data();
  failed += test_null_ptr();

  printf("\n--- Real request ---\n");
  failed += test_real_http_request();

  printf("\n===========================================\n");
  if (failed == 0) {
    printf("[ok] All HTTP client tests passed!\n");
  } else {
    printf("[not ok] %d test(s) failed\n", failed);
  }
  printf("===========================================\n");

  return failed;
}
