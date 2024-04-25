#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <stdbool.h>

typedef struct // Stores curl Response
{
  char *string;
  size_t size;
} Response;
typedef struct
{
  URLQueueNode *head, *tail;
  pthread_mutex_t lock;
} URLQueue;

typedef struct URLQueueNode
{
  char *url;
  struct URLQueueNode *next;
  int depth;
} URLQueueNode;

typedef struct
{
  URLQueueNode **list;
  pthread_mutex_t lock;
} URL_list;

typedef struct
{
  FILE *file;
  URLQueue *queue;
  URL_list *vlist;
} arguments;

int depth_limit;

int hashing(char *url);

void initQueue(URLQueue *queue);
URLQueueNode *dequeue(URLQueue *queue);
void enqueue(URLQueueNode *newNode, URLQueue *queue);

URLQueueNode *createURLQueueNode(char *url);
URLQueue *createURLQueue();
void delete_node(URLQueueNode *node);

void extract_url(char *html, URLQueue *queue, URLQueueNode *parent);
bool url_filter(URLQueueNode *node);
int get_html(URLQueue *queue, FILE *file, URL_list *vlist);
size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void logURL(FILE *file, const char *url);

URL_list *create_visitor_list(int size);
void add_list_node(char *url, URL_list *vlist);
void delete_list(URL_list *vlist);

int hashing(char *url)
{
  return strlen(url) % 100;
}

void logURL(FILE *file, const char *url)
{
  fprintf(file, "%s\n", url); // write the URL to the file
}

URLQueueNode *createURLQueueNode(char *url)
{
  URLQueueNode *node = malloc(sizeof(URLQueueNode));
  node->url = malloc((strlen(url) + 1) * sizeof(char));
  strcpy(node->url, url);
  node->next = NULL;
  node->depth = 0;
  return node;
}

URLQueue *createURLQueue()
{
  URLQueue *queue = malloc(sizeof(URLQueue));
  queue->head = queue->tail = NULL;
  pthread_mutex_init(&queue->lock, NULL);

  return queue;
}

void enqueue(URLQueueNode *newNode, URLQueue *queue)
{
  pthread_mutex_lock(&queue->lock);
  if (queue->tail != NULL)
  {
    queue->tail->next = newNode;
  }
  else
  {
    queue->head = newNode;
  }
  queue->tail = newNode;
  pthread_mutex_unlock(&queue->lock);
}

// Remove a URL from the queue.
URLQueueNode *dequeue(URLQueue *queue)
{
  pthread_mutex_lock(&queue->lock);
  if (queue->head == NULL)
  {
    pthread_mutex_unlock(&queue->lock);
    return NULL;
  }
  URLQueueNode *temp = queue->head;
  queue->head = queue->head->next;
  if (queue->head == NULL)
  {
    queue->tail = NULL;
  }

  pthread_mutex_unlock(&queue->lock);
  return temp;
}

void delete_queue(URLQueue *queue)
{
  URLQueueNode *node = dequeue(queue);
  while (node != NULL)
  {
    delete_node(node);
    node = dequeue(queue);
  }
}

URL_list *create_visitor_list(int size)
{
  URL_list *vlist = malloc(sizeof(URL_list));
  vlist->list = malloc(sizeof(URLQueueNode *) * size);

  pthread_mutex_init(&vlist->lock, NULL);

  for (int i = 0; i < size; i++)
  {
    vlist->list[i] = NULL;
  }
  return vlist;
}

void delete_list(URL_list *vlist)
{
  URLQueueNode *current;
  URLQueueNode *next;
  for (int i = 0; i < 100; i++)
  {
    if (vlist->list[i] != NULL)
    {
      current = vlist->list[i];
      while (current != NULL)
      {
        next = current->next;
        delete_node(current);
        current = next;
      }
    }
  }
  free(vlist->list);
  free(vlist);
}

void add_list_node(char *url, URL_list *vlist)
{
  int index = hashing(url);
  URLQueueNode *node = createURLQueueNode(url);
  URLQueueNode *N;

  pthread_mutex_lock(&vlist->lock);

  if (vlist->list[index] == NULL)
  {
    vlist->list[index] = node;
  }
  else
  {
    N = vlist->list[index];
    while (N->next != NULL)
    {
      N = N->next;
    }
    N->next = node;
  }
  pthread_mutex_unlock(&vlist->lock);
}

