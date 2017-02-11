CXXFLAGS:=-std=gnu++14 -Wall -MMD -MP -Iext -Iext/PEGTL

all: test 

clean:
	rm -f *~ *.o *.d test

-include *.d

test: test.o 
	g++ $(CXXFLAGS) $^ -o $@


