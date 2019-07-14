DIR_FONTS = ../Fonts
DIR_OBJ = ../obj
DIR_BIN = ../bin

OBJ_C = $(wildcard ${DIR_FONTS}/*.c ${DIR_OBJ}/*.c)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = epd
#BIN_TARGET = ${DIR_BIN}/${TARGET}

#CC = gcc
CC = c99
MSG = -g -O0 -Wall
DEBUG = -D USE_DEBUG
# DEBUG = 
CFLAGS += $(MSG) $(DEBUG)

LIB = -lbcm2835 -lm

${TARGET}:${OBJ_O} main.o
	$(CC) $(CFLAGS) $(OBJ_O) main.o -o $@ $(LIB)

${DIR_BIN}/%.o : $(DIR_OBJ)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ $(LIB)

${DIR_BIN}/%.o:$(DIR_FONTS)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ 

.c.o: %c $
	$(CC) $(CFLAGS) -c -o $@ $<	
clean :
	rm $(DIR_BIN)/*.* 
	rm $(TARGET) 
