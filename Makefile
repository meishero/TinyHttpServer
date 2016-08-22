.PHONY:httpd	
httpd:httpd.c
	gcc -o httpd httpd.c -lpthread

.PHONY:clean
clean:
	rm -f httpd	
