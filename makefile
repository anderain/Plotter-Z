#====================================================
# * Compiler Related Configuration
#----------------------------------------------------
# - CC			Compiler
# - C_FLAGS		Compilation flags
# - LDFLAGS		Linker flags
#====================================================
CC          = gcc
C_FLAGS     = -c -Wall -ansi
LD_FLAGS 	=

COMMON_SRC_PATH = common
COMMON_OBJS		= vlist.o utils.o constants.o

FZ_SRC_PATH		= formula-z
FZ_OBJS     	= lexer.o parser.o
FZ_TEST_EXE 	= fz-test.exe

EZ_SRC_PATH		= evaluator-z
EZ_OBJS			= emitter.o eval.o
EZ_TEST_EXE		= ez-test.exe

#====================================================
# * Target: Test Programs
#====================================================
fz-test: $(FZ_OBJS) $(COMMON_OBJS) fz-test-main.o
	$(CC) $(LD_FLAGS) $(FZ_OBJS) $(COMMON_OBJS) fz-test-main.o -o $(FZ_TEST_EXE)

ez-test: $(EZ_OBJS) $(FZ_OBJS) $(COMMON_OBJS) ez-test-main.o
	$(CC) $(LD_FLAGS) $(EZ_OBJS)  $(FZ_OBJS) $(COMMON_OBJS) ez-test-main.o -o $(EZ_TEST_EXE)

#====================================================
# * Target: Common Files
#====================================================
vlist.o: $(COMMON_SRC_PATH)/vlist.c $(COMMON_SRC_PATH)/vlist.h
	$(CC) $(C_FLAGS) $(COMMON_SRC_PATH)/vlist.c -o vlist.o

utils.o: $(COMMON_SRC_PATH)/utils.c $(COMMON_SRC_PATH)/utils.h
	$(CC) $(C_FLAGS) $(COMMON_SRC_PATH)/utils.c -o utils.o

constants.o: $(COMMON_SRC_PATH)/constants.c $(COMMON_SRC_PATH)/constants.h
	$(CC) $(C_FLAGS) $(COMMON_SRC_PATH)/constants.c -o constants.o

#====================================================
# * Target: Formula-Z Files
#====================================================
lexer.o: $(FZ_SRC_PATH)/lexer.c $(FZ_SRC_PATH)/fz.h
	$(CC) $(C_FLAGS)  $(FZ_SRC_PATH)/lexer.c -o lexer.o

parser.o: $(FZ_SRC_PATH)/parser.c $(FZ_SRC_PATH)/fz.h
	$(CC) $(C_FLAGS) $(FZ_SRC_PATH)/parser.c -o parser.o

fz-test-main.o: $(FZ_SRC_PATH)/fz-test-main.c $(FZ_SRC_PATH)/fz.h
	$(CC) $(C_FLAGS) -DIS_TEST_PROGRAM $(FZ_SRC_PATH)/fz-test-main.c -o fz-test-main.o

#====================================================
# * Target: Evaluator-Z Files
#====================================================

emitter.o: $(EZ_SRC_PATH)/emitter.c $(EZ_SRC_PATH)/ez.h
	$(CC) $(C_FLAGS)  $(EZ_SRC_PATH)/emitter.c -o emitter.o

eval.o: $(EZ_SRC_PATH)/eval.c $(EZ_SRC_PATH)/ez.h
	$(CC) $(C_FLAGS) $(EZ_SRC_PATH)/eval.c -o eval.o

ez-test-main.o: $(EZ_SRC_PATH)/ez-test-main.c $(EZ_SRC_PATH)/ez.h
	$(CC) $(C_FLAGS) -DIS_TEST_PROGRAM $(EZ_SRC_PATH)/ez-test-main.c -o ez-test-main.o

#====================================================
# * Clean
#====================================================
.PHONY: clean
clean:
	rm *.o $(FZ_TEST_EXE) $(EZ_TEST_EXE)