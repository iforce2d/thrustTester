#ifdef WIN32
    #include "libserialport/libserialport.h"
#else
    #include <libserialport.h>
#endif
#include "port.h"
#include "Log.h"


bool updatePortList()
{
    g_log.log(LL_DEBUG, "Updating serial port list");

    sp_port** ports;

    if ( SP_OK != sp_list_ports(&ports) ) {
        g_log.log(LL_ERROR, "Could not sp_list_ports");
        return false;
    }

    for (int i = 0; ports[i]; i++ ) {
        g_log.log(LL_DEBUG, "Port: %s", sp_get_port_name(ports[i]));
        if (sp_get_port_usb_product(ports[i]))
            g_log.log(LL_DEBUG, "   (%s)", sp_get_port_usb_product(ports[i]));
    }

    return true;
}
