#include <QtCore/QtGlobal>

#ifdef LIBSTACKER__
#define LIBSTACKER_EXPORT Q_DECL_EXPORT
#else
#define LIBSTACKER_EXPORT Q_DECL_IMPORT
#endif