void delete_node(URLQueueNode *node)
{
  free(node->url);
  free(node);
}

bool check_visited(char *url, URL_list *vlist)
{
  pthread_mutex_lock(&vlist->lock);
  int index = hashing(url);
  URLQueueNode *N = vlist->list[index];
  while (N != NULL)
  {
    if (strcmp(url, N->url) == 0)
    {
      pthread_mutex_unlock(&vlist->lock);
      return true;
    }
    N = N->next;
  }
  pthread_mutex_unlock(&vlist->lock);
  return false;
}

bool url_filter(URLQueueNode *node) // implement crawler policies here
{
  if (node->depth < depth_limit)
    return true; // URL is valid
  else
    return false; // URL depth exceeds limit
}

void extract_url(char *html, URLQueue *queue, URLQueueNode *parent) // pass parent
{
  char *sub;
  int i;
  int j;
  if (html == NULL)
  {
    return;
  }
  if (strlen(html) < 5)
  {
    return;
  }
  sub = strstr(html, "href=\"http");
  if (sub == NULL)
  {
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
    newNode->depth = parent->depth + 1;
    if (!url_filter(newNode))
    {
      delete_node(newNode);
      free(furl);
      return;
    }
    enqueue(newNode, queue);

    free(furl);

    html = html + (sizeof(char) * i);
    extract_url(html, queue, parent);
  }
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

int get_html(URLQueue *queue, FILE *file, URL_list *vlist) // fetches url in response struct
{
  //-1 to end;  0 for failed get request; 1 for continue
  URLQueueNode *node = dequeue(queue);
  char msg[1100] = "Request Failed: ";
  if (node == NULL)
  {
    return -1;
  }

  char *url = node->url;

  if (check_visited(node->url, vlist))
  {
    delete_node(node);
    return 1;
  }
  CURL *curl;              // declaring handle
  CURLcode result;         // http status code
  curl = curl_easy_init(); // initialising handle
  Response response;
  response.string = malloc(1);
  response.size = 0;

  if (curl == NULL)
  {
    strcat(msg, node->url);
    strcat(msg, "\n");
    printf("REQUEST FAILED\n");
    logURL(file, msg);

    free(response.string);
    curl_easy_cleanup(curl);
    delete_node(node);
    return 0;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  result = curl_easy_perform(curl);

  if (result != CURLE_OK)
  {
    printf("Error %s\n", curl_easy_strerror(result));
    strcat(msg, node->url);
    strcat(msg, curl_easy_strerror(result));
    strcat(msg, "\n");
    logURL(file, msg);

    free(response.string);
    curl_easy_cleanup(curl);
    delete_node(node);
    return 0;
  }
  if (response.size < 5)
  {
    free(response.string);
    curl_easy_cleanup(curl);
    delete_node(node);
    return 0;
  }

  printf("Depth :  %d\t%s \n", node->depth, node->url);
  logURL(file, node->url);
  add_list_node(node->url, vlist);
  curl_easy_cleanup(curl);
  extract_url(response.string, queue, node);
  free(response.string);
  delete_node(node);
  return 1;
}

void *engine(void *arg)
{
  arguments *a = (arguments *)arg;
  int status_code = 1;
  while (status_code != -1)
  {
    status_code = get_html(a->queue, a->file, a->vlist);
  }
  return NULL;
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
  depth_limit = 3;

  URLQueueNode *firstNode = createURLQueueNode(url);
  firstNode->depth = 0;
  enqueue(firstNode, queue);
  FILE *file = fopen("log.txt", "w+");
  if (file == NULL)
  {
    printf("Error opening file!\n");
    return EXIT_FAILURE;
  }

  URL_list *vlist = create_visitor_list(100);

  arguments ar;
  arguments *a = &ar;
  a->file = file;
  a->queue = queue;
  a->vlist = vlist;

  pthread_t threads[10];
  for (int i = 0; i < 10; i++)
  {
    pthread_create(&threads[i], NULL, engine, (void *)a);
  }

  for (int i = 0; i < 10; i++)
  {
    pthread_join(threads[i], NULL);
  }

  delete_queue(queue);
  delete_list(vlist);
  fclose(file); // close the file
  free(queue);

  return EXIT_SUCCESS;
}
