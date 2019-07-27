
obfuscate_binary: 	obfuscate_binary.o
	cc -o obfuscate_binary obfuscate_binary.o

obfuscate_binary.o:	obfuscate_binary.c
	cc -c obfuscate_binary.c
