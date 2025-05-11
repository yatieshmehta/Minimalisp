# Compiler and flags
CC = gcc
CFLAGS = -Wall -std=c99 -g -I.  # -g for debugging symbols, -I. for the current directory includes
LDFLAGS =  -ledit # Linker flags (if any)
OBJDIR = obj
OBJFILES = $(OBJDIR)/arena.o $(OBJDIR)/builtin.o $(OBJDIR)/eval.o $(OBJDIR)/lenv.o $(OBJDIR)/lval.o $(OBJDIR)/main.o $(OBJDIR)/mpc.o $(OBJDIR)/parser.o utils.o

# Executable
TARGET = minimalisp

# Source files
SRCS = builtin.c eval.c lenv.c lval.c main.c mpc.c parser.c arena.c utils.c
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# Create the object directory if it doesn't exist
$(OBJDIR):
	mkdir $(OBJDIR)

# Compile the object files
$(OBJDIR)/%.o: %.c $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Clean up generated files
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Run the program (optional)
run: $(TARGET)
	./$(TARGET) ./test.minlsp

# Default target
all: $(TARGET)

.PHONY: clean run all
