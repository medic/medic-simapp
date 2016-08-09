
all: tests
	(cd src && make all)

clean: clean-tests clean-output
	(cd src && make clean)

clean-output:
	rm -f output/forms/*.c output/main/main.c

tests: all-tests

all-tests:
	for dir in tests/*; do \
        (cd "$$dir" && make) \
    done

clean-tests:
	for dir in tests/*; do \
        (cd "$$dir" && make clean) \
    done

