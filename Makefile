target=httpd
obj=httpd.o
CC=gcc
LDFLAGS=-lpthread
$(target):$(obj)
	CC $(obj) -o $(target) 
%.o:%.c 
	CC -c $< -o $@

.PHONY:clean
clean:
	-rm -f *.o $(target)	
