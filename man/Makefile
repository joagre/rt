# man/Makefile - Install and uninstall man pages
#
# Can be run standalone or from top-level Makefile.

# Installation directories
PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man

# Man page files
MAN3_PAGES := $(wildcard man3/*.3)

# Default target
.PHONY: all
all:
	@echo "Man pages are documentation only, no build needed."
	@echo "Use 'make install' to install or 'make view-<page>' to view."

# Install man pages
.PHONY: install
install:
	@echo "Installing man pages to $(MANPREFIX)/man3/"
	install -d $(MANPREFIX)/man3
	install -m 644 man3/*.3 $(MANPREFIX)/man3/

# Uninstall man pages
.PHONY: uninstall
uninstall:
	@echo "Removing man pages from $(MANPREFIX)/man3/"
	rm -f $(MANPREFIX)/man3/hive_*.3

# View a man page without installing
.PHONY: view-%
view-%:
	@man man3/$*.3 2>/dev/null || man man3/hive_$*.3

# List available man pages
.PHONY: list
list:
	@echo "Available man pages:"
	@for f in man3/*.3; do basename "$$f" .3; done

.PHONY: help
help:
	@echo "man/Makefile targets:"
	@echo "  install   - Install man pages to $(MANPREFIX)/man3"
	@echo "  uninstall - Remove installed man pages"
	@echo "  list      - List available man pages"
	@echo "  view-<page> - View a man page (e.g., view-hive_ipc)"
