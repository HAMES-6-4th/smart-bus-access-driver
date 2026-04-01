#include "board_api.h"

#if defined(BOARD_TC375_LITE)
#include "board_tc375_lite.h"
#elif defined(BOARD_CUSTOM_X1)
#include "board_custom_x1.h"
#else
#error "No board selected"
#endif

BoardStatus boardInit(void)
{
    return boardImplInit();
}

const BoardDescriptor *boardGetDescriptor(void)
{
    return boardImplGetDescriptor();
}
    
const BoardCANPort *boardFindCANPort(const char *name)
{
    const BoardDescriptor *desc = boardGetDescriptor();
    uint32_t i;

    if ((desc == 0) || (name == 0))
    {
        return 0;
    }

    for (i = 0; i < desc->can_port_count; ++i)
    {
        const char *n = desc->can_ports[i].name;
        uint32_t j = 0;
        bool equal = true;

        while ((n[j] != '\0') || (name[j] != '\0'))
        {
            if (n[j] != name[j])
            {
                equal = false;
                break;
            }
            ++j;
        }

        if (equal)
        {
            return &desc->can_ports[i];
        }
    }

    return 0;
}

BoardStatus boardCANPortPowerOn(const BoardCANPort *port)
{
    return boardImplCANPortPowerOn(port);
}

BoardStatus boardCANPortPowerOff(const BoardCANPort *port)
{
    return boardImplCANPortPowerOff(port);
}

BoardStatus boardCANPortSetStandby(const BoardCANPort *port, bool enable)
{
    return boardImplCANPortSetStandby(port, enable);
}

BoardStatus boardCANPortSetTermination(const BoardCANPort *port, bool enable)
{
    return boardImplCANPortSetTermination(port, enable);
}