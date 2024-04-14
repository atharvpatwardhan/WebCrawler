#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

// Okay
// Define a structure for queue elements.
typedef struct URLQueueNode {
    char *url;
    struct URLQueueNode *next;
} URLQueueNode;

// Define a structure for a thread-safe queue.
typedef struct {
    URLQueueNode *head, *tail;
    pthread_mutex_t lock;
} URLQueue;


void initQueue(URLQueue *queue);
char *dequeue(URLQueue *queue);
void enqueue(URLQueue *queue, const char *url);

// Placeholder for the function to fetch and process a URL.
void *fetch_url(void *arg);



int main(int argc, char ** argv)
{
    if(argc <2)
    {
        printf("not enough args\n");
        return EXIT_SUCCESS;
    }
    
    char* url = argv[1];
    printf("%s\n",url);

    CURL* curl;
    CURLcode result;
    curl = curl_easy_init();

    if(curl==NULL)
      {
	printf("REQUEST FAILED\n");
	//return 1;
      }

    curl_easy_setopt(curl, CURLOPT_URL,url);
    result =curl_easy_perform(curl);

    if(result != CURLE_OK)
      {
	printf("Error %s\n",curl_easy_strerror(result));
	//return -1;
      }

    curl_easy_cleanup(curl);
    
    return EXIT_SUCCESS;
}
