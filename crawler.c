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

int depth_limit;

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void initQueue(URLQueue *queue);
URLQueueNode *dequeue(URLQueue *queue);
void enqueue(URLQueueNode *newNode, URLQueue *queue);
URLQueueNode *createURLQueueNode(char *url);
URLQueue *createURLQueue();
void extract_url(char *html, URLQueue *queue, URLQueueNode* parent);
int hashing(char *url);
void logURL(FILE *file, const char *url);
bool url_filter(URLQueueNode *node);
void *fetch_url(void *url,FILE* file, URLQueueNode** list);

URLQueueNode** create_visitor_list(int size);
void add_list_node(char *url,URLQueueNode** list);
void delete_list(URLQueueNode** list);


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
  if (queue->tail!=NULL)
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
  //printf("deque: try temp url %s\n",temp->url);
  queue->head = queue->head->next;
  if (queue->head == NULL)
  {
    queue->tail = NULL;
  }

  pthread_mutex_unlock(&queue->lock);
  return temp;
}
void delete_node(URLQueueNode* node)
{
  free(node->url);
  free(node);
}
void delete_queue(URLQueue* queue)
{
  URLQueueNode *node = dequeue(queue);
  while(node!=NULL)
    {
      delete_node(node);
      node=dequeue(queue);
    }
}





URLQueueNode** create_visitor_list(int size)
{
  URLQueueNode** vlist = malloc(sizeof(URLQueueNode*)*size);
  for(int i=0;i<size;i++)
    {
      vlist[i] = NULL;
    }
  return vlist;
}

bool check_visited(char *url, URLQueueNode** list)
{
  int index = hashing(url);
  URLQueueNode* N = list[index];
  while(N!=NULL)
    {
      if(strcmp(url,N->url)==0)
	{
	  return true;
	}
      N = N->next;
    }
  return false;
}

void add_list_node(char *url,URLQueueNode** list)
{
  int index = hashing(url);
  URLQueueNode* node = createURLQueueNode(url);
  URLQueueNode* N;
  if (list[index]==NULL)
    {
      list[index] = node;
    }
  else
    {
      N = list[index];
      while(N->next!=NULL)
	{
	  N = N->next;
	}
      N->next = node;
    }  
}

void delete_list(URLQueueNode** list)
{
  URLQueueNode* current;
  URLQueueNode* next;
  for(int i=0;i<100;i++)
    {
      if(list[i]!=NULL)
	{
	  current = list[i];
	  while(current!=NULL)
	    {
	      next = current->next;
	      delete_node(current);
	      current=next;
	    }
	}
    }
  free(list);
}

bool url_filter(URLQueueNode *node) //implement crawler policies here
{
  if (node->depth < depth_limit)
    return true; // URL is valid
  else
    return false; // URL depth exceeds limit
}
void extract_url(char *html, URLQueue *queue,URLQueueNode* parent ) //pass parent
{
  char *sub;
  int i;
  int j;
  if(html==NULL)
    {
      return;
    }
  if(strlen(html)<5)
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
    newNode->depth = parent->depth+1;
    if (!url_filter(newNode))
    {
      delete_node(newNode);
      free(furl);
      return;
    }
    enqueue(newNode, queue);

    free(furl);

    html = html + (sizeof(char) * i);
    extract_url(html, queue,parent);
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
int fetchurl(URLQueue *queue,FILE* file, URLQueueNode** list) // fetches url in response struct
{
  //-1 to end;  0 for failed get request; 1 for continue
  URLQueueNode *node = dequeue(queue);
  if(node==NULL)
    {
      return -1;
    }

  char *url = node->url;
  
  if(check_visited(node->url,list))
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
    printf("REQUEST FAILED\n");
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
    free(response.string);
    curl_easy_cleanup(curl);
    delete_node(node);
    return 0;
  }
  if (response.size<5)
    {
    free(response.string);
    curl_easy_cleanup(curl);
    delete_node(node);
      return 0;
    }
  
  
  printf("%s\n",node->url);
  //logURL(file, node->url);
  add_list_node(node->url,list);
  curl_easy_cleanup(curl);
  extract_url(response.string, queue,node);
  free(response.string);
  delete_node(node);
  return 1;
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
  depth_limit = 2;

  URLQueueNode *firstNode = createURLQueueNode(url);
  firstNode->depth = 0;
  enqueue(firstNode, queue);
  FILE *file = fopen("log.txt", "w+"); 
  if (file == NULL)
  {
    printf("Error opening file!\n");
    return EXIT_FAILURE;
  }

  URLQueueNode** list = create_visitor_list(100);
  
  while(fetchurl(queue,file,list)!=-1)
    {
    }
  delete_queue(queue);
  delete_list(list);
  fclose(file); // close the file
  free(queue);

  return EXIT_SUCCESS;
}
