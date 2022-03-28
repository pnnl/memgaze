#!/bin/bash
set -x
mkdir toolkit
cd toolkit
git clone -c feature.manyFiles=true https://github.com/spack/spack.git
mv spack/etc/spack/defaults/config.yaml spack/etc/spack/defaults/org_config.yaml
cp ../config.yaml spack/etc/spack/defaults/config.yaml
#git clone https://github.com/hpctoolkit/hpctoolkit.git
cd spack/bin
ARCH=$(./spack arch)
#./spack install hpctoolkit@2022.01.15
./spack install hpctoolkit@2022.01.15 -papi -mpi
cd ../..
echo `pwd`
for folder in `pwd`/${ARCH}/*
do
   echo $folder
   LIB_PATH=$folder
done
for folder in ${LIB_PATH}/*
do
   if [[ $folder == ${LIB_PATH}/hpctoolkit* ]]
   then 
      HPCTOOLKIT=${folder}
   elif [[ $folder == ${LIB_PATH}/dyninst* ]]
   then
      DYNINST=${folder}
   elif [[ $folder == ${LIB_PATH}/binutils* ]]
   then
      BINUTILS=${folder}
   elif [[ $folder == ${LIB_PATH}/intel-xed* ]]
   then
      XED=${folder}
   elif [[ $folder == ${LIB_PATH}/boost* ]]
   then
      BOOST=${folder}
   elif [[ $folder == ${LIB_PATH}/intel-tbb* ]]
   then
      TBB=${folder}
   fi
done
echo "${HPCTOOLKIT}/"
echo "${DYNINST}/"
export PALM_EXT_ROOT=${LIB_PATH}
export PALM_EXT_HPCTKEXT_ROOT=${HPCTOOLKIT}
export DYNINST_ROOT=${DYNINST}
export DYNINSTAPI_RT_LIB=${DYNINST}/lib/libdyninstAPI_RT.so
export XED_ROOT=${XED}
export BINUTILS_ROOT=${BINUTILS}
export BOOST_ROOT=${BOOST}
export TBB_ROOT=${TBB}
cd ../bin-anlys
make -j4 


#Build Notes

# NOTE DYNINST SPACK build doesnt have following:
#load_module.C:1146:11: error: ‘class BPatch’ has no member named ‘setRelocateJumpTable’
#    bpatch.setRelocateJumpTable(true);
#TODO FIXME I commet out that line for now
#I will first trry to patch or I will build dyninst from .tar.gz file

#src/Scheduler/Makefile
#36 DYNINST_CXXFLAGS = \
# 37         -I$(DYNINST_INC) \
#  38         -I$(BOOST_INC)
#   39         #-I$(TBB_INC) \ may need to add this 

#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:168:9: error: ‘bfd_get_section_flags’ was not declared in this scope
#    if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
#         ^~~~~~~~~~~~~~~~~~~~~
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:168:9: note: suggested alternative: ‘bfd_set_section_flags’
#    if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
#         ^~~~~~~~~~~~~~~~~~~~~
#         bfd_set_section_flags
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:175:10: error: ‘bfd_get_section_vma’ was not declared in this scope
#    vma = bfd_get_section_vma (abfd, section);
#          ^~~~~~~~~~~~~~~~~~~
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:175:10: note: suggested alternative: ‘bfd_set_section_vma’
#    vma = bfd_get_section_vma (abfd, section);
#          ^~~~~~~~~~~~~~~~~~~
#          bfd_set_section_vma
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:176:11: error: ‘bfd_get_section_size’ was not declared in this scope
#    size = bfd_get_section_size (section);
#           ^~~~~~~~~~~~~~~~~~~~
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:176:11: note: suggested alternative: ‘bfd_set_section_size’
#    size = bfd_get_section_size (section);
#           ^~~~~~~~~~~~~~~~~~~~
#           bfd_set_section_size
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C: In function ‘int MIAMIP::get_source_file_location(void*, MIAMI::addrtype, int32_t*, int32_t*, std::__cxx11::string*, std::__cxx11::string*)’:
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:217:19: error: ‘bfd_get_section_vma’ was not declared in this scope
#    addrtype vma = bfd_get_section_vma (info->abfd, section);
#                   ^~~~~~~~~~~~~~~~~~~
#/files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:217:19: note: suggested alternative: ‘bfd_set_section_vma’
#    addrtype vma = bfd_get_section_vma (info->abfd, section);
#                   ^~~~~~~~~~~~~~~~~~~
#                   bfd_set_section_vma

#src/common/InstructionDecoder-xed-iclass.h
#25 //FIXME:NEWBUILD    case XED_ICLASS_PFCPIT1:            // 3DNOW
#38 //FIXME:NEWBUILD    case XED_ICLASS_PFSQRT:             // 3DNOW
