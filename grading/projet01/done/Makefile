## ======================================================================
## partial Makefile provided to students
##

CFLAGS = -std=c11 -Wall -Wpedantic -g

# a bit more checks if you'd like to (uncomment
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
# CPPFLAGS += -DDEBUG

# ----------------------------------------------------------------------
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box
LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

all:: test-memory

addr_mng.o: error.h addr.h addr_mng.h addr_mng.c
commands.o: commands.h mem_access.h addr.h addr_mng.h error.h commands.c 
error.o: error.h error.c
memory.o: memory.h addr.h page_walk.h addr_mng.h util.h error.h memory.c
page_walk.o: addr.h addr_mng.h error.h page_walk.c 
test-addr.o: test-addr.c tests.h error.h util.h addr.h addr_mng.h
test-commands.o: test-commands.c error.h commands.h mem_access.h addr.h
test-memory.o: test-memory.c error.h memory.h addr.h page_walk.h util.h addr_mng.h


test-addr: test-addr.o error.o addr_mng.o
test-commands: test-commands.o error.o addr_mng.o commands.o 
test-memory: test-memory.o error.o memory.o page_walk.o addr_mng.o 


# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.

clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS)

new: clean all

static-check:
	scan-build -analyze-headers --status-bugs -maxloop 64 make CC=clang new

style:
	astyle -n -o -A8 *.[ch]

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

check:: $(CHECK_TARGETS)
	$(foreach target,$(CHECK_TARGETS),./$(target);)

# target to run tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
	  for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
	fi

IMAGE=arashpz/feedback:latest
feedback:
	@docker pull $(IMAGE)
	@docker run -it --rm -v ${PWD}:/home/tester/done $(IMAGE)

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'
