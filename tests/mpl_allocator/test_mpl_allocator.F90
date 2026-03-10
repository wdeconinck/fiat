! (C) Copyright 2026- ECMWF.
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! In applying this licence, ECMWF does not waive the privileges and immunities
! granted to it by virtue of its status as an intergovernmental organisation
! nor does it submit to any jurisdiction.

PROGRAM TEST_MPL_ALLOCATOR

USE EC_PARKIND, ONLY: JPIM, JPRD
USE MPL_MODULE, ONLY: MPL_INIT, MPL_ABORT, MPL_END, MPL_OUTPUT, MPL_UNIT

IMPLICIT NONE

INTEGER(JPIM), PARAMETER :: STDOUT = 6
INTEGER(JPIM), PARAMETER :: GB = 1024*1024*1024

IF (MPL()) THEN
    CALL MPL_INIT(KOUTPUT=1)
ELSE
    MPL_UNIT=STDOUT
    MPL_OUTPUT=1
ENDIF

BLOCK
    USE MPL_ALLOCATOR_MOD, ONLY: MPL_ALLOCATOR_INIT, MPL_ALLOCATOR_FINALIZE, MPL_ALLOCATE, MPL_DEALLOCATE, MPL_RESOURCE
    REAL(JPRD), POINTER :: ARRAY(:,:)

    ! Initialize the allocator with 1 GB of reserved memory in the pool resource with a given resource
    ! CALL MPL_ALLOCATOR_INIT(RESOURCE="BUDDY_ALLOC", RESERVE=1*GB)  ! "BUDDY_ALLOC" can be used without pluto available
    ! CALL MPL_ALLOCATOR_INIT(RESOURCE="mpi_pool",    RESERVE=1*GB)  ! "mpi_pool" can only be used with pluto available
    ! CALL MPL_ALLOCATOR_INIT(RESOURCE="FORTRAN",     RESERVE=1*GB)  ! Uses standard Fortran allocate/deallocate instead 
                                                                    ! Reserve is ignored for FORTRAN resource
    ! CALL MPL_ALLOCATOR_INIT(RESERVE=1*GB) ! Use default resource: 
    !                                            - "BUDDY_ALLOC" when pluto is not available
    !                                            - "mpi_pool" when pluto is available

    CALL MPL_ALLOCATOR_INIT(RESOURCE=GET_MPL_RESOURCE_FROM_ENV(), RESERVE=1*GB)
    !            When GET_MPL_RESOURCE_FROM_ENV() is empty, same effect as when RESOURCE is not passed

    WRITE(STDOUT,'(3A)') "MPL_RESOURCE = """, MPL_RESOURCE(),""""

    CALL MPL_ALLOCATE("ARRAY_1", ARRAY, SHAPE=[32,100]) ! Allocate a 32x100 array using the allocator
    ! Use the array for some computations here...
    ARRAY(:,1) = 1.
    ARRAY(:,2) = 2.
    ARRAY(:,3) = 3.
    WRITE(STDOUT,'(A)') "ARRAY(1:4,1:2) = "
    WRITE(STDOUT,'(4F8.2)') ARRAY(1:4,1:2)
    CALL MPL_DEALLOCATE("ARRAY_1", ARRAY) ! Deallocate the array when done

    CALL MPL_ALLOCATE("ARRAY_2", ARRAY, LBOUNDS=[0,0], UBOUNDS=[15,49])
    ARRAY(:,0) = 1.
    ARRAY(:,1) = 2.
    ARRAY(:,2) = 3.
    WRITE(STDOUT,'(A)') "ARRAY(0:3,0:1) = "
    WRITE(STDOUT,'(4F8.2)') ARRAY(0:3,0:1)
    CALL MPL_DEALLOCATE("ARRAY_2", ARRAY)

    CALL MPL_ALLOCATOR_FINALIZE() ! Finalize the allocator
END BLOCK

if (MPL()) CALL MPL_END(LDMEMINFO=.FALSE.)

CONTAINS

FUNCTION MPL() RESULT(LMPL)
    LOGICAL :: LMPL
    CHARACTER(LEN=512) :: ENV
    CALL GET_ENVIRONMENT_VARIABLE("MPL",ENV)
    IF( ENV == '0' ) THEN
        LMPL = .FALSE.
    ELSE
        LMPL = .TRUE.
    ENDIF
END FUNCTION

FUNCTION GET_MPL_RESOURCE_FROM_ENV() RESULT(CMPL_RESOURCE)
    LOGICAL :: LMPL
    CHARACTER(LEN=512) :: CMPL_RESOURCE
    CALL GET_ENVIRONMENT_VARIABLE("MPL_RESOURCE",CMPL_RESOURCE)
END FUNCTION

END PROGRAM TEST_MPL_ALLOCATOR
