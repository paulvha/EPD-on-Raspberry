# make file for epaper
# version 1.0 paulvha

DIR_FONTS = ./Fonts
DIR_OBJ = ./obj
DIR_BIN = ./bin

OBJ_C = $(wildcard ${DIR_FONTS}/*.c ${DIR_OBJ}/*.c epaper.c)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = epaper
CC = c99
MSG = -g -O0 -Wall

CFLAGS += $(MSG)
LIB = -lbcm2835 -lm

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $@ $(LIB)

${DIR_BIN}/%.o : $(DIR_OBJ)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ $(LIB)

${DIR_BIN}/%.o:$(DIR_FONTS)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ 
	
${DIR_BIN}/%.o:%.c
	$(CC) $(CFLAGS) -c  $< -o $@ 
	
clean :
	rm $(DIR_BIN)/*.* 
	rm $(TARGET) 
