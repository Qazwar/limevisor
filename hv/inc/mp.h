


#pragma once


typedef HVSTATUS( *PMP_PROCEDURE )(
    __in PVOID Argument
    );

HVSTATUS
MpProcessorExecute(
    __in PMP_PROCEDURE  Procedure,
    __in PVOID          Argument
);
