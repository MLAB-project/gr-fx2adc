INCLUDES = -Iinclude
CC = sdcc -mmcs51
BASENAME = fx2adc

all: $(BASENAME).ihx

$(BASENAME).ihx: $(BASENAME).c
	$(CC) $(INCLUDES) $(BASENAME).c -o $@

clean:
	rm -f *.asm  *.ihx  *.lnk *.lst *.map *.mem *.rel *.rst *.sym *.lk
