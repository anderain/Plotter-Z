#====================================================
# * Compiler Related Configuration
#----------------------------------------------------
# - CC			Compiler
# - C_FLAGS		Compilation flags
# - LDFLAGS		Linker flags
#====================================================
CC		  		= gcc
C_FLAGS	 		= -c -Wall -ansi
LD_FLAGS 		=
#----------------------------------------------------
COMMON_SRC_PATH = common
COMMON_OBJS		= vlist.o utils.o constants.o
#----------------------------------------------------
FZ_SRC_PATH		= formula-z
FZ_OBJS	 		= lexer.o parser.o
FZ_TEST_EXE 	= fz-test.exe
#----------------------------------------------------
EZ_SRC_PATH		= evaluator-z
EZ_OBJS			= emitter.o eval.o
EZ_TEST_EXE		= ez-test.exe
#----------------------------------------------------
RZ_SRC_PATH		= renderer-z
RZ_OBJS			= transform.o
RZ_TEST_EXE		= rz-test.exe

#====================================================
# * Target: Test Programs
#====================================================
all: fz-test ez-test rz-test

fz-test: $(FZ_OBJS) $(COMMON_OBJS) fz-test-main.o
	$(CC) $(LD_FLAGS) $(FZ_OBJS) $(COMMON_OBJS) fz-test-main.o -o $(FZ_TEST_EXE)

ez-test: $(EZ_OBJS) $(FZ_OBJS) $(COMMON_OBJS) ez-test-main.o
	$(CC) $(LD_FLAGS) $(EZ_OBJS) $(FZ_OBJS) $(COMMON_OBJS) ez-test-main.o -o $(EZ_TEST_EXE)

rz-test: $(RZ_OBJS) $(FZ_OBJS) $(COMMON_OBJS) rz-test-main.o
	$(CC) $(LD_FLAGS) $(RZ_OBJS) $(FZ_OBJS) $(COMMON_OBJS) rz-test-main.o -o $(RZ_TEST_EXE)

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
# * Target: Renderer-Z Files
#====================================================
transform.o: $(RZ_SRC_PATH)/transform.c $(RZ_SRC_PATH)/rz.h
	$(CC) $(C_FLAGS)  $(RZ_SRC_PATH)/transform.c -o transform.o

rz-test-main.o: $(RZ_SRC_PATH)/rz-test-main.c $(RZ_SRC_PATH)/rz.h
	$(CC) $(C_FLAGS) -DIS_TEST_PROGRAM $(RZ_SRC_PATH)/rz-test-main.c -o rz-test-main.o

#====================================================
# * Clean
#====================================================
.PHONY: clean
clean:
	rm *.o $(FZ_TEST_EXE) $(EZ_TEST_EXE)