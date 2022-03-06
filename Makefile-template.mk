# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# $HeadURL$
#
# Nathan Tallent
#****************************************************************************

#****************************************************************************
# Interface and user settings
#****************************************************************************

#----------------------------------------------------------------------------
# Build/compilation rules
#----------------------------------------------------------------------------

#-----------------------------------------------------------
# MK_PROGRAMS_CXX = List of <exe>
# MK_PROGRAMS_C =   List of <exe>
#-----------------------------------------------------------
# CXXLINK, CLINK
# CXX, CC
# CXXFLAGS, CFLAGS
# <exe>_SRCS
# <exe>_CXXFLAGS, <exe>_CFLAGS
# <exe>_LDFLAGS
# <exe>_LIBS
# <exe>_LDADD

#-----------------------------------------------------------
# MK_LIBRARIES_CXX = List of <lib>
# MK_LIBRARIES_C   = List of <lib>
#-----------------------------------------------------------
# AR, ARFLAGS
# CXX, CC
# CXXFLAGS, CFLAGS
# <lib>_SRCS
# <lib>_CXXFLAGS, <lib>_CFLAGS


#----------------------------------------------------------------------------
# Targets and recursive makes
#----------------------------------------------------------------------------

#-----------------------------------------------------------
# MK_SUBDIRS: subdirs for recursive makes
#-----------------------------------------------------------

#-----------------------------------------------------------
# MK_TARGETS_POST: post-order dependencies w.r.t. MK_SUBDIRS
#-----------------------------------------------------------

MK_TARGETS_POST = \
	all \
	\
	clean distclean \
	\
	install \
	\
	check check_clean check_diff check_update

#-----------------------------------------------------------
# MK_TARGETS_PRE: pre-order dependencies w.r.t MK_SUBDIRS
#-----------------------------------------------------------

MK_TARGETS_PRE = \
	info


#----------------------------------------------------------------------------
# Check (test) rules
#----------------------------------------------------------------------------

#-----------------------------------------------------------
# MK_CHECK: List of test/check classes, each designated <class>
#-----------------------------------------------------------
#
# <class>_CHECK: List of test/check output files, each <check>

# <class>_CHECK_BASE: Given <check>, generate the base/stem
#   <check_base>, which will be used to generate <check_diff> and
#   <check_update> targets. Should be a GNU Make 'function variable',
#   i.e., invokable with $(call...). There is one parameter, the input
#   <check> target.
#
#   Example that removes a suffix using a pattern rule:
#     CHECK_FN = $(patsubst %.out,%,$(1))

# Generate target name suffixes for <check_diff> and <check_update>
#   (overridable).  Each target is generated using a GNU Make suffix
#   pattern rule. For example:
#     <check_diff> = %<class>_DIFF, where % is <check_base>.
#
#   <class>_DIFF:   Name of <check_diff> target from <check_base>
#   <class>_UPDATE: Name of <check_update> target from <check_base>

CHECK_DIFF   := %.diff
CHECK_UPDATE := %.update

# Commands that execute and generate target files. Each variable
#   defines a GNU Make 'recipe', i.e., shell command that may use GNU
#   Make 'automatic variables'.
#
#   <class>_RUN:        Generate each <check> (output files)
#   <class>_RUN_DIFF:   Generate <check_diff> file
#   <class>_RUN_UPDATE: Generate <check_gold> file
#
#   Useful variables and notes:
#     $@          : target
#     $${chk_base}: <check_base> for RUN (no pattern rule is assumed)
#     $*          : <check_base> for RUN_DIFF and RUN_UPDATE
#     bash note:  : <command> && { <$? -eq 0> } || { $? -ne 0 }
#   
#   Example (simple)
#     RUN        = <app> -o $@ $${chk_base}.in
#     RUN_DIFF   = diff -C0 -N $*.out $*.gold > $@
#     RUN_UPDATE = mv $*.out $*.gold
#
#   Example: generate both <check> and sdtout/stderr
#     RUN        = <app> ... -o $@ >& $${chk_base}.out-oe
#     RUN_DIFF   = diff $*.out $*.gold > $@ && diff $*.out-oe $*.gold-oe >> $@
#     RUN_UPDATE = mv $*.out $*.gold        && mv $*.out-oe $*.gold-oe

