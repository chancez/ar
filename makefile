ERR = $(shell which icc >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
	CC=icc
else
	CC=gcc
endif
CFLAGS=
ALL_CFLAGS=-std=c99 $(CFLAGS)

SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)
EXTRA = $(TRG:%.dvi=%.aux) $(TRG:%.dvi=%.aux) $(TRG:%.dvi=%.out) $(TRG:%.dvi=%.log)


default: all

all: pdf

pdf: $(PDF)

ps: $(PSF)

clean:
	rm -f $(TRG) $(PSF) $(PDF) $(EXTRA)
	rm -f sieve.o

$(TRG): %.dvi : %.tex
	latex $<
	latex $<

$(PSF): %.ps : %.dvi
	dvips -R -Poutline -t letter $< -o $@

$(PDF): %.pdf : %.ps
	ps2pdf $<

