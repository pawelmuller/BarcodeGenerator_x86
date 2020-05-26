section .text
global encodeBarcode, writeBarcodeRow

; ====================== encodeBarcode ======================= ;

encodeBarcode:
; PROLOGUE
	push EBP
	mov EBP, ESP

    push EBX
    push ESI
    push EDI

	mov EBX, DWORD[EBP+8]
	mov ESI, DWORD[EBP+12]
	mov EDI, DWORD[EBP+16]

; Procedure:
;   EBX - ean_code_in (8 digit C string)
;   ESI - ean_code_encoded (the output string of 1 and 0 representing barcode bars)
;   EDI - coding
;   DL - encoding type - flag

    add ESI, 2

    mov DL, 0 ; Flag - "left" coding
    call encode
    call encode
    call encode
    call encode

    add ESI, 5
    mov DL, 1 ; Flag - "roght" coding
    call encode
    call encode
    call encode
    call encode

; EPILOGUE
    pop EDI
    pop ESI
    pop EBX
	pop EBP
	ret

encode:
;   CL - single code character
;   EAX - address of the character used in the coding string
;   CH - used character from the coding string

    mov EAX, 0 ; Resetting the EAX registry for calculation
    mov CL, BYTE[EBX] ; Loading a character
    sub CL, '0'
    shl CL, 3 ; Multiplication by 8 to get the index in the coding string 
    mov AL, CL
    add EAX, EDI ; Calculation of the address of the first character in the coding string
    mov CH, BYTE[EAX] ; Reading the first character

encodeLoop:
    inc ESI ; Move to the next character in coded string
    xor CH, DL ; Flag inclusion - left (0) or right (1) coding
    mov BYTE[ESI], CH ; Saving

    inc EAX ; Move to the next character in coding string
    mov CH, BYTE[EAX] ; Character reading
    cmp CH, 'X'
    jne encodeLoop

encodeExit:
    inc EBX ; Move to the next character
    ret



; ===================== writeBarcodeRow ====================== ;

writeBarcodeRow:
; PROLOGUE
    push EBP
    mov EBP, ESP

    push EDI
    push ESI

	mov EDI, DWORD[EBP+8]
	mov ESI, DWORD[EBP+12]


; Procedure:
;   EDI - encoded_barcode
;   ESI - image_with_frame

    mov AH, 1 ; Initialization of the color (white)
    mov AL, 0 ; Initialization of the buffer for single bytes of the image
    mov CL, 8 ; Bit counter initialization

    mov CH, BYTE[EDI] ; Reading the number of color pixels
    cmp CH, 0
    je changeColor

writeByte:
    shl AL, 1
    add AL, AH ; Setting the last bit to a given color

    dec CL ; Bit counter decrement
    jnz writeByteContinuation

saveImageByte:
    mov BYTE[ESI], AL
    inc ESI
    mov AL, 0 ; Clearing the buffer for individual bytes of the image
    mov CL, 8 ; Bit counter setting

writeByteContinuation:
    dec CH
    jnz writeByte

changeColor:
    xor AH, 0b1 ; Last bit negation
    inc EDI ; Go to the opposite color
    mov CH, BYTE[EDI]
    cmp CH, 0
    jnz writeByte

writeBarcodeRow_exit:
; Correction and saving of the last, unprocessed byte:
    shl AL, 1
    add AL, 1
    dec CL
    jnz writeBarcodeRow_exit

    mov BYTE[ESI], AL

; EPILOGUE
    pop ESI
    pop EDI
	pop EBP
	ret
