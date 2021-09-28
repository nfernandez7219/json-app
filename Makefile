OBJS=json-app.o

json-app: $(OBJS)
	gcc -o $@ $^ -ljson-c

%.o: %.c
	gcc -c $< -o $@

.PHONY: run
run: json-app
	./json-app JSON1.txt

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf json-app
	rm -rf tags


