! (C) Copyright 2026- ECMWF.
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! In applying this licence, ECMWF does not waive the privileges and immunities
! granted to it by virtue of its status as an intergovernmental organisation
! nor does it submit to any jurisdiction.


MODULE MPL_ALLOCATOR_MOD
IMPLICIT NONE
PRIVATE
PUBLIC :: MPL_ALLOCATOR_INIT
PUBLIC :: MPL_ALLOCATOR_FINALIZE
PUBLIC :: MPL_ALLOCATOR_INITIALIZED
PUBLIC :: MPL_ALLOCATE
PUBLIC :: MPL_DEALLOCATE
PUBLIC :: MPL_RESOURCE

INTERFACE
  MODULE FUNCTION MPL_ALLOCATOR_INITIALIZED() RESULT(INITIALIZED)
    LOGICAL :: INITIALIZED
  END FUNCTION

  MODULE SUBROUTINE MPL_ALLOCATOR_INIT(RESOURCE, RESERVE)
    CHARACTER(LEN=*), INTENT(IN), OPTIONAL :: RESOURCE
      !! The memory resource to use for the pool allocator. This can be used to select between different underlying memory management
      !! strategies (e.g. a memory pool, a custom allocator, or the system allocator).
    INTEGER, INTENT(IN), OPTIONAL :: RESERVE
      !! The amount of memory to reserve in the pool resource, in bytes.
      !! Depending on the underlying RESOURCE, this may be treated as a soft limit (e.g. for a memory pool)
      !! or a hard limit (e.g. for a custom allocator that directly reserves memory from the system).
      !! Any allocation request that exceeds the reserved amount may cause an error or trigger additional memory management behavior,
      !! depending on the implementation.
  END SUBROUTINE 

  MODULE SUBROUTINE MPL_ALLOCATOR_FINALIZE()
    !! Finalize the MPL allocator
  END SUBROUTINE
END INTERFACE

INTERFACE
  MODULE FUNCTION MPL_RESOURCE() RESULT(RESOURCE)
    !! Returns the name of the currently initialized memory resource for the MPL allocator
    CHARACTER(LEN=:), ALLOCATABLE :: RESOURCE
  END FUNCTION
END INTERFACE

INTERFACE MPL_ALLOCATE
    !! Allocate arrays using the MPL allocator
    MODULE PROCEDURE :: MPL_ALLOCATE_LABEL_ARRAY_BOUNDS
    MODULE PROCEDURE :: MPL_ALLOCATE_LABEL_ARRAY_SHAPE
END INTERFACE MPL_ALLOCATE
INTERFACE
  MODULE SUBROUTINE MPL_ALLOCATE_LABEL_ARRAY_BOUNDS(LABEL, ARRAY, LBOUNDS, UBOUNDS)
    USE, INTRINSIC :: ISO_FORTRAN_ENV, ONLY : REAL64, INT32
    CHARACTER(LEN=*), INTENT(IN) :: LABEL ! Label used for tracing or debugging
    REAL(REAL64), POINTER, INTENT(INOUT) :: ARRAY(:,:) ! Array to be allocated
    INTEGER(INT32), INTENT(IN) :: LBOUNDS(2), UBOUNDS(2) ! Lower and upper bounds (inclusive) for each dimension of the array
  END SUBROUTINE
  MODULE SUBROUTINE MPL_ALLOCATE_LABEL_ARRAY_SHAPE(LABEL, ARRAY, SHAPE)
    USE, INTRINSIC :: ISO_FORTRAN_ENV, ONLY : REAL64, INT32
    CHARACTER(LEN=*), INTENT(IN) :: LABEL ! Label used for tracing or debugging
    REAL(REAL64), POINTER, INTENT(INOUT) :: ARRAY(:,:) ! Array to be allocated
    INTEGER(INT32), INTENT(IN) :: SHAPE(2) ! Shape of the array (number of elements in each dimension)
  END SUBROUTINE
END INTERFACE

INTERFACE MPL_DEALLOCATE
    !! Deallocate arrays using the MPL allocator
    MODULE PROCEDURE :: MPL_DEALLOCATE_LABEL_ARRAY
END INTERFACE MPL_DEALLOCATE
INTERFACE
  MODULE SUBROUTINE MPL_DEALLOCATE_LABEL_ARRAY(LABEL, ARRAY)
    USE, INTRINSIC :: ISO_FORTRAN_ENV, ONLY : REAL64
    CHARACTER(LEN=*), INTENT(IN) :: LABEL ! Label used for tracing or debugging
    REAL(REAL64), POINTER, INTENT(INOUT) :: ARRAY(:,:) ! Array to be deallocated
  END SUBROUTINE
END INTERFACE
END MODULE

!----------------------------------------------------------------------------------------------------------------------------------
