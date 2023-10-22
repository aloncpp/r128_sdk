
program: compile-objs
	xt-xcc reset_user_init.o ddr_init.o  main.c -g -O0  
	rm -rf dumpprog; xt-objdump -D a.out > dumpprog

compile-objs:
	xt-xcc -c reset_user_init.S
	xt-xcc -mlongcalls -c -mrename-section-.text=.iram0.text -mrename-section-.data=.dram0.data -mrename-section-.literal=.iram0.literal -mrename-section-.rodata=.dram0.rodata -mrename-section-.bss=.dram0.bss -mtext-section-literals ddr_init.c   

run: program
	rm -rf trace; xt-run --trace=5 a.out > trace

clean:
	rm -rf *.o a.out dump* trace*

