#include <pthread.h>
#include <libkern/OSAtomic.h>  // OSAtomic*
#include <assert.h>

#include "main.cpp"

global_variable pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

internal_function u64
LockedAddAndReturnPreviousValue(volatile u64* value, u64 to_add)
{
    // 'value' is a shared resource!

    // https://stackoverflow.com/questions/28888719/multi-threaded-c-program-much-slower-in-os-x-than-linux

    // https://stackoverflow.com/questions/33991644/why-is-performance-of-pthread-mutex-so-bad-on-mac-os-x-compared-to-linux

    s64 result = OSAtomicAdd64((s64)to_add, (s64*)value);
    result -= (s64)to_add;
    return (u64)result;
}

internal_function u32
GetCoreCount()
{
    return 1;
}

internal_function void*
RenderTileThreadProcedure(void* parameter)
{
    work_queue* queue = (work_queue*) parameter;
    while (RenderTile(queue)) {}
}

internal_function void
CreateRenderTileThread(work_queue* queue)
{
    // Create the thread using POSIX routines.
    pthread_t thread_id;
    int error;
 
    error = pthread_create(&thread_id, NULL, RenderTileThreadProcedure, (void*)queue);
    if (error)
    {
        fprintf(stderr, "Failed to create thread. Error code: %d\n", error);
    }
    
    error = pthread_detach(thread_id);
    if (error)
    {
        fprintf(stderr, "Failed to make thread detachable. Error code: %d\n", error);
    }
}


int main(int argument_count, char* argument_array[])
{
    s32 return_value = EntryPoint(argument_count, argument_array);
    return return_value;
}