# <class>_CLEAN: Additional clean targets (e.g., additional output files)


# TODO:
# - prerequisite for RUN could be the program
# - cleaner version of RUN command (doesn't work -- trickier than it seemed)
CHECK_RUN_VARS = chk_base="$$(call $$(1)_CHECK_BASE,$@)"


#----------------------------------------------------------------------------
# Interface for common utilities
#----------------------------------------------------------------------------

SHELL = /bin/bash

PRINTF = printf --
RM     = rm -f

SED    = sed -r
AWK    = awk
TAR    = tar


#****************************************************************************
# Helpers
#****************************************************************************

_msg_target_dbg = "debug: '$@' ('$(*D)', '$(*F)')\n"

#----------------------------------------------------------------------------

_msg_ClrBeg0="$$'\e[1m\e[4m\e[35m'" # '$' syntax quotes escapes
_msg_ClrBeg1="$$'\e[1m\e[4m\e[31m'" # '$' syntax quotes escapes
_msg_ClrEnd="$$'\e[0m'"

_msg_sep1 := "*****************************************************************************"
_msg_sep2 := "---------------------------------------------------------"

_msg_infoGrp  = "$(_msg_sep1)\n$@: $(CURDIR)\n$(_msg_sep1)\n"
_msg_info     = "$@: $^\n"
_msg_dbg      = "debug: '$@' : '$^'"
_msg_err      = "$(_msg_ClrBeg1)Error$(_msg_ClrEnd): "

_msg_vars_dbg = "$(_msg_sep2)\ncheck:        $(_mk_check)\ncheck_diff:   $(_mk_check_diff)\ncheck_update: $(_mk_check_update)\ncheck_clean:  $(_mk_check_clean)\n$(_msg_sep2)\n"

#----------------------------------------------------------------------------

# Check that given variables are set and all have non-empty values,
# die with an error otherwise.
#   https://stackoverflow.com/questions/10858261/how-to-abort-makefile-if-variable-not-set
#
# Params:
#   1. List of variable name(s) to test
#   2. (optional) Error message to print.
#
_ensure_def = \
  $(strip $(foreach var,$(1),\
            $(call _ensure_def1,$(var),$(strip $(value 2)))))
_ensure_def1 = \
  $(if $(value $(1)),,$(error Undefined $(1)$(if $(2), ($(2)))))


#****************************************************************************
# Generate dependencies for targets
#****************************************************************************

# _realTargetL: target names for targets

#_realTargetSfx = .real

#_realTargetL_post = $(addsuffix $(_realTargetSfx), $(MK_TARGETS_POST))

# t <-- t.local-post <-- t.real <-- t.local-pre <-- subdirs

#----------------------------------------------------------------------------

# _localTargetL_*: target names for local targets

_localTargetSfx = .local
_localTargetSfx1  = $(_localTargetSfx)1

_localTargetL_pre =  $(addsuffix $(_localTargetSfx),  $(MK_TARGETS_PRE))
_localTargetL_pre1 = $(addsuffix $(_localTargetSfx1), $(MK_TARGETS_PRE))
_localTargetL_post = $(addsuffix $(_localTargetSfx),  $(MK_TARGETS_POST))

#----------------------------------------------------------------------------

# _subdirTargetL_*: target names for all (target, subdir) pairs.

_subdirTargetSfx_post = .subdir-post
_subdirTargetSfx_pre  = .subdir-pre

_subdirTargetL_post = \
  $(foreach tgt, $(MK_TARGETS_POST), \
     $(foreach dir, $(MK_SUBDIRS), $(tgt)/$(dir)$(_subdirTargetSfx_post)))

_subdirTargetL_pre = \
  $(foreach tgt, $(MK_TARGETS_PRE), \
    $(foreach dir, $(MK_SUBDIRS), $(tgt)/$(dir)$(_subdirTargetSfx_pre)))

