#include <windows.h>

internal_function u64
LockedAddAndReturnPreviousValue(volatile u64* value, u64 to_add)
{
    u64 result = InterlockedExchangeAdd64((LONGLONG*)value, to_add);
    return result;
}

internal_function u32
GetCoreCount()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    
    u32 result = info.dwNumberOfProcessors;

    return result;
}

internal_function DWORD WINAPI
RenderTileThreadProcedure(void* parameter)
{
    work_queue* queue = (work_queue*) parameter;
    while (RenderTile(queue)) {}
    return 0;
}

internal_function void
CreateRenderTileThread(work_queue* queue)
{
    DWORD thread_id;
    HANDLE thread_handle = CreateThread(0, 0, RenderTileThreadProcedure, queue, 0, &thread_id);
    CloseHandle(thread_handle);
}