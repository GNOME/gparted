#ifndef I18N 
#define I18N

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_NLS
#include <libintl.h> 
#define _(String) gettext(String)
#endif /* ENABLE_NLS */

#endif /* I18N */
