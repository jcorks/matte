# default is GCC. Override CC when invoking make for something else.
CC:=gcc

with-extensions:
	$(CC) -std=c99 -g -DMATTE_USE_SYSTEM_EXTENSIONS -DMATTE_ROMLESS ../*.c -lm ./makerom.c  -o makerom
	
no-extensions:
	$(CC) -std=c99 -g -DMATTE_ROMLESS ../*.c -lm ./makerom.c  -o makerom
