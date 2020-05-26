/*  ==========================================================  */
/*                    Author: Pawel Muller                      */
/*                 EAN8 Barcode Generator (x86)                 */
/*  ==========================================================  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

extern char *encodeBarcode(const char *ean_code_in, char *ean_code_encoded, const char *coding);
extern char *writeBarcodeRow(const char *encoded_barcode, unsigned char *image_with_frame);

typedef struct {
	unsigned short bfType; 
	unsigned long  bfSize; 
	unsigned short bfReserved1; 
	unsigned short bfReserved2; 
	unsigned long  bfOffBits; 
	unsigned long  biSize; 
	long  biWidth; 
	long  biHeight; 
	short biPlanes; 
	short biBitCount; 
	unsigned long  biCompression; 
	unsigned long  biSizeImage; 
	long biXPelsPerMeter; 
	long biYPelsPerMeter; 
	unsigned long  biClrUsed; 
	unsigned long  biClrImportant;
	unsigned long  RGBQuad_0;
	unsigned long  RGBQuad_1;
} bmpHdr;

typedef struct {
	int width, height;		// Image width and height
	unsigned char* pImg;	// Pointer to the beginning of the image section itself
	int cX, cY;				// "Current coordinates" 
	int col;				// "Current color"
} imgInfo;

void *freeResources(FILE *pFile, void *pFirst, void *pSnd) {
	if (pFile != 0)
		fclose(pFile);
	if (pFirst != 0)
		free(pFirst);
	if (pSnd !=0)
		free(pSnd);
	return 0;
}

imgInfo *InitScreen (int w, int h) {
	imgInfo *pImg;
	if ((pImg = (imgInfo *) malloc(sizeof(imgInfo))) == 0)
		return 0;
	pImg -> height = h;
	pImg -> width = w;
	pImg -> pImg = (unsigned char*) malloc((((w + 31) >> 5) << 2) * h);
	if (pImg -> pImg == 0) {
		free(pImg);
		return 0;
	}
	memset(pImg -> pImg, 0xFF, (((w + 31) >> 5) << 2) * h);
	pImg -> cX = 0;
	pImg -> cY = 0;
	pImg -> col = 0;
	return pImg;
}

void freeScreen(imgInfo *pInfo) {
	if (pInfo && pInfo -> pImg)
		free(pInfo -> pImg);
	if (pInfo)
		free(pInfo);
}

int saveBMP(const imgInfo *pInfo, const char* fname) {
	int lineBytes = ((pInfo -> width + 31) >> 5) << 2;
	bmpHdr bmpHead = 
	{
	0x4D42,							// unsigned short bfType; 
	sizeof(bmpHdr),					// unsigned long  bfSize; 
	0, 0,							// unsigned short bfReserved1, bfReserved2; 
	sizeof(bmpHdr),					// unsigned long  bfOffBits; 
	40,								// unsigned long  biSize; 
	pInfo -> width,					// long  biWidth; 
	pInfo -> height,				// long  biHeight; 
	1,								// short biPlanes; 
	1,								// short biBitCount; 
	0,								// unsigned long  biCompression; 
	lineBytes * pInfo -> height,	// unsigned long  biSizeImage; 
	11811,							// long biXPelsPerMeter; = 300 dpi
	11811,							// long biYPelsPerMeter; 
	2,								// unsigned long  biClrUsed; 
	0,								// unsigned long  biClrImportant;
	0x00000000,						// unsigned long  RGBQuad_0;
	0x00FFFFFF						// unsigned long  RGBQuad_1;
	};

	FILE *fbmp;
	unsigned char *ptr;
	int y;

	if ((fbmp = fopen(fname, "wb")) == 0)
		return -1;
	if (fwrite(&bmpHead, sizeof(bmpHdr), 1, fbmp) != 1) {
		fclose(fbmp);
		return -2;
	}

	ptr = pInfo -> pImg + lineBytes * (pInfo -> height - 1);
	for (y = pInfo -> height; y > 0; --y, ptr -= lineBytes)
		if (fwrite(ptr, sizeof(unsigned char), lineBytes, fbmp) != lineBytes) {
			fclose(fbmp);
			return -3;
		}
	fclose(fbmp);
	return 0;
}

void stretchBarcode(imgInfo *pInfo, const int px_to_leave, const int bytes_per_row) {
	// Copies one line to the entire length of the image, including the frame
	unsigned char *written_line = pInfo->pImg + px_to_leave * bytes_per_row;
	unsigned char *image_to_write = written_line + bytes_per_row;

	int lines_to_write = pInfo -> height - 2 * px_to_leave - 1;
	int i = 0, j = 0; 

	while (lines_to_write-- > 0) {
		memcpy(image_to_write, written_line, bytes_per_row);
		image_to_write += bytes_per_row;
	}
}

char convertBarcode(char *ean_code_out, const char *ean_code_encoded, const int width_factor, const int center_factor) {
	char px_frame_offset = center_factor & 0b111; // Calculation of the number of pixels remaining to create the frame
	char current_color = '0';

	ean_code_out[0] = px_frame_offset;
	
	int i = 0, j = 0;
	for (i = 0; i < 67; ++i)
		if (ean_code_encoded[i] == current_color) {
			ean_code_out[j] += width_factor;
		} else {
			ean_code_out[++j] = width_factor;
			current_color ^= 0b1;
		}
	ean_code_out[++j] = 0;
}

void createBarcode(const char *ean_code_in, imgInfo *pInfo) {
	int image_width = pInfo -> width, image_height = pInfo -> height;
	char ean_code_encoded[] = "101XXXXXXXXXXXXXXXXXXXXXXXXXXXX01010XXXXXXXXXXXXXXXXXXXXXXXXXXXX101";

	// Coding (ASM)
	encodeBarcode(ean_code_in, ean_code_encoded, "0001101X0011001X0010011X0111101X0100011X0110001X0101111X0111011X0110111X0001011X");
	
	printf("Given:   %s \n", ean_code_in);
	printf("Encoded: %s \n", ean_code_encoded);

	// Calculations:
	int width_factor = image_width / 67; // Bar width factor
	int center_factor = (image_width - width_factor * 67) / 2; // Center factor
	int bytes_per_row = ((image_width + 31) >> 5) << 2; // Bytes per row bytes
	int px_to_leave = image_height < 20 ? 1 : 0.05 * image_height; // Pixels to leave on frame (top and bottom)
	int byte_frame_offset = center_factor >> 3; // Bytes to leave on frame (top and bottom)

	// Creating a char array that stores the number of pixels of the same color in a row
	char ean_code_out[68];
	convertBarcode(ean_code_out, ean_code_encoded, width_factor, center_factor);

	// Write one line of code to memory (ASM)
	writeBarcodeRow(ean_code_out, pInfo->pImg + px_to_leave * bytes_per_row + byte_frame_offset);

	// Code extension to the rest of the picture
	stretchBarcode(pInfo, px_to_leave, bytes_per_row);
}

int main() {
	imgInfo *pInfo;

	if (sizeof(bmpHdr) != 62)
	{
		printf("Change compilation options so as bmpHdr struct size is 62 bytes.\n");
		return 1;
	}

	pInfo = InitScreen(400, 100);

	createBarcode("21099732", pInfo);
	saveBMP(pInfo, "result.bmp");

	freeScreen(pInfo);
	return 0;
}
