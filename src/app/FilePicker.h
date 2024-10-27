
struct linked_list
{
    char *pStr;
    linked_list *pNext;
};

linked_list *GetFilesInFolder(const char *pDirectory);
linked_list *SortLinkedList(linked_list *node);
void FreeLinkedList(linked_list *node);