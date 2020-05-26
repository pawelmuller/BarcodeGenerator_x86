/*  ==========================================================  */
/*                 Autor: Pawel Muller (304080)                 */
/*            Temat: generator kodow kreskowych EAN8            */
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
	int width, height;		// szerokosc i wysokosc obrazu
	unsigned char* pImg;	// wskazanie na początek danych pikselowych
	int cX, cY;				// "aktualne współrzędne" 
	int col;				// "aktualny kolor"
} imgInfo;

void* freeResources(FILE* pFile, void* pFirst, void* pSnd) {
	if (pFile != 0)
		fclose(pFile);
	if (pFirst != 0)
		free(pFirst);
	if (pSnd !=0)
		free(pSnd);
	return 0;
}

imgInfo* InitScreen (int w, int h) {
	imgInfo *pImg;
	if ( (pImg = (imgInfo *) malloc(sizeof(imgInfo))) == 0)
		return 0;
	pImg->height = h;
	pImg->width = w;
	pImg->pImg = (unsigned char*) malloc((((w + 31) >> 5) << 2) * h);
	if (pImg->pImg == 0)
	{
		free(pImg);
		return 0;
	}
	memset(pImg->pImg, 0xFF, (((w + 31) >> 5) << 2) * h);
	pImg->cX = 0;
	pImg->cY = 0;
	pImg->col = 0;
	return pImg;
}

void freeScreen(imgInfo* pInfo) {
	if (pInfo && pInfo->pImg)
		free(pInfo->pImg);
	if (pInfo)
		free(pInfo);
}

int saveBMP(const imgInfo* pInfo, const char* fname) {
	int lineBytes = ((pInfo->width + 31) >> 5)<<2;
	bmpHdr bmpHead = 
	{
	0x4D42,				// unsigned short bfType; 
	sizeof(bmpHdr),		// unsigned long  bfSize; 
	0, 0,				// unsigned short bfReserved1, bfReserved2; 
	sizeof(bmpHdr),		// unsigned long  bfOffBits; 
	40,					// unsigned long  biSize; 
	pInfo->width,		// long  biWidth; 
	pInfo->height,		// long  biHeight; 
	1,					// short biPlanes; 
	1,					// short biBitCount; 
	0,					// unsigned long  biCompression; 
	lineBytes * pInfo->height,	// unsigned long  biSizeImage; 
	11811,				// long biXPelsPerMeter; = 300 dpi
	11811,				// long biYPelsPerMeter; 
	2,					// unsigned long  biClrUsed; 
	0,					// unsigned long  biClrImportant;
	0x00000000,			// unsigned long  RGBQuad_0;
	0x00FFFFFF			// unsigned long  RGBQuad_1;
	};

	FILE * fbmp;
	unsigned char *ptr;
	int y;

	if ((fbmp = fopen(fname, "wb")) == 0)
		return -1;
	if (fwrite(&bmpHead, sizeof(bmpHdr), 1, fbmp) != 1)
	{
		fclose(fbmp);
		return -2;
	}

	ptr = pInfo->pImg + lineBytes * (pInfo->height - 1);
	for (y=pInfo->height; y > 0; --y, ptr -= lineBytes)
		if (fwrite(ptr, sizeof(unsigned char), lineBytes, fbmp) != lineBytes)
		{
			fclose(fbmp);
			return -3;
		}
	fclose(fbmp);
	return 0;
}

void stretchBarcode(imgInfo* pInfo, const int px_to_leave, const int bytes_per_row) {
	// Kopiuje jedna linie na cala dlugosc obrazka, z uwzglednieniem ramki
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
	char px_frame_offset = center_factor & 0b111; // Obliczenie ilosci pikseli pozostalych do stworzenia ramki
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

void createBarcode(const char* ean_code_in, imgInfo* pInfo) {
	int image_width = pInfo -> width, image_height = pInfo -> height;
	char ean_code_encoded[] = "101XXXXXXXXXXXXXXXXXXXXXXXXXXXX01010XXXXXXXXXXXXXXXXXXXXXXXXXXXX101";

	// Kodowanie (ASM)
	encodeBarcode(ean_code_in, ean_code_encoded, "0001101X0011001X0010011X0111101X0100011X0110001X0101111X0111011X0110111X0001011X");
	
	printf("Given:   %s \n", ean_code_in);
	printf("Encoded: %s \n", ean_code_encoded);

	// Obliczanie potrzebnych wartosci:
	int width_factor = image_width / 67; // Wsplczynnik szerokosci paska
	int center_factor = (image_width - width_factor * 67) / 2; // Wspolczynnika wyposrodkowania
	int bytes_per_row = ((image_width + 31) >> 5) << 2; // Ilosc bajtow na wiersz
	int px_to_leave = image_height < 20 ? 1 : 0.05 * image_height; // Ilosc pikseli, ktora nalezy pozostawic na ramke z gory i z dolu
	int byte_frame_offset = center_factor >> 3; // Ilosc bajtow potrzebnych do stworzenia ramki z gory oraz dolu

	// Tworzenie tablicy typu char, ktora przechowuje ilosc pikseli tego samego koloru pod rzad
	char ean_code_out[68];
	convertBarcode(ean_code_out, ean_code_encoded, width_factor, center_factor);

	// Zapis jednego wiersza kodu do pamieci (ASM)
	writeBarcodeRow(ean_code_out, pInfo->pImg + px_to_leave * bytes_per_row + byte_frame_offset);

	// Rozszerzenie kodu na pozostala czesc obrazka
	stretchBarcode(pInfo, px_to_leave, bytes_per_row);
}

int main() {
	imgInfo* pInfo;

	if (sizeof(bmpHdr) != 62)
	{
		printf("Change compilation options so as bmpHdr struct size is 62 bytes.\n");
		return 1;
	}

	pInfo = InitScreen(400, 100);

	createBarcode("21099732", pInfo);
	saveBMP(pInfo, "Generated_barcodes/result_21099732.bmp");

	freeScreen(pInfo);
	return 0;
}
