


#include "hv.h"

#pragma pack(push, 8)
typedef struct _MP_PROCESSOR_CONTEXT {

    //
    //  the procedure, it's argument and the result.
    //

    PMP_PROCEDURE   Procedure;
    PVOID           Argument;
    HVSTATUS        ProcessorStatus;

    //
    //  object's for the dispatcher
    //

    KEVENT          CompletionEvent;
    KDPC            DispatcherObject;

    //
    //  CR3 of the caller.
    //

    ULONG64         CallerContext;

} MP_PROCESSOR_CONTEXT, *PMP_PROCESSOR_CONTEXT;
#pragma pack(pop)

VOID
MpProcessorDpcProcedure(
    __in PKDPC Dpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
)
{
    Dpc;
    SystemArgument1;
    SystemArgument2;

    //
    //  dpc procedure for MpProcessorExecute, will execute
    //  a procedure on each processor.
    //
    //  irql = dispatch_level
    //

    ULONG64 PreviousContext;
    PMP_PROCESSOR_CONTEXT ProcessorContext;

    //
    //  switch thread context to the original MpProcessorExecute
    //  caller's context passed in the MP_PROCESSOR_CONTEXT struct
    //

    ProcessorContext = DeferredContext;
    PreviousContext = HvGetThreadContext( );
    HvSetThreadContext( ProcessorContext->CallerContext );

    //
    //  execute the procedure and save the result in 
    //  the MP_PROCESSOR_CONTEXT struct for MpProcessorExecute
    //  to confirm success.
    //

    ProcessorContext->ProcessorStatus = ProcessorContext->Procedure( ProcessorContext->Argument );

    //
    //  signal the completion event for MpProcessorExecute,
    //  this will cause it to begin executing on the next
    //  processor.
    //

    KeSetEvent( &ProcessorContext->CompletionEvent, LOW_PRIORITY, FALSE );

    //
    //  revert the thread context changes such that when the dpc
    //  returns there are no #PF's or anything like that.
    //

    HvSetThreadContext( ProcessorContext->CallerContext );

}

HVSTATUS
MpProcessorExecute(
    __in PMP_PROCEDURE  Procedure,
    __in PVOID          Argument
)
{
    //
    //  this procedure queues a dpc on each processor
    //  for execution of Procedure, it executes it in order
    //  and checks the return status each time, aborting if
    //  not successful.
    //
    //  irql <= apc_level
    //

    MP_PROCESSOR_CONTEXT ProcessorContext;

    NT_ASSERT( KeGetCurrentIrql( ) < DISPATCH_LEVEL );

    ProcessorContext.Procedure = Procedure;
    ProcessorContext.Argument = Argument;
    ProcessorContext.ProcessorStatus = STATUS_SUCCESS;
    ProcessorContext.CallerContext = HvGetThreadContext( );

    KeInitializeEvent( &ProcessorContext.CompletionEvent, SynchronizationEvent, FALSE );

    //
    //  TODO:   can we initialize the dpc outside the loop instead?
    //          and then switch the target processor?
    //

    for (ULONG32 Processor = 0; Processor < KeQueryActiveProcessorCount( 0 ); Processor++) {

        KeInitializeDpc( &ProcessorContext.DispatcherObject, MpProcessorDpcProcedure, &ProcessorContext );
        KeSetTargetProcessorDpc( &ProcessorContext.DispatcherObject, ( CCHAR )Processor );
        KeInsertQueueDpc( &ProcessorContext.DispatcherObject, NULL, NULL );

        KeWaitForSingleObject( &ProcessorContext.CompletionEvent, Executive, KernelMode, FALSE, NULL );

        if (!NT_SUCCESS( ProcessorContext.ProcessorStatus )) {

            break;
        }

        KeResetEvent( &ProcessorContext.CompletionEvent );
    }

    return ProcessorContext.ProcessorStatus;
}