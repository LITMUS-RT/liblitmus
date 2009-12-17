.PHONY: all-32 all-64 all-sparc clean purge

all-32:
	echo "Legacy warning: Building is done with scons."
	ARCH=x86 scons
all-64:
	ARCH=x86_64 scons

all-sparc:
	ARCH=sparc64 scons

clean:
	echo "Legacy warning: Building is now done with scons."
	scons -c

purge: clean
	rm -rf .sconf_temp .sconsign.dblite
