#!/bin/bash

# generate doxygen documentation
cd ../../
echo "Generating doxygen documentation..."
doxygen >doc/reference/refman.log
cd doc/reference


cd latex

# add bibliography to the end and remove index page
sed -i -e 's/\\end{document}/\\bibliographystyle{..\/IEEEtranLV}\n\\bibliography{..\/refman}\n\\end{document}/' -e '/\\input{index}$/d' -e '/\\chapter{Main Page}/d' refman.tex

# generate metapost file
echo "Generating aux files..."
pdflatex -interaction=batchmode refman.tex >>../refman.log

# process metapost file
echo "Processing metapost file..."
mpost refman.mp >>../refman.log

# generate bibliography
echo "Processing bibliography..."
bibtex refman >>../refman.log

# generate index
echo "Generating index..."
makeindex refman.idx >>../refman.log

# re-run latex to get cross-references right
for i in {1..3}
do
	echo "Re-running LaTeX..."
	pdflatex -interaction=batchmode refman.tex >>../refman.log
done

cd ..

echo "Done."
