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

typedef struct
{
  char *string;
  size_t size;
} Response;



size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void initQueue(URLQueue *queue);
char *dequeue(URLQueue *queue);
void enqueue(URLQueue *queue, const char *url);

// Placeholder for the function to fetch and process a URL.
void *fetch_url(void *arg);


size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata)
{
  size_t real_size = size * nmemb; 
  
  // The function prototype requires the 4th parameter to be a void pointer, but
  // WE know it's really a pointer to a Response struct so we type cast it here.
  Response *response = (Response *) userdata; 
  
  // Attempt to reallocate space for a larger block of memory for the Response 
  // struct string member to point to... we increase the size of the block of 
  // memory by the existing size PLUS the size of the chunk and 1 more byte to
  // store the null terminator.
  char *ptr = realloc(response->string, response->size + real_size + 1);

  if (ptr == NULL)
  {
    return 0;
  }
  
  response->string = ptr;
  

  memcpy(&(response->string[response->size]), data, real_size);

  // Add the size of the chunk to the size member to keep track of the size of
  // the string received.
  response->size += real_size;

  // Set the last character of the block of memory for the string to the null 
  response->string[response->size] = '\0';
   
  // Return the size of the chunk in bytes as required by libcurl
  return real_size;
}


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
    Response response;
    response.string = malloc(1);
    response.size = 0;
    

    if(curl==NULL)
      {
	printf("REQUEST FAILED\n");
	//return 1;
      }

    curl_easy_setopt(curl, CURLOPT_URL,url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
    result =curl_easy_perform(curl);

    if(result != CURLE_OK)
      {
	printf("Error %s\n",curl_easy_strerror(result));
	//return -1;
      }


    printf("%s\n", response.string);

    free(response.string);
    curl_easy_cleanup(curl);
    
    return EXIT_SUCCESS;
}
