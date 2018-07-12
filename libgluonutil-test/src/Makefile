all: libgluonutil-test

libgluonutil-test: libgluonutil-test.c
	$(CC) ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -o $@ $^ $(LDLIBS) -lgluonutil -lubus -lubox

clean:
	rm libgluonutil-test
