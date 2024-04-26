
# Webcrawler: client side program to traverse through a seed and fetch all URL embedded and then visit them. A breath First Search on websites.


how to Run
you will need the following Libraries and can be on any Unix system only
#Libraries : 
1)	libcurl
2)	pthread
3)	tiddylib

Commands to execute and Run (Check Makefile)

make : to compile and create a object file

make run : to run the created object file

make clean : to remove existing object file 



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

int depth_limit; //max height of bfs tree



int hashing(char *url);
// return an index to locate a url in Visited 

URLQueueNode *dequeue(URLQueue *queue);
//Deque an element from queue

void enqueue(URLQueueNode *newNode, URLQueue *queue);
//enqueue and element to the queue


URLQueueNode *createURLQueueNode(char *url);
//create a queue Node


URLQueue *createURLQueue();
// create a queue, set head and tail of the queue to NULL

void extract_url(char *html, URLQueue *queue, URLQueueNode* parent);
extract URL from a given HTML page, then enques the URL


bool url_filter(URLQueueNode *node);
//Implement web crawler policies here like depth limit

int get_html(void *url,FILE* file, URLQueueNode** list);
//sends a get to a given URL, receives HTML and call extract_url()


size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
check LIBCURL documentation, write received HTML in a string in chunks is called recursively

void logURL(FILE *file, const char *url);
implement logging to file here


URLQueueNode** create_visitor_list(int size);
create a list to check for visited URLs

void add_list_node(char *url,URLQueueNode** list);
add a node to theist

void delete_list(URLQueueNode** list);
delete list


