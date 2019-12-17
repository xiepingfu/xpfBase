CC = gcc -g

BINS 		:= debuger xpfBase
SRCS 		:= $(wildcard *.c)
OBJS 		:= $(SRCS:.c=.o)
BINOS 		:= $(addsuffix .o, $(BINS))
TEMP_OBJ 	= $(filter-out $(BINOS), $^)

.PHONY: all clean

all:$(BINS)

$(BINS):$(OBJS)
	$(foreach BIN, $@, $(CC) $(TEMP_OBJ) $(BIN).o  -o $(BIN))

%.o:%.c
	$(CC) -c $< -o $@

clean:
	-rm -rf $(OBJS) $(BINS)
