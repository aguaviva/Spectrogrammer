#include "FilePicker.h"
#include <dirent.h>
#include <stdlib.h> 
#include <string.h>

linked_list *GetFilesInFolder(const char *pDirectory)
{
    linked_list *head = NULL;

    DIR *dir = opendir(pDirectory);
    if (dir != NULL) 
    {
        for(;;) 
        {
            struct dirent *ent = readdir(dir);
            if (ent==NULL)
                break;
            const char *pS = ent->d_name;

            if (strcmp(pS, ".")==0 || strcmp(pS, "..")==0)
                continue;

            linked_list *node = (linked_list *)malloc(sizeof(linked_list));
            node->pStr = (char*)malloc(strlen(pS)+1);
            strcpy(node->pStr, pS);            

            node->pNext = head;
            head = node;
        }
        closedir (dir);    
    }
    return head;
}

int LinkedListSize(linked_list *node)
{
    int i=0;
    while(node!=NULL)
    {
        i++;
        node = node->pNext;
    }
    return i;
}


int comp (const void *elem1, const void *elem2) 
{
    return strcmp(((const linked_list *)elem1)->pStr, ((const linked_list *)elem2)->pStr);
}

linked_list *SortLinkedList(linked_list *node)
{
    int count = LinkedListSize(node);
    if (count<=1)
        return node;

    linked_list **array = (linked_list **)malloc(sizeof(linked_list **) * count);

    for(int i=0; i<count; i++)
    {
        array[i] = node;
        node = node->pNext;
    }

    qsort(array, count, sizeof(linked_list **), comp);

    for(int i=0; i<count-1; i++)
    {
        array[i]->pNext = array[i+1];
    }
    array[count-1]->pNext = NULL;

    linked_list *head = array[0];

    free(array);

    return head;
}


void FreeLinkedList(linked_list *node)
{    
    while(node!=NULL)
    {
        free(node->pStr);
        linked_list *next = node->pNext;
        free(node);
        node = next;
    }
}