## ======================================================================
## partial Makefile provided to students
##

CFLAGS = -std=c11 -Wall -Wpedantic -g -lcheck -lm -lrt -pthread -lsubunit

# a bit more checks if you'd like to (uncomment
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
#CPPFLAGS += -DDEBUG

# ----------------------------------------------------------------------
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box
LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

all:: test-memory

addr.o: addr.h
<<<<<<< HEAD
addr_mng.o: addr_mng.c addr_mng.h addr.h error.h
=======
addr_mng.o: addr_mng.h addr.h error.h addr_mng.c
>>>>>>> 3dd57ed59575794473e08dca5ff2eddfb93e5b59
cache.o: cache.h
cache_mng.o: cache_mng.h mem_access.h addr.h cache.h
commands.o: commands.c commands.h mem_access.h addr.h addr_mng.h error.h
error.o: error.h error.c 
<<<<<<< HEAD
list.o: list.h
MainTest.o: MainTest.c error.h commands.h mem_access.h addr.h addr_mng.h
=======
>>>>>>> 3dd57ed59575794473e08dca5ff2eddfb93e5b59
mem_access.o: mem_access.h
memory.o: memory.h addr.h page_walk.h addr_mng.h util.h error.h memory.c
page_walk.o: page_walk.c page_walk.h addr.h addr_mng.h
test-addr.o: test-addr.c tests.h error.h util.h addr.h addr_mng.h
test-commands.o: test-commands.c error.h commands.h mem_access.h addr.h
test-memory.o: test-memory.c error.h memory.h addr.h page_walk.h util.h addr_mng.h
tests.o: tests.h error.h
<<<<<<< HEAD
tlb.o: tlb.h addr.h
tlb_hrchy.o: tlb_hrchy.h addr.h
tlb_hrchy_mng.o: tlb_hrchy_mng.h tlb_hrchy.h addr.h mem_access.h
tlb_mng.o: tlb_mng.h tlb.h addr.h list.h
util.o: util.h


##test-addr:: addr_mng.o test-addr.o 
##commands:: addr_mng.o commands.o MainTest.o
test-memory:: error.o commands.o addr_mng.o page_walk.o memory.o test-memory.o
=======

##test-addr:: addr_mng.o test-addr.o 
##commands:: addr_mng.o commands.o 
test-commands:: error.o commands.o addr_mng.o test-commands.o
test-memory: error.o memory.o addr.o addr_mng.o page_walk.o test-memory.o
>>>>>>> 3dd57ed59575794473e08dca5ff2eddfb93e5b59



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

IMAGE=arashpz/feedback
feedback:
	@docker run -it --rm -v ${PWD}:/home/tester/done $(IMAGE)

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'
