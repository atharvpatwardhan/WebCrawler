#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <stdbool.h>

// Okay
// Define a structure for queue elements.
typedef struct URLQueueNode
{
  char *url;
  struct URLQueueNode *next;
  struct URLQueueNode *parent;
  int depth;
} URLQueueNode;

// Define a structure for a thread-safe queue.
typedef struct
{
  URLQueueNode *head, *tail;
  pthread_mutex_t lock;
} URLQueue;

typedef struct // Declaring Response struct
{
  char *string;
  size_t size;
} Response;

char *seed_domain;
int depth_limit;

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void initQueue(URLQueue *queue);
char *dequeue(URLQueue *queue);
void enqueue(URLQueue *queue, const char *url);

// Placeholder for the function to fetch and process a URL.
void *fetch_url(void *url);

URLQueueNode *creteURLQueueNode(char *url)
{
  URLQueueNode *node = malloc(sizeof(URLQueueNode));
  node->url = malloc(strlen(url) + 1);
  strcpy(node->url, url);
  node->next = NULL;
  node->parent = NULL;
  node->depth = 0;
  return node;
}

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata)
{
  size_t real_size = size * nmemb;
  Response *response = (Response *)userdata;
  char *ptr = realloc(response->string, response->size + real_size + 1);

  if (ptr == NULL)
  {
    return 0;
  }
  response->string = ptr;
  memcpy(&(response->string[response->size]), data, real_size);
  response->size += real_size;
  response->string[response->size] = '\0';
  return real_size;
}

void extract_url(char *html)
{
  char *sub;
  int i;
  int j;
  sub = strstr(html, "href=\"http");
  if (sub == NULL)
  {
    // printf("Boo hoo not working\n"); // implement while or remove entirely
    return;
  }
  else
  {
    i = 0;
    while (i >= 0)
    {
      if (sub[i + 6] == '"') // starts searching from http
      {
        break;
      }
      i++;
    }
    char *furl = malloc(sizeof(char) * (i + 1));
    j = 0;
    while (j < i)
    {
      furl[j] = sub[j + 6];
      j++;
    }
    furl[i] = '\0';
    printf("String: %s\n", furl);
    free(furl);
    html = html + (sizeof(char) * i);
    extract_url(html);
  }
}
void *fetchurl(char *url) // fetches url in response struct
{
  CURL *curl;              // declaring handle
  CURLcode result;         // http status code
  curl = curl_easy_init(); // initialising handle
  Response response;
  response.string = malloc(1);
  response.size = 0;

  if (curl == NULL)
  {
    printf("REQUEST FAILED\n");
    free(response.string);
    curl_easy_cleanup(curl);
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  result = curl_easy_perform(curl);

  if (result != CURLE_OK)
  {
    printf("Error %s\n", curl_easy_strerror(result));
    free(response.string);
    curl_easy_cleanup(curl);
    return NULL;
  }

  // printf("%s\n", response.string);
  curl_easy_cleanup(curl);
  extract_url(response.string);
  free(response.string);
  return NULL;
}
int hashing(char *url)
{
  return strlen(url) % 100;
}
void logURL(const char *url)
{
  FILE *file = fopen("log.txt", "a+"); // "a" mode appends to the file if it exists, creates it if not
  if (file == NULL)
  {
    printf("Error opening file!\n");
    return;
  }

  fprintf(file, "%s\n", url); // write the URL to the file

  fclose(file); // close the file
}
bool url_filter(URLQueueNode *node)
{
  if (strstr(node->url, seed_domain) != NULL)
  {
    // Check if the depth is less than depth_limit
    if (node->depth < depth_limit)
    {
      return true; // URL is valid
    }
    else
    {
      return false; // URL depth exceeds limit
    }
  }
  else
  {
    return false; // URL does not belong to seed domain
  }
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("not enough args\n");
    return EXIT_SUCCESS;
  }
  char *url = argv[1]; // setting url as the first argument
  seed_domain = argv[1];
  depth_limit = 5;
  fetchurl(argv[1]); // calling fetchurl on first argument
  return EXIT_SUCCESS;
}
