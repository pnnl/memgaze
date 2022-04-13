# -*-Mode: makefile;-*-

MEMGAZE := $(shell pwd)

BUILD_DIR = lib-externals
SPACK := $(MEMGAZE)/utils/spack/bin/spack

setup:
	mkdir -p utils
	cd utils && \
	git clone -c feature.manyFiles=true https://github.com/spack/spack.git
	git clone https://github.com/hpctoolkit/hpctoolkit.git

dyninst:

hpctoolkit:

build:  dyninst
	cd utils;  \
	cp ../config.yaml spack/etc/spack/ && \
	cp hpctoolkit/spack/packages.yaml  spack/etc/spack/ && \
	ARCH="$(shell $(SPACK) arch)"; \
	$(SPACK) install --reuse  hpctoolkit@2022.01.15 -papi -mpi

all: setup build


#TAR = tar
#TARNM = perfect-suite-1.0.1
#
#
#
#dist :
#nm_cur=`basename ${PWD}` ; \
#nm_new=$(TARNM) ; \
#cd .. ; \
#if [[ ! -e $${nm_new} ]] ; then ln -s $${nm_cur} $${nm_new} ; fi ; \
#${TAR} zcvf $${nm_new}.tar.gz \
#-h \
#--exclude=".git" \
#--exclude=".svn" \
#--exclude="suite/sar/tools" \
#--exclude="suite/wami/tools" \
#--exclude="doc/src" \
#$${nm_new}
