# Variables
CC=gcc 
CFLAGS=-Wall -std=gnu99 -D_GNU_SOURCE
BINDIR=bin
EXEC_SRC_DIR=executable_src
SHELL_SRC_DIR=shell_src

# List of executables to build
EXECUTABLES=$(BINDIR)/pa2_head $(BINDIR)/pa2_tail $(BINDIR)/pa2_cat $(BINDIR)/pa2_cp $(BINDIR)/pa2_mv $(BINDIR)/pa2_rm $(BINDIR)/pa2

# shell codes
# SHELL=$(BINDIR)/pa2
OBJECTS=$(SHELL_SRC_DIR)/pa2.o $(SHELL_SRC_DIR)/parser.o $(SHELL_SRC_DIR)/built_in.o

# Default target
all: $(BINDIR) $(EXECUTABLES) bin/pa2

# Create bin directory if it doesn't exist
$(BINDIR):
	mkdir -p $(BINDIR)

# Rules for building executables
$(BINDIR)/pa2_head: $(EXEC_SRC_DIR)/pa2_head.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/pa2_tail: $(EXEC_SRC_DIR)/pa2_tail.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/pa2_cat: $(EXEC_SRC_DIR)/pa2_cat.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/pa2_cp: $(EXEC_SRC_DIR)/pa2_cp.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/pa2_mv: $(EXEC_SRC_DIR)/pa2_mv.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/pa2_rm: $(EXEC_SRC_DIR)/pa2_rm.c
	$(CC) $(CFLAGS) -o $@ $^

# Rules for building shell
bin/pa2: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

$(SHELL_SRC_DIR)/pa2.o: $(SHELL_SRC_DIR)/pa2.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(SHELL_SRC_DIR)/parser.o: $(SHELL_SRC_DIR)/parser.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(SHELL_SRC_DIR)/built_in.o: $(SHELL_SRC_DIR)/built_in.c
	$(CC) $(CFLAGS) -o $@ -c $^


# $(BINDIR)/pa2: $(SHELL_SRC_DIR)/pa2.c
# 	$(CC) $(CFLAGS) -o $@ $^ -lreadline

# $(SHELL): $(OBJECTS)
# 	$(CC) $(CFLAGS) -o $@ $^

# pa2.o: $(SHELL_SRC_DIR)/pa2.c
# 	$(CC) $(CFLAGS) -c $^
# $(BINDIR)/pa2: $(EXEC_SRC_DIR)/pa2.c
# 	$(CC) $(CFLAGS) -o $@ $^

# Clean rule to remove generated files
clean:
	rm -rf $(BINDIR) *.o ./shell_src/*.o

# Phony targets
.PHONY: all clean
