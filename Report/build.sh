#!/bin/bash

# Compile the LaTeX document
pdflatex main.tex

# Run BibTeX to generate the bibliography
bibtex main

# Compile the LaTeX document again to include the bibliography
pdflatex main.tex

# Clean up temporary files
rm -f *.aux *.bbl *.blg *.log *.out *.toc

# Open the PDF file
xdg-open main.pdf
