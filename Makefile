OBJS=json-app.o

json-app: $(OBJS)
	gcc -o $@ -ljson-c $^

%.o: %.c
	gcc -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf json-app
	rm -rf tags


