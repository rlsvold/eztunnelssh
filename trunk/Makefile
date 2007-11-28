
all: build/Makefile build/ezTunnelSSH_


build/Makefile:
	( cd build; qmake )

build/ezTunnelSSH_:
	( cd build; make -f Makefile.Release )

clean:
	rm -rf build/debug build/release build/GeneratedFiles build/ezTunnelSSH build/ezTunnelSSH.ini build/Makefile*
