include ../make.config

ifeq ($(HOSTARCH), i386)
# Required for I386
PROJECTS :

# ACPICA
ifeq ($(USE_ACPICA), 1)
PROJECTS += acpica
endif

else
PROJECTS :=
endif

# i love make i love make cmake is bad i love make i love make
PROJECTS_HEADERS = $(addsuffix /installheaders, $(PROJECTS))
PROJECTS_INSTALL = $(addsuffix /install, $(PROJECTS))
<<<<<<< HEAD
PROJECTS_CLEAN   = $(addsuffix /clean, $(PROJECTS))
=======
>>>>>>> d4e9572 (acpica/build: Implement external project build system (this will need merge to master))

# Target disabled
all:
	@echo "=== TARGET DISABLED ==="
	@echo "Please use make install"

$(PROJECTS_HEADERS):
	make -C $(dir $@) install-headers

$(PROJECTS_INSTALL):
	make -C $(dir $@) install

<<<<<<< HEAD
$(PROJECTS_CLEAN):
	make -C $(dir $@) clean

=======
>>>>>>> d4e9572 (acpica/build: Implement external project build system (this will need merge to master))
# Because I could care less each external project should at least have an
# install-headers dummy target. Suck it.
install-headers: $(PROJECTS_HEADERS)
	@echo "[EXTERNAL] Finished headers: $(PROJECTS)"
install: $(PROJECTS_INSTALL)
<<<<<<< HEAD
	@echo "[EXTERNAL] Finished compiling: $(PROJECTS)"
clean: $(PROJECTS_CLEAN)
=======
	@echo "[EXTERNAL] Finished compiling: $(PROJECTS)"
>>>>>>> d4e9572 (acpica/build: Implement external project build system (this will need merge to master))
