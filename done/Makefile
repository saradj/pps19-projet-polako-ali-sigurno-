## ======================================================================
## partial Makefile provided to students
##

CFLAGS = -std=c11 -Wall -Wpedantic -g

 #a bit more checks if you'd like to (uncomment
 #CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
 -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
 -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
 CPPFLAGS += -DDEBUG

# ----------------------------------------------------------------------
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box
LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

all:: test-addr test-commands test-memory test-tlb_simple test-tlb_hrchy test-cache



addr_mng.o: addr_mng.c addr_mng.h addr.h error.h
commands.o: commands.c commands.h mem_access.h addr.h error.h addr_mng.h
error.o: error.c
list.o: list.c list.h error.h
memory.o: memory.c memory.h addr.h page_walk.h addr_mng.h util.h error.h
page_walk.o: page_walk.c page_walk.h addr.h addr_mng.h error.h
test-addr.o: test-addr.c tests.h error.h util.h addr.h addr_mng.h
test-commands.o: test-commands.c error.h commands.h mem_access.h addr.h
test-memory.o: test-memory.c error.h memory.h addr.h page_walk.h util.h \
 addr_mng.h
test-tlb_hrchy.o: test-tlb_hrchy.c error.h util.h addr_mng.h addr.h \
 commands.h mem_access.h memory.h tlb_hrchy.h tlb_hrchy_mng.h page_walk.h
test-tlb_simple.o: test-tlb_simple.c error.h util.h addr_mng.h addr.h commands.h mem_access.h memory.h list.h tlb.h tlb_mng.h
tlb_hrchy_mng.o: tlb_hrchy_mng.c tlb_hrchy.h tlb_mng.h tlb.h addr.h list.h addr_mng.h error.h \
 page_walk.h list.h
tlb_mng.o: tlb_mng.c tlb_mng.h tlb.h addr.h list.h addr_mng.h error.h \
 page_walk.h
 cache_mng.o: cache_mng.c cache_mng.h cache.h lru.h addr_mng.h addr.h error.h 
 test-cache.o: test-cache.c cache_mng.h cache.h lru.h addr_mng.h addr.h error.h page_walk.h commands.h memory.h

test-addr: test-addr.o error.o addr_mng.o
test-commands: test-commands.o error.o addr_mng.o commands.o 
test-memory: test-memory.o error.o memory.o page_walk.o addr_mng.o 
test-tlb_simple: test-tlb_simple.o error.o list.o addr_mng.o memory.o page_walk.o tlb_mng.o commands.o
test-tlb_hrchy: test-tlb_hrchy.o tlb_hrchy_mng.o error.o addr_mng.o commands.o memory.o page_walk.o list.o
test-cache: test-cache.o cache_mng.o error.o commands.o page_walk.o addr_mng.o memory.o
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
