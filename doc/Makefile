.PHONY: all doxygen pdf pdf-pandoc docx clean changelog dots puml copy-img \
        install-deps install-deps-html install-deps-pdf last-modified

# Часовой пояс для дат: PST оставлен как в исходном Makefile, чтобы метки
# совпадали с уже сгенерированными артефактами.
timestamp := $(shell TZ=PST git log -1 --remove-empty --no-merges --format='%cd' --date="format-local:%Y%m%d" .)

# Версия Espresso из файла VERSION в корне проекта подставляется в Doxyfile
# как $(ESP_VERSION).
export ESP_VERSION := $(shell tr -d '" \n' < ../VERSION)

# PlantUML: команда из пакета (apt install plantuml). Можно переопределить
# через `make puml PLANTUML='java -jar /path/to/plantuml.jar'`.
PLANTUML ?= plantuml


all: doxygen


# Полный набор зависимостей: и для HTML, и для PDF. Сохранён для
# обратной совместимости; ставит ~1ГБ texlive.
install-deps: install-deps-html install-deps-pdf

# Минимум для HTML-документации.
install-deps-html:
	sudo apt install -y doxygen doxygen-gui graphviz plantuml

# Дополнительные пакеты для сборки PDF / DOCX через XeLaTeX и pandoc.
install-deps-pdf:
	sudo apt install -y build-essential clang \
	                    pandoc pandoc-plantuml-filter \
	                    texlive-xetex \
	                    texlive-fonts-extra texlive-fonts-recommended \
	                    texlive-lang-cyrillic texlive-latex-extra \
	                    texlive-pstricks texlive-science \
	                    fonts-dejavu fonts-cmu


# Старый вывод удаляется до того, как картинки из dot, puml и md/images
# кладутся в html/images: иначе rm -rf html удалил бы их до запуска Doxygen.
doxygen:
	@rm -rf html
	$(MAKE) dots puml copy-img
	doxygen

pdf-pandoc:	changelog dots puml
	pandoc -F pandoc-plantuml --resource-path=md -f markdown+smart md/*.md -o espresso-$(timestamp).pdf

# Doxygen готовит latex/ (графы — в PDF, USE_PDFLATEX = YES), затем XeLaTeX
# собирает refman.pdf.
pdf: changelog last-modified
	@rm -rf html latex
	$(MAKE) dots puml copy-img
	doxygen
	cp tex/espresso.sty latex/
	cd latex && $(MAKE) LATEX_CMD=xelatex && mv refman.pdf ../espresso-$(timestamp).pdf

docx:	changelog dots puml
	pandoc -F pandoc-plantuml --resource-path=md -f markdown+smart md/*.md -o espresso-$(timestamp).docx

last-modified:
	$(shell env TZ=Europe/Moscow git log -1 --remove-empty --no-merges --format='%cd' --date="format:%B %d %Y" . > last-modified.tex)

dots:
	if [ ! -d html/images ]; then \
		mkdir -p html/images; \
	fi
	for i in `find dot -name '*.dot'`; \
	do \
		j=html/images/`basename $$i`.png; \
		dot -Tpng "$$i" -o$$j; \
	done

puml:
	if [ ! -d html/images ]; then \
		mkdir -p html/images; \
	fi
	for i in `find puml -name '*.puml' 2>/dev/null`; \
	do \
		$(PLANTUML) -tpng "$$i" -o../html/images; \
	done

copy-img:
	mkdir -p html/images
	for i in `find md/images -type f 2>/dev/null`; \
	do \
		cp "$$i" html/images/; \
	done

# Страница с историей изменений каталога doc по журналу git. Её читают и
# Doxygen, и pandoc, поэтому в ней только общая для них разметка. Знаки,
# особые для одного из них (\ @ & $ # < > % |), экранируются обратной
# косой чертой: иначе, например, «@brief» в описании коммита Doxygen
# принял бы за команду.
changelog:
	echo '# История изменений документации {#changelog}' > md/01-changelog.md
	echo >> md/01-changelog.md
	echo '| Дата | Автор | Изменение |' >> md/01-changelog.md
	echo '|---|---|---|' >> md/01-changelog.md
	git log --no-merges --date-order --format='%cd%x09%cN%x09%s' \
	        --date='format-local:%Y-%m-%d %H:%M' . \
	    | sed -e 's/[\\@&$$#<>%|]/\\&/g' -e 's/\t/ | /g' -e 's/^/| /' -e 's/$$/ |/' \
	    >> md/01-changelog.md

clean:
	rm -rf html latex plantuml-images *.pdf *.epub *.docx md/01-changelog.md *.log tex/*.aux tex/*.fdb_latexmk tex/*.fls tex/*.log symbols.sql last-modified.tex doxy.log Doxyfile.bak *.aux
