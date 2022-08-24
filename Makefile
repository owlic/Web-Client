

clean: web/
	rm -rf web/

main: main.c
	gcc -o main main.c -lssl -lcrypto

run:
	./main https://www.openfind.com.tw/taiwan/ web/ 5


