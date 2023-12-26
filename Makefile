# Makefile
.SUFFIXES : .c .o


## FLAGS ##
CC := gcc
CFLAGS := -g -Wall -pg 
LIBS := -lm -lpthread

## FILES ##
SRCS := server.c client.c
OBJS := $(SRCS:%.c=%.o) 

TARGET := server client
 
RM = rm -rf

## RULES
all:
	$(MAKE) $(TARGET)

server: server.c
	$(info $<)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

client: client.c
	$(info $<)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)
	
clean:
	$(RM) $(OBJS) $(TARGET) 

new : 
	$(MAKE) clean 
	$(MAKE) $(TARGET)