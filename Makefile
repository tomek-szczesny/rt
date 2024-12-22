default: 
	gcc -Ofast -o rt rt.c -lm

install:
	install -p -s rt /usr/local/bin
	mkdir -p /var/lib/rt
	cp -r ./lists /var/lib/rt/

uninstall:
	rm -f /usr/local/bin/rt

purge:
	rm -f /usr/local/bin/rt
	rm -rf /var/lib/rt
