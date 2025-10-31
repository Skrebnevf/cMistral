# cMistral

C library for working with Mistral AI API - a simple and efficient wrapper for integrating language models into your C applications.

## Features

- **Chat Completions** - conversational sessions with Mistral models
- **Fill-in-the-Middle (FIM)** - code completion for Codestral models
- **Embeddings** - get vector representations of text
- **Debug Mode** - verbose request logging

## Installation

### Requirements

- GCC with C99 support
- libcurl
- libcjson

### Build

```bash
# Clone repository
git clone https://github.com/your-username/cMistral.git
cd cMistral

# Build library
make all

# Build examples
make example

# Run tests
make test
```

## Quick Start

### 1. Set API Key

```bash
export MISTRAL_API_KEY='your-api-key-here'
```

### 2. Chat Completions

```c
#include "mistral.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  // Initialize library
  if (mistral_init() != 0) {
    fprintf(stderr, "Failed to initialize Mistral library\n");
    return 1;
  }

  // Create configuration
  const char *api_key = getenv("MISTRAL_API_KEY");
  mistral_config_t *config = mistral_config_create(api_key);
  config->temperature = 0.7;
  config->max_tokens = 200;
  config->model = strdup("mistral-small-latest");

  // Prepare messages
  mistral_message_t messages[] = {
    {.role = "system", .content = "You are a helpful assistant."},
    {.role = "user", .content = "What is C programming?"}
  };

  // Send request
  mistral_response_t response = {0};
  if (mistral_chat_completions(config, messages, 2, &response) == 0) {
    printf("Response: %s\n", response.content);
    printf("Tokens used: %d\n", response.total_tokens);
  } else {
    printf("Error: %s\n", response.error_message);
  }

  // Cleanup
  mistral_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();
  return 0;
}
```

### 3. Fill-in-the-Middle (code completion)

```c
#include "mistral.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  mistral_init();
  
  const char *api_key = getenv("MISTRAL_API_KEY");
  mistral_config_t *config = mistral_config_create(api_key);
  config->model = strdup("codestral-2405");
  config->temperature = 0.3;

  // Prepare FIM request
  mistral_fim_t fim = {
    .prompt = "func calculate_sum(a, b int) int {",
    .suffix = "return result"
  };

  mistral_response_t response = {0};
  if (mistral_fim_completions(config, &fim, &response) == 0) {
    printf("Generated code:\n%s\n", response.content);
  }

  mistral_response_free(&response);
  mistral_config_free(config);
  mistral_cleanup();
  return 0;
}
```

### 4. Embeddings

```c
#include "mistral.h"
#include <stdio.h>

int main(void) {
  mistral_init();
  
  const char *api_key = getenv("MISTRAL_API_KEY");
  mistral_config_t *config = mistral_config_create(api_key);

  mistral_embeddings_t embeddings = {
    .input = "Hello, world!"
  };

  mistral_embeddings_response_t response = {0};
  if (mistral_embeddings(config, &embeddings, 1, &response) == 0) {
    printf("Embedding generated successfully\n");
    printf("Model: %s\n", response.model);
    printf("Dimensions: %d\n", response.usage->prompt_tokens);
  }

  mistral_api_error_free(response.api_error);
  mistral_config_free(config);
  mistral_cleanup();
  return 0;
}
```

## API Reference

### Configuration

```c
typedef struct {
  char *api_key;        // Mistral API key
  char *model;          // Model (mistral-small-latest, codestral-2405, etc.)
  double temperature;    // Temperature (0.0-1.0)
  int max_tokens;       // Maximum number of tokens
  int max_retries;      // Number of retry attempts
  int retry_delay_ms;   // Delay between retries
  int timeout_sec;      // Request timeout
  int debug_mode;       // Debug mode
} mistral_config_t;
```

### Core Functions

- `mistral_init()` - initialize library
- `mistral_cleanup()` - cleanup resources
- `mistral_config_create(api_key)` - create configuration
- `mistral_config_free(config)` - free configuration
- `mistral_chat_completions()` - send chat request
- `mistral_fim_completions()` - send FIM request
- `mistral_embeddings()` - get embeddings
- `mistral_response_free()` - free chat/FIM response
- `mistral_embeddings_response_free()` - free embeddings response

### Error Handling

```c
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
```

## Building and Usage

### Compiling with the library

```bash
# Build library
make all

# Compile your application
gcc -Iinclude your_app.c -L. -lmistral -lcurl -lcjson -lm -o your_app
```

### Running examples

```bash
# Build examples
make example

# Run chat example
./chat_example

# Run FIM example  
./fim_example

# Run embeddings example
./embeddings_example
```

## Supported Models

- **Chat models**: `mistral-small-latest`, `mistral-medium-latest`, `mistral-large-latest`
- **Code models**: `codestral-2405`, `codestral-latest`
- **Embedding models**: `mistral-embed`

## License

MIT License

## Additional Resources

- [Mistral AI Documentation](https://docs.mistral.ai/)
- [API Reference](https://docs.mistral.ai/api/)
- [Models](https://docs.mistral.ai/models/)
