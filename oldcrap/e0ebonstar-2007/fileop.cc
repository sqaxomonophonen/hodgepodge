// fileop - file operations
// current acts as a wrapper for stdio.h type file functions
// later on it's meant to be able to load files from a multifile package (with no changes in the prototypes)

#include <stdio.h> 

#include "fileop.h"

struct fileop_vars_s {
} fileop_vars;


int fileop_init(void)
{
    return 0;
}

fileop_handle *fileop_open(const char *filename, const char *type)
{
    fileop_handle *handle;
    FILE *file;
    
    file = fopen(filename, type);
    if(file == NULL) return NULL;

    handle = new struct fileop_handle;
    handle->file=file;

    return handle;
}

void fileop_close(fileop_handle *handle)
{
    fclose(handle->file);
    delete handle;
}

int fileop_read(void *pointer, int n, fileop_handle *handle)
{
    int i;
    
    i = fread(pointer, 1, n, handle->file);
    
    return i;    
}


