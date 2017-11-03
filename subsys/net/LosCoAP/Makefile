allobjs = main.o coap_core.o coap_os_unix.o

coap_client : $(allobjs)
	cc -o coap_client $(allobjs) -g

main.o : ./test/main.c ./include/coap_core.h ./include/los_coap.h 
	cc -c ./test/main.c -g

coap_core.o : ./src/coap_core.c ./include/coap_core.h ./include/los_coap.h ./include/los_unix.h 
	cc -c ./src/coap_core.c -g

coap_os_unix.o : ./src/coap_os_unix.c ./include/coap_core.h ./include/los_coap.h ./include/los_unix.h 
	cc -c ./src/coap_os_unix.c -g
	
clean :
	rm coap_client $(allobjs)