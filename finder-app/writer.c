#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int count = 0;
    int temp = argc;
    while(temp)
    {
        printf("%s\n", argv[count]);
        count++;
        temp--;
    }

    //printf("Dhiraj Argument Count = %d\n", argc);

    if(argc < 3)
    {
        syslog(LOG_ERR , "Insufficient Arguments");
        exit(1);
        return 1;
    }

    char writefile[100];
    char writestr[100];

    strcpy(writefile , argv[1]);
    strcpy(writestr , argv[2]);

    //printf("%s\n", writefile);
    //printf("%s\n", writestr);

    char *base;
    base = basename(writefile);

    
    FILE *filePointer; 

    filePointer = fopen(writefile , "w");

    if (filePointer == NULL)
    {
        syslog(LOG_ERR , "File could not be created");
        exit(1);
        return 1;
    }

    fputs(writestr, filePointer);
    syslog(LOG_DEBUG, "Writing %s to %s", writestr , writefile);

    fclose(filePointer);

    //printf("%s\n", base);

    return 0;
}
