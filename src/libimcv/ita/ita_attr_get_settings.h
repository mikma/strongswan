/*
 * Copyright (C) 2012 Andreas Steffen
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup ita_attr_get_settingst ita_attr_get_settings
 * @{ @ingroup ita_attr_get_settings
 */

#ifndef ITA_ATTR_GET_SETTINGS_H_
#define ITA_ATTR_GET_SETTINGS_H_

typedef struct ita_attr_get_settings_t ita_attr_get_settings_t;

#include "pa_tnc/pa_tnc_attr.h"

/**
 * Class implementing the ITA Get Settings PA-TNC attribute.
 *
 */
struct ita_attr_get_settings_t {

	/**
	 * Public PA-TNC attribute interface
	 */
	pa_tnc_attr_t pa_tnc_attribute;

	/**
	 * Add a setting request to the list
	 *
	 * @param name			name of the requested setting
	 */
	void (*add)(ita_attr_get_settings_t *this, char *name);

	/**
	 * Return an enumerator over all requested settings
	 *
	 * @return				enumerator returns char *name
	 */
	enumerator_t* (*create_enumerator)(ita_attr_get_settings_t *this);
};

/**
 * Creates an ita_attr_get_settings_t object with an empty settings list
 */
pa_tnc_attr_t* ita_attr_get_settings_create(void);

/**
 * Creates an ita_attr_get_settings_t object from received data
 *
 * @param value				binary value blob
 */
pa_tnc_attr_t* ita_attr_get_settings_create_from_data(chunk_t value);

#endif /** ITA_ATTR_GET_SETTINGS_H_ @}*/
