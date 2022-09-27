DOC=legion
SHELL=/bin/bash

# dependencies via include files
INCLUDED_TEX = installation.tex \
	       tasks.tex \
	       regions.tex \
	       partitioning.tex \
	       mapping.tex \
	       interop.tex \
               control_replication.tex \
               coherence.tex \
	       reference.tex
INCLUDED_FIGS =  

all: $(DOC).pdf 

$(DOC).pdf: $(DOC).tex bibliography.bib date_and_version.tex $(INCLUDED_TEX) $(INCLUDED_FIGS)


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

date_and_version.tex: get_date_and_version.py
	./get_date_and_version.py > date_and_version.tex

spelling :
	for f in *.tex; do aspell -p ./aspell.en.pws --repl=./aspell.en.prepl -c $$f; done

clean:
	rm -f *.bbl *.aux *.log *.blg *.lot *.lof *.toc *.dvi $(DOC).pdf *~
