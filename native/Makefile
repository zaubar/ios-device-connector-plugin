all: list

#list: list.c
#	clang -o list -framework MobileDevice -framework CoreFoundation -F/Library/Apple/System/Library/PrivateFrameworks  list.c

utils.o: utils.c utils.h
	clang -c utils.c 

list: list.c utils.o 
	clang -o list -lplist-2.0 -limobiledevice-1.0 -framework MobileDevice -framework CoreFoundation -F/Library/Apple/System/Library/PrivateFrameworks list.c utils.o

install:
	cp list ../src/main/resources/org/jenkinsci/plugins/ios/connector/list

clean:
	rm list


