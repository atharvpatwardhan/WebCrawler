#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
    if(argc <2)
    {
        printf("not enough args\n");
        return EXIT_SUCCESS;
    }
    FILE* file = fopen(argv[1],"r");
    if(file==NULL)
    {
        printf("Error file did not open\n");
        return EXIT_SUCCESS;
    }
    char* url;
    if(fscanf(file,"%s",url)!=1)
    {
        printf("file reading error\n");
        return EXIT_SUCCESS;
    }
    printf("Hello world!\n");
    return EXIT_SUCCESS;
}
