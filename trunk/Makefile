
all: build/Makefile build/ezTunnelSSH


build/Makefile:
	( cd build; qmake )

build/ezTunnelSSH:
	( cd build; make -f Makefile.Release )

clean:
	rm -rf build/debug build/release build/GeneratedFiles build/ezTunnelSSH build/ezTunnelSSH.ini build/Makefile*
