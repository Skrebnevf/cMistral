#ifndef MISTRAL_H
#define MISTRAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define MISTRAL_VERSION 0.1.1

#define MISTRAL_BASE_API "https://api.mistral.ai/v1"

typedef struct{
  char *message;
  char *type;
  char *param;
  char *code;
} mistral_api_error_t;
/*
* Client config
*/
typedef struct {
  char *api_key;
  char *model;
  double temperature;
  int max_tokens;
  int max_retries;
  int retry_delay_ms;
  int timeout_sec;
  int debug_mode;
} mistral_config_t;

/*
* Message struct
*/
typedef struct {
  char *role;
  char *content;
} mistral_message_t;

/*
* FIM struct
*/
typedef struct {
  char *prompt;
  char *suffix;
} mistral_fim_t;

/*Errors code*/
typedef enum {
  MISTRAL_OK = 0,
  MISTRAL_ERR_INVALID_PARAM,
  MISTRAL_ERR_NETWORK,
  MISTRAL_ERR_AUTH,
  MISTRAL_ERR_RATE_LIMIT,
  MISTRAL_ERR_SERVER,
  MISTRAL_ERR_PARSE,
  MISTRAL_ERR_TIMEOUT,
  MISTRAL_ERR_MEM
} mistral_error_code_t;

/*
* Response from chat/completions
*/
typedef struct {
  char *id;
  char *model;
  char *content;
  int prompt_tokens;
  int completion_tokens;
  int total_tokens;
  char *error_message;
  mistral_error_code_t error_code;
  long http_code;
  mistral_api_error_t *api_error;
} mistral_response_t ;

/*
* Call first
* Return 0 if ok, -1 if error 
*/
int mistral_init(void);

/*
* Call last
*/
void mistral_cleanup(void);

/*
* Create mistral config
*/
mistral_config_t *mistral_config_create(const char *api_key);

/*
* Free config
*/
void mistral_config_free(mistral_config_t *config);

/*
* Config validator
* Return 0 or -1 if error
*/
int mistral_config_validate(const mistral_config_t *config);

/*
* Send request to chat/completions
* config: client config
* messages: array of messages
* message_count: qty massages in array
* response ptr to struct for write response
*/
int mistral_chat_completions(
  const mistral_config_t *config,
  const mistral_message_t *messages,
  size_t message_count,
  mistral_response_t *response
);

/*
* Send request to fim /fim/completions
* config: client config
* fim: object with suffix and prompt
* response ptr to struct for write response
*/
int mistral_fim_completions(
  const mistral_config_t *config,
  const mistral_fim_t *fim,
  mistral_response_t *response
);

/*
* Clear request
*/
void mistral_response_free(mistral_response_t *response);

/*
* Get text error
*/
const char *mistral_error_string(mistral_error_code_t code);

/*
* On or off debug mode
*/
void mistral_set_debug(int enabled);

/*
* Struct cleanup
*/
void mistral_api_error_free(mistral_api_error_t *error);

#ifdef __cplusplus
}
#endif

#endif
