CC=g++ -g -Wall -fno-builtin -std=c++17

# List of source files for your pager
PAGER_SOURCES=vm_pager.cpp vm_pager_class.cpp

# Generate the names of the pager's object files
PAGER_OBJS=${PAGER_SOURCES:.cpp=.o}

APPSOURCESS = $(wildcard test*.cpp)
APPS = $(APPSOURCESS:.cpp=_app)

all: pager app

# Compile the pager and tag this compilation
pager: ${PAGER_OBJS} libvm_pager.o
	./autotag.sh
	${CC} -DDEBUG -o $@ $^

test%_app:test%.cpp libvm_app.o
	${CC} -o $@ $^ -ldl

# Compile an application program
app: $(APPS)

test: tests/pager_class.cpp vm_pager_class.cpp vm_pager_class.h 
	${CC} -DDEBUG -o test_pager_class $^

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -DDEBUG -c $<
%.o: %.cc
	${CC} -DDEBUG -c $<

clean:
	rm -f ${PAGER_OBJS} pager test_pager_class test*_app
	rm -rf *.out *.log

clean_app:
	rm -f app
