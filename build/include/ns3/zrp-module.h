#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ZRP
    // Module headers: 
    #include <ns3/zrp-routing-protocol.h>
    #include <ns3/zrp-helper.h>
#endif 