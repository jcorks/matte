#This is essentially just a script to make compiling easier

# For CC on unix likes and MSYS2 on windows
CC:= gcc
CC_MAKE_OBJECT_FILES:= -c 
RELEASE_CC_FLAGS:= -O2  -std=c99
DEBUG_CC_FLAGS:= -g  -std=c99
LIBRARY_NAME:=libmatte.a
MAKE_LIBRARY:= ar rcs $(LIBRARY_NAME) *.o
MAKE_CLEAN:= rm ./*.o ./src/MATTE_ROM ./libmatte.a




# operating system differences for linking libraries
# these can be removed if no extentions are used.
LIBS:= -lm

#For windows
#LIBS+=

#For Linux
LIBS+= -lpthread -lssl -lcrypto







# Flags for compiling system-based binaries on unix-likes
DEFINES= -D_GNU_SOURCE -D_XOPEN_SOURCE=500

# Flag for compilation of system extentions
DEFINES+= -DMATTE_USE_SYSTEM_EXTENSIONS

# Flag for compilation inclusion of basic system extensions
DEFINES+= -DMATTE_USE_SYSTEM_EXTENSIONS__BASIC

# Flag for compilation of all system extensions.
DEFINES+= -DMATTE_USE_SYSTEM_EXTENSIONS__ALL



# These flags are for debugging the VM and compiler itself

# Normal debugging
DEFINES+= -DMATTE_DEBUG

# Deep debugging for the VM and store modules
#DEFINES+= -DMATTE_DEBUG__COMPILER -DMATTE_DEBUG__STORE

#Even deeper debugging of the store module
#DEFINES+= -DMATTE_DEBUG_STORE__LEVEL_2

all:
	cd ./src/rom/ && make CC="$(CC)" DEFINES="$(DEFINES)"
	cd ./src/rom/ && ./makerom
	$(CC) $(CC_MAKE_OBJECT_FILES) $(RELEASE_CC_FLAGS) ./src/*.c ./src/rom/native.c $(DEFINES)
	$(MAKE_LIBRARY)
	cd ./cli/ && make FLAGS="$(RELEASE_CC_FLAGS)" CC="$(CC)" DEFINES="$(DEFINES)" LIBS="$(LIBS)"

	@echo ""
	@echo ""
	@echo ""
	@echo "Done! The following were created:"
	@echo ""
	@echo "src/MATTE_ROM -- ROM including built-in modules"
	@echo "cli/matte -- General purpose interpreter"
	@echo "$(LIBRARY_NAME) -- Static library including compiler and virtual machine"
	@echo ""
	@echo "See /samples/embedding for embedding examples and /samples/matte for Matte code samples to run in the interpreter."
	@echo ""
	@echo ""

debug:
	cd ./src/rom/ && make CC="$(CC)" DEFINES="$(DEFINES)"
	cd ./src/rom/ && ./makerom
	$(CC) $(CC_MAKE_OBJECT_FILES) $(DEBUG_CC_FLAGS) ./src/*.c ./src/rom/native.c $(DEFINES)
	$(MAKE_LIBRARY)
	cd ./cli/ && make FLAGS="$(DEBUG_CC_FLAGS)" CC="$(CC)" DEFINES="$(DEFINES)" LIBS="$(LIBS)"

	@echo ""
	@echo ""
	@echo ""
	@echo "Done! The following were created:"
	@echo ""
	@echo "src/MATTE_ROM -- ROM including built-in modules. If youre using naked sources and dropping them in your project, include this with the sources!"
	@echo "cli/matte -- General purpose interpreter"
	@echo "$(LIBRARY_NAME) -- Static library including compiler and virtual machine"
	@echo ""
	@echo "See /samples/embedding for embedding examples and /samples/matte for Matte code samples to run in the interpreter."
	@echo ""
	@echo ""

clean:
	$(MAKE_CLEAN)
