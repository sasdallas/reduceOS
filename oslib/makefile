include config.mk

all:
	make -C xlib all
	make -C libm all
	make -C libc all
	make -C libcons all
	make -C kl all

install:
	make -C xlib install
	make -C libm install
	make -C libc install
	make -C libcons install
	make -C kl install

clean:
	make -C xlib clean
	make -C libm clean
	make -C libc clean
	make -C libcons clean
	make -C kl clean
	make -C examples clean

allclean: clean
	$(RMDIR) lib
