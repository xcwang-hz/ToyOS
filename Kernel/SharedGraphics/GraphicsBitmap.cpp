#include "GraphicsBitmap.h"
#include <AK/kmalloc.h>

RetainPtr<GraphicsBitmap> GraphicsBitmap::create_wrapper(const Size& size, RGBA32* data)
{
    return adopt(*new GraphicsBitmap(size, data));
}

GraphicsBitmap::GraphicsBitmap(const Size& size, RGBA32* data)
    : m_size(size)
    , m_data(data)
    , m_pitch(size.width() * sizeof(RGBA32))
{
}

GraphicsBitmap::~GraphicsBitmap()
{
// #ifdef KERNEL
//     if (m_client_region)
//         m_client_process->deallocate_region(*m_client_region);
//     if (m_server_region)
//         WSMessageLoop::the().server_process().deallocate_region(*m_server_region);
// #endif
    m_data = nullptr;
}

