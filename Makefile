BUILD_TYPE?=Release
BUILD_DIR?=build
BUILD_TESTS?=OFF
BUILD_CLI?=OFF

#
# Traditional build commands / make interface
#
default: make

.PHONY: debug
debug:
	$(eval BUILD_TYPE := Debug)

.PHONY: tests
tests:
	$(eval BUILD_TESTS := ON)

.PHONY: cli
cli:
	$(eval BUILD_CLI := ON)

.PHONY: cmake_check
cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

.PHONY: configure
configure: cmake_check
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DTESTS=$(BUILD_TESTS) -DCLI=$(BUILD_CLI) ../
	@echo "Modify build configuration in '$(BUILD_DIR)'"

.PHONY: make
make: configure
	cd $(BUILD_DIR) && make

.PHONY: install
install:
	cd $(BUILD_DIR) && make install

.PHONY: make-pkg
make-pkg: configure
	cd $(BUILD_DIR) && make package

.PHONY: install-pkg
install-pkg:
	sudo dpkg -i $(BUILD_DIR)/*.deb

.PHONY: uninstall-pkg
uninstall-pkg:
	sudo apt-get --yes remove liblightnvm || true

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true
	rm -f tags || true

all: clean default install

# Removes packages, cleans up, builds lib, cli, tests, pkg and installs
.PHONY: dev
dev: uninstall-pkg clean cli tests make-pkg install-pkg

.PHONY: tags
tags:
	ctags * -R .
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

#
# Experimental section
#
.PHONY: doxy
doxy:
	@mkdir -p $(BUILD_DIR)/doc/doxy
	doxygen doc/doxy.cfg

.PHONY: doxy-view
doxy-view:
	xdg-open $(BUILD_DIR)/doc/doxy/html/index.html

.PHONY: sphinx
sphinx:
	@mkdir -p $(BUILD_DIR)/doc/sphinx/html
	@mkdir -p $(BUILD_DIR)/doc/sphinx/pdf
	python doc/gen/gen.py capi --header include/liblightnvm.h -t doc/gen/capi.tmpl -r doc/src/c.rst
	python doc/gen/gen.py cli -j doc/gen/cli.json -t doc/gen/cli.tmpl -r doc/src/cli.rst
	sphinx-build -b html -c doc -E doc/src $(BUILD_DIR)/doc/sphinx/html
#	sphinx-build -b pdf -c doc -E doc/src $(BUILD_DIR)/doc/sphinx/pdf

.PHONY: sphinx-view
sphinx-view:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/index.html

.PHONY: doc
doc: doxy sphinx

.PHONY: doc-view-html
doc-view-html:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/index.html

#.PHONY: doc-view-pdf
#doc-view-pdf:
#	xdg-open $(BUILD_DIR)/doc/sphinx/pdf/liblightnvm.pdf

.PHONY: doc-publish
doc-publish:
	rm -rf $(BUILD_DIR)/ghpages
	mkdir -p $(BUILD_DIR)/ghpages
	git clone -b gh-pages git@github.com:OpenChannelSSD/liblightnvm.git --single-branch $(BUILD_DIR)/ghpages
	cd $(BUILD_DIR)/ghpages && git rm -rf --ignore-unmatch .
	cp -r $(BUILD_DIR)/doc/sphinx/html/. $(BUILD_DIR)/ghpages/
	touch $(BUILD_DIR)/ghpages/.nojekyll
	cd $(BUILD_DIR)/ghpages && git add .
	if [ -z "`git config user.name`" ]; then git config user.name "Mr. Robot"; fi
	if [ -z "`git config user.email`" ]; then git config user.email "foo@example.com"; fi
	cd $(BUILD_DIR)/ghpages && git commit -m "Autogen docs for `git rev-parse --short HEAD`."
	cd $(BUILD_DIR)/ghpages && git push origin --delete gh-pages
	cd $(BUILD_DIR)/ghpages && git push origin gh-pages

