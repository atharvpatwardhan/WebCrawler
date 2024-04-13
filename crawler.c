#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>


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
    printf("%s\n",argv[1]);
    FILE* file = fopen(argv[1],"r");
    if(file==NULL)
    {
        printf("Error file did not open\n");
        return EXIT_SUCCESS;
    }
    
    fseek(file,0,SEEK_END);
    unsigned long fsize = ftell(file);
    printf("%lu\n",fsize);
    
    fseek(file,0,SEEK_SET);
    
    char* url = malloc(fsize*sizeof(char));
    int ret = fscanf(file,"%s",url);

    if(ret!=1)
    {
        printf("file reading error\n");
        free(url);
        return EXIT_SUCCESS;
    }
    
    
    CURL* curl; // HANDLE FOR ALL LIBCURL FUNCTIONS
    CURLcode result; 	//to store result code from server
    
    
   curl = curl_easy_init();
   if (curl == NULL)
   {
       fprintf(stderr, "HTTP request failed\n");
       return -1;
   }
  
   curl_easy_setopt(curl, CURLOPT_URL, url);
   result = curl_easy_perform(curl);
    
   if(result!=CURLE_OK)
    {
      printf("error : %s\n",curl_easy_strerror(result));
    	return -1;
    }
    curl_easy_cleanup(curl);
    fclose(file);
    free(url);
    return EXIT_SUCCESS;
}
