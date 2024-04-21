#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

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

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void initQueue(URLQueue *queue);
char *dequeue(URLQueue *queue);
void enqueue(URLQueueNode* newNode,URLQueue *queue);

// Placeholder for the function to fetch and process a URL.
void *fetch_url(void *url);

URLQueueNode* createURLQueueNode(char* url)
{
  URLQueueNode* node = malloc(sizeof(URLQueueNode));
  node->url = malloc((strlen(url)+1)*sizeof(char));
  strcpy(node->url,url);
  node->next = NULL;
  node->parent = NULL;
  node->depth = 0;
  return node;
}

URLQueue* createURLQueue()
{
  URLQueue* queue = malloc(sizeof(URLQueue));
  queue->head = queue->tail = NULL;
  pthread_mutex_init(&queue->lock, NULL);

  return queue;

}


void enqueue(URLQueueNode *newNode,URLQueue *queue) {
pthread_mutex_lock(&queue->lock);
if (queue->tail) {
queue->tail->next = newNode;
} else {
queue->head = newNode;
}
queue->tail = newNode;
pthread_mutex_unlock(&queue->lock);
}


// Remove a URL from the queue.
char *dequeue(URLQueue *queue) {
pthread_mutex_lock(&queue->lock);
if (queue->head == NULL) {
pthread_mutex_unlock(&queue->lock);
return NULL;
}
URLQueueNode *temp = queue->head;
char *url = temp->url;
queue->head = queue->head->next;
if (queue->head == NULL) {
queue->tail = NULL;
}
// printf("\nDequeue : %s",url);
free(temp);
pthread_mutex_unlock(&queue->lock);
return url;
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

void extract_url(char *html, URLQueue *queue)
{
  char *sub;
  int i;
  int j;
  sub = strstr(html, "href=\"http");
  if (sub == NULL)
  {
    //printf("Boo hoo not working\n"); // implement while or remove entirely
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

    URLQueueNode *newNode = createURLQueueNode(furl);
    enqueue(newNode,queue);

    free(furl);

    html = html + (sizeof(char) * i);
    extract_url(html,queue);
  }
}

void *fetchurl(URLQueue *queue) // fetches url in response struct
{
  char *url = dequeue(queue);
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
  extract_url(response.string, queue);
  free(response.string);
  free(url);
  return NULL;
}
int hashing (char* url)
{
  return strlen(url)%100;
}


int main(int argc, char **argv)
{

  URLQueue *queue = createURLQueue();

  if (argc < 2)
  {
    printf("not enough args\n");
    return EXIT_SUCCESS;
  }

  char *url = argv[1]; // setting url as the first argument
  URLQueueNode *firstNode = createURLQueueNode(url);
  enqueue(firstNode,queue);
  // printf("%s\n", url);

  fetchurl(queue); // calling fetchurl on first argument

  while(queue->head != NULL){
    char *hhtps = dequeue(queue); 
    printf("Main : %s\n",hhtps);
    free(hhtps);
  }

  char *uurl = dequeue(queue);
  free(uurl);
  free(queue);
  return EXIT_SUCCESS;
}
