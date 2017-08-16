CXXFLAGS:=-std=gnu++14 -Wall -MMD -MP -Iext -Iext/PEGTL

all: test prime

clean:
	rm -f *~ *.o *.d test

-include *.d

test: test.o 
	g++ $(CXXFLAGS) $^ -o $@

prime: prime.o 
	g++ $(CXXFLAGS) $^ -o $@