#----------------------------------------------------------------------------

# Post-order target definitions: A post-order target t launches its
# local target after its subdir targets.

$(MK_TARGETS_POST) : % : %$(_localTargetSfx)

$(_localTargetL_post) : %$(_localTargetSfx) : \
  $(foreach dir, $(MK_SUBDIRS), %/$(dir)$(_subdirTargetSfx_post))

$(_subdirTargetL_post) : %$(_subdirTargetSfx_post) :
ifdef DEBUG
	@printf $(_msg_target_dbg)
endif
	@tgt=$(*D) && dir=$(*F) \
	  && $(MAKE) -C $${dir} $${tgt}

.PHONY : $(MK_TARGETS_POST)
.PHONY : $(_localTargetL_post)
.PHONY : $(_subdirTargetL_post)

#----------------------------------------------------------------------------

# Pre-order target definitions: A pre-order target launches subdir
# targets after its local target.

$(MK_TARGETS_PRE) : % : \
  %$(_localTargetSfx1) \
  $(foreach dir, $(MK_SUBDIRS), %/$(dir)$(_subdirTargetSfx_pre))

.SECONDEXPANSION:
$(_subdirTargetL_pre) : %$(_subdirTargetSfx_pre) : $$(*D)$$(_localTargetSfx)
ifdef DEBUG
	@printf $(_msg_target_dbg)
endif
	@tgt=$(*D) && dir=$(*F) \
	  && $(MAKE) -C $${dir} $${tgt}

$(_localTargetL_pre1) : %$(_localTargetSfx1) : %$(_localTargetSfx)

.PHONY : $(MK_TARGETS_PRE)
.PHONY : $(_localTargetL_pre)
.PHONY : $(_localTargetL_pre1)
.PHONY : $(_subdirTargetL_pre)

#----------------------------------------------------------------------------

_force : 
.PHONY : _force


#****************************************************************************
# Build/Compilation rules
#****************************************************************************

_sfx_cpp = .cpp
_sfx_c = .c

CXXLINK ?= $(CXX)

CLINK ?= $(CC)

ARFLAGS ?= rcs

#----------------------------------------------------------------------------

define _program_template_cxx
  # Args: (1): program target
  #  
  # Note: qualify .o patterns so multiple targets can build the same
  # source file with different options:
  #   %.o -> %-$(1).o

  $(1)_objs = \
    $$(patsubst %$(_sfx_cpp),%-$(1).o,$$(patsubst %$(_sfx_c),%-$(1).o,$$($(1)_SRCS)))

  $(1)_objs_cpp = \
    $$(patsubst %$(_sfx_cpp),%-$(1).o,$$(filter %$(_sfx_cpp),$$($(1)_SRCS)))

  $(1)_objs_c = \
    $$(patsubst %$(_sfx_c),%-$(1).o,$$(filter %$(_sfx_c),$$($(1)_SRCS)))

  $(1) : $$($(1)_objs) $$($(1)_LIBS)
	$$(CXXLINK) -o $$@ \
		$$($(1)_CXXFLAGS) $$(CXXFLAGS) \
		$$($(1)_LDFLAGS) $$(LDFLAGS) \
		$$^ $$($(1)_LDADD)

  $$($(1)_objs_cpp) : %-$(1).o : %$(_sfx_cpp)
	$$(CXX) -c -o $$@ $$($(1)_CXXFLAGS) $$(CXXFLAGS) $$^

  $$($(1)_objs_c) : %-$(1).o : %$(_sfx_c)
	$$(CC) -c -o $$@ $$($(1)_CFLAGS) $$(CFLAGS) $$^

  _program_objs += $$($(1)_objs)
endef


#----------------------------------------------------------------------------

