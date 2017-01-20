DOC=legion
SHELL=/bin/bash

# dependencies via include files
INCLUDED_TEX = 
INCLUDED_FIGS =  

all: $(DOC).pdf 

$(DOC).pdf: $(DOC).tex bibliography.bib $(INCLUDED_TEX) $(INCLUDED_FIGS)


%.pdf: %.tex bibliography.bib
	pdflatex -halt-on-error $*.tex
	(if grep -q bibliography $*.tex; \
	then \
		bibtex $*; \
		pdflatex -halt-on-error $*.tex; \
	fi)
	pdflatex -halt-on-error $*.tex

%.pdf: %.tex
	pdflatex -halt-on-error $*.tex
	pdflatex -halt-on-error $*.tex

spelling :
	for f in *.tex; do aspell -p ./aspell.en.pws --repl=./aspell.en.prepl -c $$f; done

clean:
	rm -f *.bbl *.aux *.log *.blg *.lot *.lof *.toc *.dvi $(DOC).pdf
