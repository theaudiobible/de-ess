de-ess: de-ess.o
	gcc -o de-ess de-ess.c -lm
	strip de-ess

install: de-ess
	sudo cp -v de-ess /usr/local/bin
	sudo chown root.root /usr/local/bin/de-ess
	sudo chmod 755 /usr/local/bin/de-ess
	sudo cp -v eq?.cfg /etc
	sudo chown root.root /etc/eq?.cfg

uninstall:
	sudo rm -f /usr/local/bin/de-ess
	sudo rm -f /etc/eq?.cfg

clean:
	rm -f de-ess *.o