define _program_template_c
  # Args: (1): program target
  #  
  # Note: qualify .o patterns so multiple targets can build the same
  # source file with different options:
  #   %.o -> %-$(1).o

  $(1)_objs = $$(patsubst %$(_sfx_c),%-$(1).o,$$($(1)_SRCS))

  $(1) : $$($(1)_objs) $$($(1)_LIBS)
	$$(CLINK) -o $$@ \
		$$($(1)_CFLAGS) $$(CFLAGS) \
		$$($(1)_LDFLAGS) $$(LDFLAGS) \
		$$^ $$($(1)_LDADD)

  $$($(1)_objs) : %-$(1).o : %$(_sfx_c)
	$$(CC) -c -o $$@ $$($(1)_CFLAGS) $$(CFLAGS) $$^

  _program_objs += $$($(1)_objs)
endef


#----------------------------------------------------------------------------

define _library_template_c_cxx
  # Args: (1): library target
  #
  # Note: qualify .o patterns so multiple targets can build the same
  # source file with different options:
  #   %.o -> %-$(1).o

  $(1)_objs = \
    $$(patsubst %$(_sfx_cpp),%-$(1).o,$$(patsubst %$(_sfx_c),%-$(1).o,$$($(1)_SRCS)))

  $(1)_objs_cpp = \
    $$(patsubst %$(_sfx_cpp),%-$(1).o,$$(filter %$(_sfx_cpp),$$($(1)_SRCS)))

  $(1)_objs_c = \
    $$(patsubst %$(_sfx_c),%-$(1).o,$$(filter %$(_sfx_c),$$($(1)_SRCS)))

  $(1) : $$($(1)_objs)
	$$(AR) $$(ARFLAGS) $$@ $$^

  $$($(1)_objs_cpp) : %-$(1).o : %$(_sfx_cpp)
	$$(CXX) -c -o $$@ $$($(1)_CXXFLAGS) $$(CXXFLAGS) $$^

  $$($(1)_objs_c) : %-$(1).o : %$(_sfx_c)
	$$(CC) -c -o $$@ $$($(1)_CFLAGS) $$(CFLAGS) $$^

  _library_objs += $$($(1)_objs)
endef


#----------------------------------------------------------------------------

$(foreach x,$(MK_PROGRAMS_CXX),$(eval $(call _program_template_cxx,$(x))))

$(foreach x,$(MK_PROGRAMS_C),$(eval $(call _program_template_c,$(x))))

$(foreach x,$(MK_LIBRARIES_C) $(MK_LIBRARIES_CXX),$(eval $(call _library_template_c_cxx,$(x))))

#$(MK_PROGRAMS_CXX) :
#	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

#$(MK_PROGRAMS_C) :
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

#%.o : %.c
#	$(CC) $(CFLAGS) -c -o $@ $^

#%.o : %.cpp
#	$(CXX) $(CXXFLAGS) -c -o $@ $^


#****************************************************************************
# Check (test) rules
#****************************************************************************

define _check_template
  # Args: (1): check class

  #---------------------------------------------------------

  $(call _ensure_def, $(1)_CHECK)

  $(call _ensure_def, $(1)_CHECK_BASE)

  $(call _ensure_def, $(1)_RUN $(1)_RUN_DIFF $(1)_RUN_UPDATE)

  #---------------------------------------------------------

  $(1)_DIFF   ?= $$(CHECK_DIFF)
  $(1)_UPDATE ?= $$(CHECK_UPDATE)

  $(1)_ck_base = $$(foreach x,$$($(1)_CHECK),$$(call $(1)_CHECK_BASE,$$(x)))

  $(1)_ck_diff = $$(patsubst %,$$($(1)_DIFF),$$($(1)_ck_base))

  $(1)_ck_updt = $$(patsubst %,$$($(1)_UPDATE),$$($(1)_ck_base))

  #---------------------------------------------------------

  _mk_check        += $$($(1)_CHECK)

  _mk_check_clean  += $$($(1)_CLEAN)

  _mk_check_diff   += $$($(1)_ck_diff)

  _mk_check_update += $$($(1)_ck_updt)


  #---------------------------------------------------------


$$($(1)_CHECK) : 
	  @$(PRINTF) $$(_msg_info)
