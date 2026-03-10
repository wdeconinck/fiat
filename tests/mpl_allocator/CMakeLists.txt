# (C) Copyright 2020- ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

# ----------------------------------------------------------------------------------------
# Tests: fiat_test_mpl_allocator
#   test that MPL allocator functionality works

find_package(pluto)
set(HAVE_PLUTO 0)
if (pluto_FOUND)
  set(HAVE_PLUTO 1)
endif()
ecbuild_add_test(
    TARGET fiat_test_mpl_allocator
    SOURCES test_mpl_allocator.F90
            mpl_allocator_mod.F90
            mpl_allocator_smod.F90
            internal/fiat_buddy_mod.F90
            internal/fiat_buddy_mod.cc
            internal/buddy_alloc.h
    LIBS    ${fiatlib} $<${HAVE_PLUTO}:pluto_f>
    DEFINITIONS HAVE_PLUTO=${HAVE_PLUTO}
    LINKER_LANGUAGE Fortran
    ENVIRONMENT "MPL=0"
)
