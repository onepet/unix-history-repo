/******************************************************************************
 *
 * Module Name: utalloc - local memory allocation routines
 *              $Revision: 1.164 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2007, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#define __UTALLOC_C__

#include <contrib/dev/acpica/acpi.h>
#include <contrib/dev/acpica/acdebug.h>

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utalloc")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCreateCaches
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create all local caches
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtCreateCaches (
    void)
{
    ACPI_STATUS             Status;


    /* Object Caches, for frequently used objects */

    Status = AcpiOsCreateCache ("Acpi-Namespace", sizeof (ACPI_NAMESPACE_NODE),
                ACPI_MAX_NAMESPACE_CACHE_DEPTH, &AcpiGbl_NamespaceCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-State", sizeof (ACPI_GENERIC_STATE),
                ACPI_MAX_STATE_CACHE_DEPTH, &AcpiGbl_StateCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-Parse", sizeof (ACPI_PARSE_OBJ_COMMON),
                ACPI_MAX_PARSE_CACHE_DEPTH, &AcpiGbl_PsNodeCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-ParseExt", sizeof (ACPI_PARSE_OBJ_NAMED),
                ACPI_MAX_EXTPARSE_CACHE_DEPTH, &AcpiGbl_PsNodeExtCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-Operand", sizeof (ACPI_OPERAND_OBJECT),
                ACPI_MAX_OBJECT_CACHE_DEPTH, &AcpiGbl_OperandCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }


#ifdef ACPI_DBG_TRACK_ALLOCATIONS

    /* Memory allocation lists */

    Status = AcpiUtCreateList ("Acpi-Global", 0,
                &AcpiGbl_GlobalList);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiUtCreateList ("Acpi-Namespace", sizeof (ACPI_NAMESPACE_NODE),
                &AcpiGbl_NsNodeList);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }
#endif

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDeleteCaches
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Purge and delete all local caches
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtDeleteCaches (
    void)
{
#ifdef ACPI_DBG_TRACK_ALLOCATIONS
    char                    Buffer[7];

    if (AcpiGbl_DisplayFinalMemStats)
    {
        ACPI_STRCPY (Buffer, "MEMORY");
        AcpiDbDisplayStatistics (Buffer);
    }
#endif

    (void) AcpiOsDeleteCache (AcpiGbl_NamespaceCache);
    AcpiGbl_NamespaceCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_StateCache);
    AcpiGbl_StateCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_OperandCache);
    AcpiGbl_OperandCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_PsNodeCache);
    AcpiGbl_PsNodeCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_PsNodeExtCache);
    AcpiGbl_PsNodeExtCache = NULL;


#ifdef ACPI_DBG_TRACK_ALLOCATIONS

    /* Debug only - display leftover memory allocation, if any */

    AcpiUtDumpAllocations (ACPI_UINT32_MAX, NULL);

    /* Free memory lists */

    AcpiOsFree (AcpiGbl_GlobalList);
    AcpiGbl_GlobalList = NULL;

    AcpiOsFree (AcpiGbl_NsNodeList);
    AcpiGbl_NsNodeList = NULL;
#endif

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidateBuffer
 *
 * PARAMETERS:  Buffer              - Buffer descriptor to be validated
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform parameter validation checks on an ACPI_BUFFER
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtValidateBuffer (
    ACPI_BUFFER             *Buffer)
{

    /* Obviously, the structure pointer must be valid */

    if (!Buffer)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Special semantics for the length */

    if ((Buffer->Length == ACPI_NO_BUFFER)              ||
        (Buffer->Length == ACPI_ALLOCATE_BUFFER)        ||
        (Buffer->Length == ACPI_ALLOCATE_LOCAL_BUFFER))
    {
        return (AE_OK);
    }

    /* Length is valid, the buffer pointer must be also */

    if (!Buffer->Pointer)
    {
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInitializeBuffer
 *
 * PARAMETERS:  Buffer              - Buffer to be validated
 *              RequiredLength      - Length needed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validate that the buffer is of the required length or
 *              allocate a new buffer.  Returned buffer is always zeroed.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtInitializeBuffer (
    ACPI_BUFFER             *Buffer,
    ACPI_SIZE               RequiredLength)
{
    ACPI_STATUS             Status = AE_OK;


    switch (Buffer->Length)
    {
    case ACPI_NO_BUFFER:

        /* Set the exception and returned the required length */

        Status = AE_BUFFER_OVERFLOW;
        break;


    case ACPI_ALLOCATE_BUFFER:

        /* Allocate a new buffer */

        Buffer->Pointer = AcpiOsAllocate (RequiredLength);
        if (!Buffer->Pointer)
        {
            return (AE_NO_MEMORY);
        }

        /* Clear the buffer */

        ACPI_MEMSET (Buffer->Pointer, 0, RequiredLength);
        break;


    case ACPI_ALLOCATE_LOCAL_BUFFER:

        /* Allocate a new buffer with local interface to allow tracking */

        Buffer->Pointer = ACPI_ALLOCATE_ZEROED (RequiredLength);
        if (!Buffer->Pointer)
        {
            return (AE_NO_MEMORY);
        }
        break;


    default:

        /* Existing buffer: Validate the size of the buffer */

        if (Buffer->Length < RequiredLength)
        {
            Status = AE_BUFFER_OVERFLOW;
            break;
        }

        /* Clear the buffer */

        ACPI_MEMSET (Buffer->Pointer, 0, RequiredLength);
        break;
    }

    Buffer->Length = RequiredLength;
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAllocate
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: Subsystem equivalent of malloc.
 *
 ******************************************************************************/

void *
AcpiUtAllocate (
    ACPI_SIZE               Size,
    UINT32                  Component,
    char                    *Module,
    UINT32                  Line)
{
    void                    *Allocation;


    ACPI_FUNCTION_TRACE_U32 (UtAllocate, Size);


    /* Check for an inadvertent size of zero bytes */

    if (!Size)
    {
        ACPI_WARNING ((Module, Line,
            "Attempt to allocate zero bytes, allocating 1 byte"));
        Size = 1;
    }

    Allocation = AcpiOsAllocate (Size);
    if (!Allocation)
    {
        /* Report allocation error */

        ACPI_WARNING ((Module, Line,
            "Could not allocate size %X", (UINT32) Size));

        return_PTR (NULL);
    }

    return_PTR (Allocation);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAllocateZeroed
 *
 * PARAMETERS:  Size                - Size of the allocation
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: Subsystem equivalent of calloc. Allocate and zero memory.
 *
 ******************************************************************************/

void *
AcpiUtAllocateZeroed (
    ACPI_SIZE               Size,
    UINT32                  Component,
    char                    *Module,
    UINT32                  Line)
{
    void                    *Allocation;


    ACPI_FUNCTION_ENTRY ();


    Allocation = AcpiUtAllocate (Size, Component, Module, Line);
    if (Allocation)
    {
        /* Clear the memory block */

        ACPI_MEMSET (Allocation, 0, Size);
    }

    return (Allocation);
}

