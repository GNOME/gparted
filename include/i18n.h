#ifndef I18N 
#define I18N

	#ifdef HAVE_CONFIG_H
		#include "config.h"
	#endif /* HAVE_CONFIG_H */
	
	#ifdef ENABLE_NLS
		#include <glibmm/i18n.h>
		
		//compose library, dedicated to the translators :P
		#include "../compose/ucompose.hpp"
	#endif /* ENABLE_NLS */

#endif /* I18N */
