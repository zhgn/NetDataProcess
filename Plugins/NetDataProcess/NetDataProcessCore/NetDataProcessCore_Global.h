#ifndef NETDATAPROCESSCORE_GLOBAL_H
#define NETDATAPROCESSCORE_GLOBAL_H

#include <QtGlobal>

#if defined(NETDATAPROCESSCORE_LIBRARY)
#  define NETDP_EXPORT Q_DECL_EXPORT
#else
#  define NETDP_EXPORT Q_DECL_IMPORT
#endif

#endif // NETDATAPROCESSCORE_GLOBAL_H