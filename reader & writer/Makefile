CC:=gcc

obj : ssu_vim.o ssu_ofm.o
	$(CC) -o ssu_vim ssu_vim.o
	$(CC) -o ssu_ofm ssu_ofm.o

ssu_vim.o :	ssu_vim.c
	$(CC) -c ssu_vim.c

ssu_ofm.o :	ssu_ofm.c
	$(CC) -c ssu_ofm.c

clean :
	rm ssu_vim.o
	rm ssu_ofm.o
	rm ssu_vim
	rm ssu_ofm
