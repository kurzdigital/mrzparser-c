BIN = parser
OBJECTS = main.o
CFLAGS = -O2 -Wall -Wextra -pedantic -std=c99

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(BIN)
	./$(BIN) < samples

$(BIN): $(OBJECTS) mrzparser.h
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f *.o $(BIN)
