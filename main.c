#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <regex.h>
#include "stack.h"
#include <semaphore.h>
#include <curl/curl.h>

#include "list.h"
const int LISTEN_SKIP = 0;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

sem_t mutex;
LIST *result_list;

int get_and_write_rss(char *url)
{

    printf("\n\nbegin for %ss\n\n", url);
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char *outfilename = "rss.xml";
    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(outfilename, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        sem_wait(&mutex);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        fclose(fp);

        sem_post(&mutex);

        sem_wait(&mutex);

        fp = fopen(outfilename, "r");
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        if (fp == NULL)
            exit(EXIT_FAILURE);

        unsigned int count_total = 0;
        unsigned int count_captured = 0;

        int is_item_group = 0;
        int i;
        char *p;

        while ((read = getline(&line, &len, fp)) != -1)
        {

            char *captured;
            char source_copy[strlen(line) + 1];
            regex_t regex_compiled;
            regmatch_t group_array[3];

            if (regcomp(&regex_compiled, "<title>(.*?)</title>", REG_EXTENDED))
            {
                printf("Could not compile regular expression.\n");
                return 1;
            };

            if (regexec(&regex_compiled, line, 3, group_array, 0) == 0)
            {
                strcpy(source_copy, line);
                source_copy[group_array[1].rm_eo] = 0;
                captured = (source_copy + group_array[1].rm_so);

                if (FindNodeByValue(result_list, captured, strlen(captured) + 1) == NULL)
                {
                    // printf("\n\nAdded\n\n");

                    AddNode(result_list, captured, strlen(captured) + 1);
                    // printf("%s\n", source_copy + group_array[1].rm_so);
                    count_captured++;
                }
            }

            regfree(&regex_compiled);

            count_total++;
        }
        LISTNODE *node = result_list->head;
        while ((node))
        {
            printf("\ntitle: %s --", (char *)node->data);
            node = node->next;
        }

        printf("\n --%d-- captured new titles\n", count_captured);
        printf(" -%d- total lines\n\n", count_total);
        // printf("\n\ndownloaded\n\n");

        sem_post(&mutex);

        pthread_exit(count_captured);
    }

    pthread_exit(0);
}

void start()
{
    sem_init(&mutex, 0, 1);
    result_list = malloc(sizeof(LIST));
    memset(result_list, 0x00, sizeof(LIST));

    LIST *list;
    int i;
    char *p;

    list = malloc(sizeof(LIST));
    memset(list, 0x00, sizeof(LIST));

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("urls.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int count = 0;
    while ((read = getline(&line, &len, fp)) != -1) // read all urls
    {
        line[strcspn(line, "\n")] = 0; // remove \n
        AddNode(list, line, strlen(line));
        count++;
    }

    pthread_t *id = malloc(sizeof(pthread_t) * count);

    fclose(fp);
    if (line)
        free(line);

    int threadi = 0;
    LISTNODE *node;
    node = list->head;

    while ((node))
    {
        printf("here: %s --", (char *)node->data);
        const char *data = (char *)node->data;
        char *copy = malloc(node->sz + 1);
        strcpy(copy, data);
        printf(copy);
        pthread_create((id + threadi), 0, get_and_write_rss, (char *)node->data);
        node = node->next;
        threadi++;
    }

    for (int i = 0; i < count; i++)
    {
        printf("gojoin %d ", i);
        pthread_join(*(id + i), NULL);
    }
}

int startListen()
{
    int out = fork();
    if (out == 0)
    {
        start();
        return LISTEN_SKIP;
    }

    return out;
}

int main()
{
    startListen();
    return 0;
}