#include "IRQHandler.h"
#if I386
#include "i386.h"
#include "PIC.h"
#endif 

IRQHandler::IRQHandler(byte irq)
    : m_irq_number(irq)
{
#if I386
    register_irq_handler(m_irq_number, *this);
#endif
}

IRQHandler::~IRQHandler()
{
    //unregister_irq_handler(m_irq_number, *this);
}

void IRQHandler::enable_irq()
{
#if I386
    PIC::enable(m_irq_number);
#endif
}

void IRQHandler::disable_irq()
{
    //PIC::disable(m_irq_number);
}

