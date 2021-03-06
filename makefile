ERR = $(shell which icc >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
	CC=icc
else
	CC=gcc
endif
CFLAGS=-std=c99 -Wall
PROGRAM=myar
SOURCES=myar.c file_stat.c
INCLUDES=file_stat.h

SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)
EXTRA = $(TRG:%.dvi=%.aux) $(TRG:%.dvi=%.aux) $(TRG:%.dvi=%.out) $(TRG:%.dvi=%.log)

default: $(PROGRAM)

debug: $(PROGRAM)

debug: CFLAGS += -g -DDEBUG

all: pdf $(PROGRAM)

pdf: $(PDF)

ps: $(PSF)

clean:
	rm -f $(TRG) $(PSF) $(PDF) $(EXTRA)
	rm -f $(PROGRAM)

$(TRG): %.dvi : %.tex
	latex $<
	latex $<

$(PSF): %.ps : %.dvi
	dvips -R -Poutline -t letter $< -o $@

$(PDF): %.pdf : %.ps
	ps2pdf $<

$(PROGRAM): $(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
