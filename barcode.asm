section .text
global encodeBarcode, writeBarcodeRow

; ====================== encodeBarcode ======================= ;

encodeBarcode:
; PROLOG
	push EBP
	mov EBP, ESP

    push EBX
    push ESI
    push EDI

	mov EBX, DWORD[EBP+8]
	mov ESI, DWORD[EBP+12]
	mov EDI, DWORD[EBP+16]

; Wlasciwa procedura:
;   EBX - ean_code_in (8 cyfr)
;   ESI - ean_code_encoded (wyjsciowy ciag zer i jedynek reprezentujacy paski kodu kreskowego)
;   EDI - coding (string kodowania)
;   DL - rodzaj kodowania - flaga

    add ESI, 2

    mov DL, 0 ; Flaga - kodownie "lewe"
    call encode
    call encode
    call encode
    call encode

    add ESI, 5
    mov DL, 1 ; Flaga - kodownie "prawe"
    call encode
    call encode
    call encode
    call encode

; EPILOG
    pop EDI
    pop ESI
    pop EBX
	pop EBP
	ret

encode:
;   CL - pojedynczy znak kodu jawnego
;   EAX - adres uzywanego znaku w stringu kodujacym
;   CH - uzywany znak z stringa kodujacego

    mov EAX, 0 ; Zerowanie rejestru EAX do obliczen
    mov CL, BYTE[EBX] ; Wczytanie znaku (EBX - ean_code_in)
    sub CL, '0'
    shl CL, 3 ; Mnozenie przez 8, by uzyskac indeks w stringu kodujacym 
    mov AL, CL
    add EAX, EDI ; Obliczenie adresu pierwszego znaku w stringu kodujacym
    mov CH, BYTE[EAX] ; Odczyt pierwszego znaku

encodeLoop:
    inc ESI ; Przejscie do nastepnego znaku w stringu zakodowanym (ESI - ean_code_encoded)
    xor CH, DL ; Uwzglednienie flagi - kodowanie lewe (0) lub prawe (1)
    mov BYTE[ESI], CH ; Zapis

    inc EAX ; Przejscie do nastepnego znaku w stringu kodujacym
    mov CH, BYTE[EAX] ; Odczyt znaku
    cmp CH, 'X'
    jne encodeLoop

encodeExit:
    inc EBX ; Przejscie do nastepnego znaku
    ret



; ===================== writeBarcodeRow ====================== ;

writeBarcodeRow:
; PROLOG
    push EBP
    mov EBP, ESP

    push EDI
    push ESI

	mov EDI, DWORD[EBP+8]
	mov ESI, DWORD[EBP+12]


; Wlasciwa procedura:
;   EDI - encoded_barcode
;   ESI - image_with_frame

    mov AH, 1 ; Inicjalizacja koloru poczatkowego (bialy)
    mov AL, 0 ; Inicjalizacja bufora na pojedyncze bajty obrazka
    mov CL, 8 ; Inicjalizacja licznika bitow

    mov CH, BYTE[EDI] ; Odczytanie ilosci pikseli koloru
    cmp CH, 0
    je changeColor

writeByte:
    shl AL, 1
    add AL, AH ; Ustawienie ostatniego bitu na zadany kolor

    dec CL ; Dekrementacja licznika bitow
    jnz writeByteContinuation

saveImageByte:
    mov BYTE[ESI], AL
    inc ESI
    mov AL, 0 ; Czyszczenie bufora na pojedyncze bajty obrazka
    mov CL, 8 ; Ustawienie licznika bitow

writeByteContinuation:
    dec CH
    jnz writeByte

changeColor:
    xor AH, 0b1 ; Negacja ostatniego bitu
    inc EDI ; Przejscie do nastepnego koloru
    mov CH, BYTE[EDI]
    cmp CH, 0
    jnz writeByte

writeBarcodeRow_exit:
; Poprawienie i zapis ostatniego, nieprzetwarzanego bajtu:
    shl AL, 1
    add AL, 1
    dec CL
    jnz writeBarcodeRow_exit

    mov BYTE[ESI], AL

; EPILOG
    pop ESI
    pop EDI
	pop EBP
	ret
