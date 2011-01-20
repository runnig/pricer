CC=gcc
CPP=g++
MAKE=make
AR=ar

CFLAGS=$(INCLUDES) -Wall -Werror -DLINUX -std=c++0x
CFLAGS_DBG = -g3 -Wall -DDEBUG
CFLAGS_REL = -O2 -Wall -DNDEBUG

TARGET_BINARY=pricer
OUTPUT_DIR=.
TARGET=$(OUTPUT_DIR)/$(TARGET_BINARY)

LDFLAGS+=-L$(BOOST)/lib


INCLUDES = -I. -I$(BOOST)
SRC_DIR = .

SRCS=$(SRC_DIR)/pricer.cpp

OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CPP) -o $(TARGET) $(OBJS) $(LDFLAGS)

.cpp.o:
	$(CPP) $(CFLAGS) -c $< -o$@

.c.o:
	$(CC) $(CFLAGS) -c $< -o$@


pricer_transform: pricer_transform.cpp
	$(CPP) $(CFLAGS) pricer_transform.cpp -o pricer_transform 

pricer.in.bin : pricer.in pricer_transform
	./pricer_transform pricer.in pricer.in.bin

.phony: transform
transform : pricer.in.bin

clean:
	rm -f $(OBJS) $(TARGET)

.phony: debug
debug: CFLAGS+=$(CFLAGS_DBG)
debug: build

.phony: release
release: CFLAGS+=$(CFLAGS_REL)
release: build

.phony: build
build: $(TARGET)

.phony: clean
	rm -f $(TARGET) $(OBJS)
