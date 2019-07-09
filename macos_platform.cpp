#include <pthread.h>
#include <assert.h>


internal_function u64
LockedAddAndReturnPreviousValue(volatile u64* value, u64 to_add)
{
    // TODO(ted): Needs mutex lock.
    u64 result = *value;
    *value += to_add;
    return result;
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
    pthread_exit(NULL);
}

internal_function void
CreateRenderTileThread(work_queue* queue)
{
    // Create the thread using POSIX routines.
    pthread_attr_t attribute;
    pthread_t thread_id;
 
    if (!pthread_attr_init(&attribute))
    {
        assert(true);
    }
    if (!pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_DETACHED))
    {
        assert(true);
    }
    if (!pthread_create(&thread_id, &attribute, RenderTileThreadProcedure, (void*)queue))
    {
        assert(true);
    } 
//    if (!pthread_attr_destroy(&attribute))
//    {
//        assert(true);
//    }
}
