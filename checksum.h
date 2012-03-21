#include <stdio.h>
#include <stdlib.h>

void crc32_init(void);
int fileCRC(const char *sFileName, unsigned long *ulOutCRC);
