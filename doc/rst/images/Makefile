ODGS = $(wildcard *.odg)
PDFS = $(ODGS:.odg=.pdf)

all: $(PDFS)
	@echo PDFs created

_uncropped:
	mkdir _uncropped

_uncropped/%.pdf: %.odg | _uncropped
	soffice -env:UserInstallation=file:///home/$(USER)/.libreoffice-alt --headless --convert-to pdf $< --outdir _uncropped

%.pdf: _uncropped/%.pdf
	pdfcrop $< $@

wavedrom:
	WaveDromEditor source transaction_start.js png transaction_start.png
	WaveDromEditor source read_byte.js png read_byte.png
	WaveDromEditor source write_byte.js png write_byte.png
	WaveDromEditor source stop_bit.js png stop_bit.png

clean:
	-rm $(PDFS)
	-rm _uncropped/*.pdf
	-rmdir _uncropped
