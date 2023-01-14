all: man.ps

oslib.dvi: oslib.tex
	latex oslib
	latex oslib

man.ps: oslib.dvi
	dvips oslib -o man.ps

clean :
	rm -f *.log
	rm -f *~
	rm -f *.aux
	rm -f *.dvi
	rm -f man.ps
	rm -f *.ind
	rm -f *.idx
	rm -f *.ilg
	rm -f *.toc
