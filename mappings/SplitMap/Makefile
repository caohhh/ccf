# Compiler
CC = g++
# Compiler flags
CFLAGS = -std=c++11 -Wall -static -O3

# Directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(SRCDIR)/Arc.h $(SRCDIR)/CGRA.h $(SRCDIR)/definitions.h $(SRCDIR)/DFG.h $(SRCDIR)/Mapper.h $(SRCDIR)/Node.h $(SRCDIR)/Parser.h $(SRCDIR)/Schedule.h

# Output executable
TARGET = $(BINDIR)/SplitMap

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
