struct fileop_handle
{
    FILE *file;
};

int fileop_init(void);
fileop_handle *fileop_open(const char *, const char *);
void fileop_close(fileop_handle *);
int fileop_read(void *, int, fileop_handle *);

