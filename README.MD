# Webcrawler

#Libraries : 
1)	libcurl
2)	pthread
3)	tiddylib

Step by step guide to design and implment Webcrawler

1>	Set up input and output and Test

2>	Extract Url from seed

3> 	Send Get request

3>	Store Html responce

4>	extract href

5>	implement queue

6>	implement BSF on a single thread and test all functions

7>	implement depth control

8>	Test all function rigoursly

9> 	implment multi thread and implement shared queue and synchronise it

10>	Handle errors

11> 	final testing.

API

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

int hashing(char *url);


void initQueue(URLQueue *queue);

URLQueueNode *dequeue(URLQueue *queue);

void enqueue(URLQueueNode *newNode, URLQueue *queue);


URLQueueNode *createURLQueueNode(char *url);


URLQueue *createURLQueue();


void extract_url(char *html, URLQueue *queue, URLQueueNode* parent);

bool url_filter(URLQueueNode *node);

void *fetch_url(void *url,FILE* file, URLQueueNode** list);

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);

void logURL(FILE *file, const char *url);



URLQueueNode** create_visitor_list(int size);

void add_list_node(char *url,URLQueueNode** list);

void delete_list(URLQueueNode** list);


