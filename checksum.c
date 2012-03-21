#include "checksum.h"

unsigned long ulTable[256]; // CRC lookup table array.
/*
	Reflection is a requirement for the official CRC-32 standard.
	You can create CRCs without it, but they won't conform to the standard.
*/
unsigned long reflect(unsigned long ulReflect, const char cChar)
{
	int iPos;
	unsigned long ulValue = 0;

	// Swap bit 0 for bit 7, bit 1 For bit 6, etc....
	for(iPos = 1; iPos < (cChar + 1); iPos++) {
		if(ulReflect & 1) {
			ulValue |= (1 << (cChar - iPos));
		}
		ulReflect >>= 1;
	}
	return ulValue;
}

void partialCRC(unsigned long *ulCRC, const unsigned char *sData, unsigned long ulDataLength)
{
	while(ulDataLength--) {
		// If your compiler complains about the following line, try changing each
		// occurrence of *ulCRC with "((unsigned long)*ulCRC)" or "*(unsigned long *)ulCRC".

		 *(unsigned long *)ulCRC =
			((*(unsigned long *)ulCRC) >> 8) ^ ulTable[((*(unsigned long *)ulCRC) & 0xFF) ^ *sData++];
	}
}

int fileCRC(const char *sFileName, unsigned long *ulOutCRC)
{
	unsigned long ulBufferSize = 512;
	*(unsigned long *)ulOutCRC = 0xffffffff; //Initilaize the CRC.

	FILE *fSource = NULL;
	unsigned char *sBuf = NULL;
	int iBytesRead = 0;

	if((fSource = fopen(sFileName, "rb")) == NULL) {
		return -1; //Failed to open file for read access.
	}

	if(!(sBuf = (unsigned char *)malloc(ulBufferSize))) //Allocate memory for file buffering.
	{
		fclose(fSource);
		return -1; //Out of memory.
	}

	while((iBytesRead = fread(sBuf, sizeof(char), ulBufferSize, fSource))){
		partialCRC(ulOutCRC, sBuf, iBytesRead);
	}

	free(sBuf);
	fclose(fSource);

	*(unsigned long *)ulOutCRC ^= 0xffffffff; //Finalize the CRC.

	return 0;
}

void crc32_init(void)
{
	int iCodes;
	int iPos;
	//0x04C11DB7 is the official polynomial used by PKZip, WinZip and Ethernet.
	unsigned long ulPolynomial = 0x04C11DB7;

	// 256 values representing ASCII character codes.
	for(iCodes = 0; iCodes <= 0xFF; iCodes++) {
		ulTable[iCodes] = reflect(iCodes, 8) << 24;

		for(iPos = 0; iPos < 8; iPos++) {
			ulTable[iCodes] = (ulTable[iCodes] << 1)
				^ ((ulTable[iCodes] & (1 << 31)) ? ulPolynomial : 0);
		}

		ulTable[iCodes] = reflect(ulTable[iCodes], 32);
	}
}
