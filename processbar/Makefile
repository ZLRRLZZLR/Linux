SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
BIN=processbar

$(BIN):$(OBJ)
	gcc -o $@ $^
%.o:%.c
	gcc -c $<

.PHONY:clean
clean:
	rm -rf $(OBJ) $(BIN)