ifdef DEBUG
	  @chk_base="$$(call $(1)_CHECK_BASE,$$@)" && \
	    $(PRINTF) $$(_msg_dbg)" // '$$@' -> '$$$${chk_base}'\n"
endif
	  chk_base="$$(call $(1)_CHECK_BASE,$$@)" && \
	    $$($(1)_RUN) || $(PRINTF) $$(_msg_err)"$$($(1)_RUN)\n"

  #---------------------------------------------------------

  $$($(1)_ck_diff) : $$($(1)_DIFF) : _force
	@$(PRINTF) $$(_msg_info)
ifdef DEBUG
	  @$(PRINTF) $$(_msg_dbg)"\n"
endif
	  $$($(1)_RUN_DIFF) || $(PRINTF) $$(_msg_err)"$$($(1)_RUN_DIFF)\n"

  #---------------------------------------------------------

  $$($(1)_ck_updt) : $$($(1)_UPDATE) : 
	  @$(PRINTF) $$(_msg_info)
ifdef DEBUG
	  @$(PRINTF) $$(_msg_dbg)"\n"
endif
	  $$($(1)_RUN_UPDATE) || $(PRINTF) $$(_msg_err)"$$($(1)_RUN_UPDATE)\n"

  #---------------------------------------------------------

endef

$(foreach x,$(MK_CHECK),$(eval $(call _check_template,$(x))))


#****************************************************************************
# Interface targets
#****************************************************************************

all : $(MK_PROGRAMS_C) $(MK_PROGRAMS_CXX) $(MK_LIBRARIES_C) $(MK_LIBRARIES_CXX)

install : all
install.local : all

clean :
	for x in $(_program_objs) $(_library_objs) ; do $(RM) $${x} ; done
#	x="$(_program_objs)" && if test -n "$${x}" ; then $(RM) $${x} ; fi
#	x="$(_library_objs)" && if test -n "$${x}" ; then $(RM) $${x} ; fi

distclean : clean
	for x in $(MK_PROGRAMS_C) $(MK_PROGRAMS_CXX) $(MK_LIBRARIES_C) $(MK_LIBRARIES_CXX) ; do $(RM) $${x} ; done
#	x="$(MK_PROGRAMS_C)" && if test -n "$${x}" ; then $(RM) $${x} ; fi
#	x="$(MK_PROGRAMS_CXX)" && if test -n "$${x}" ; then $(RM) $${x} ; fi
#	x="$(MK_LIBRARIES_C)" && if test -n "$${x}" ; then $(RM) $${x} ; fi
#	x="$(MK_LIBRARIES_CXX)" && if test -n "$${x}" ; then $(RM) $${x} ; fi

#----------------------------------------------------------------------------

check : all
check.local : all

#----------------------------------------------------------------------------

_check_pre _check_diff_pre _check_update_pre :
	@$(PRINTF) $(_msg_infoGrp)
ifdef DEBUG
	@$(PRINTF) $(_msg_vars_dbg)
endif


check : _check_pre $(_mk_check) check_diff
ifdef DEBUG
	@$(PRINTF) $(_msg_dbg)"\n"
endif


check_diff : _check_diff_pre $(_mk_check_diff)
ifdef DEBUG
	@$(PRINTF) $(_msg_dbg)"\n"
	@$(PRINTF) $(_msg_vars_dbg)
endif


check_update : _check_update_pre $(_mk_check_update)
ifdef DEBUG
	@$(PRINTF) $(_msg_dbg)"\n"
	@$(PRINTF) $(_msg_vars_dbg)
endif


check_clean :
	@$(PRINTF) $(_msg_infoGrp)
ifdef DEBUG
	@$(PRINTF) $(_msg_dbg)"\n"
	@$(PRINTF) $(_msg_vars_dbg)
endif
	@$(RM) $(_mk_check) $(_mk_check_diff) $(_mk_check_clean)

# .PHONY : all check check_diff check_update clean


#****************************************************************************

help :
	@echo "$(MK_TARGETS_POST) $(MK_TARGETS_PRE)"

