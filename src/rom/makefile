#defines are from the root makefile. look there for more details


all:
	$(CC) $(DEFINES) -DMATTE_ROMLESS ../*.c -lm ./makerom.c  -o makerom

debug:
	$(CC) -D_GNU_SOURCE -D_XOPEN_SOURCE=500 -DMATTE_USE_SYSTEM_EXTENSIONS -DMATTE_USE_SYSTEM_EXTENSIONS__BASIC -DMATTE_USE_SYSTEM_EXTENSIONS__ALL -DMATTE_DEBUG -DMATTE_ROMLESS -g -O2 ../*.c -lm ./makerom.c -o makerom

