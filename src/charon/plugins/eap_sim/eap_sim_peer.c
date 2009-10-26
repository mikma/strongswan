/*
 * Copyright (C) 2007-2009 Martin Willi
 * Hochschule fuer Technik Rapperswil
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

#include "eap_sim_peer.h"

#include <daemon.h>

#include <simaka_message.h>

/* number of tries we do authenticate */
#define MAX_TRIES 3

/* number of triplets for one authentication */
#define TRIPLET_COUNT 3

/** length of the AT_NONCE_MT/AT_NONCE_S nonce value */
#define NONCE_LEN 16

typedef struct private_eap_sim_peer_t private_eap_sim_peer_t;

/**
 * Private data of an eap_sim_peer_t object.
 */
struct private_eap_sim_peer_t {

	/**
	 * Public authenticator_t interface.
	 */
	eap_sim_peer_t public;

	/**
	 * permanent ID of peer
	 */
	identification_t *permanent;

	/**
	 * Pseudonym identity the peer uses
	 */
	identification_t *pseudonym;

	/**
	 * EAP-SIM crypto helper
	 */
	simaka_crypto_t *crypto;

	/**
	 * how many times we try to authenticate
	 */
	int tries;

	/**
	 * version list received from server
	 */
	chunk_t version_list;

	/**
	 * Nonce value used in AT_NONCE_MT/AT_NONCE_S
	 */
	chunk_t nonce;

	/**
	 * MSK, used for EAP-SIM based IKEv2 authentication
	 */
	chunk_t msk;
};

/* version of SIM protocol we speak */
static chunk_t version = chunk_from_chars(0x00,0x01);

/**
 * Read a triplet from the SIM card
 */
static bool get_triplet(private_eap_sim_peer_t *this, identification_t *peer,
						char *rand, char *sres, char *kc)
{
	enumerator_t *enumerator;
	sim_card_t *card;
	bool success = FALSE;

	enumerator = charon->sim->create_card_enumerator(charon->sim);
	while (enumerator->enumerate(enumerator, &card))
	{
		if (card->get_triplet(card, peer, rand, sres, kc))
		{
			success = TRUE;
			break;
		}
	}
	enumerator->destroy(enumerator);
	if (!success)
	{
		DBG1(DBG_IKE, "no SIM card found with triplets for '%Y'", peer);
	}
	return success;
}

/**
 * Find a stored pseudonym on a SIM card
 */
static identification_t *get_pseudonym(private_eap_sim_peer_t *this)
{
	enumerator_t *enumerator;
	sim_card_t *card;
	identification_t *pseudonym = NULL;

	enumerator = charon->sim->create_card_enumerator(charon->sim);
	while (enumerator->enumerate(enumerator, &card))
	{
		pseudonym = card->get_pseudonym(card, this->permanent);
		if (pseudonym)
		{
			DBG1(DBG_IKE, "using stored pseudonym identity '%Y' "
				 "instead of '%Y'", pseudonym, this->permanent);
			break;
		}
	}
	enumerator->destroy(enumerator);
	return pseudonym;
}

/**
 * Store a pseudonym in a SIM card
 */
static void set_pseudonym(private_eap_sim_peer_t *this, chunk_t data)
{
	enumerator_t *enumerator;
	sim_card_t *card;
	identification_t *pseudonym;
	char buf[data.len + 1];

	snprintf(buf, sizeof(buf), "%.*s", data.len, data.ptr);
	pseudonym = identification_create_from_string(buf);
	DBG1(DBG_IKE, "received pseudonym '%Y' for next authentication", pseudonym);
	enumerator = charon->sim->create_card_enumerator(charon->sim);
	while (enumerator->enumerate(enumerator, &card))
	{
		card->set_pseudonym(card, this->permanent, pseudonym);
	}
	enumerator->destroy(enumerator);
	pseudonym->destroy(pseudonym);
}

/**
 * Create a SIM_CLIENT_ERROR
 */
static eap_payload_t* create_client_error(private_eap_sim_peer_t *this,
							u_int8_t identifier, simaka_client_error_t code)
{
	simaka_message_t *message;
	eap_payload_t *out;
	u_int16_t encoded;

	DBG1(DBG_IKE, "sending client error '%N'", simaka_client_error_names, code);

	message = simaka_message_create(FALSE, identifier, EAP_SIM,
									SIM_CLIENT_ERROR, this->crypto);
	encoded = htons(code);
	message->add_attribute(message, AT_CLIENT_ERROR_CODE,
						   chunk_create((char*)&encoded, sizeof(encoded)));
	out = message->generate(message, chunk_empty);
	message->destroy(message);
	return out;
}

/**
 * process an EAP-SIM/Request/Start message
 */
static status_t process_start(private_eap_sim_peer_t *this,
							  simaka_message_t *in, eap_payload_t **out)
{
	simaka_message_t *message;
	enumerator_t *enumerator;
	simaka_attribute_t type;
	chunk_t data, id;
	rng_t *rng;
	bool supported = FALSE;
	simaka_attribute_t id_req = 0;

	enumerator = in->create_attribute_enumerator(in);
	while (enumerator->enumerate(enumerator, &type, &data))
	{
		switch (type)
		{
			case AT_VERSION_LIST:
			{
				free(this->version_list.ptr);
				this->version_list = chunk_clone(data);
				while (data.len >= version.len)
				{
					if (memeq(data.ptr, version.ptr, version.len))
					{
						supported = TRUE;
						break;
					}
				}
				break;
			}
			case AT_ANY_ID_REQ:
			case AT_FULLAUTH_ID_REQ:
			case AT_PERMANENT_ID_REQ:
				id_req = type;
				break;
			default:
				if (!simaka_attribute_skippable(type))
				{
					*out = create_client_error(this, in->get_identifier(in),
											   SIM_UNABLE_TO_PROCESS);
					enumerator->destroy(enumerator);
					return NEED_MORE;
				}
				break;
		}
	}
	enumerator->destroy(enumerator);

	if (!supported)
	{
		DBG1(DBG_IKE, "server does not support EAP-SIM version number 1");
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_UNSUPPORTED_VERSION);
		return NEED_MORE;
	}

	switch (id_req)
	{
		case AT_ANY_ID_REQ:
			/* TODO: reauth handling */
		case AT_FULLAUTH_ID_REQ:
			DESTROY_IF(this->pseudonym);
			this->pseudonym = get_pseudonym(this);
			if (this->pseudonym)
			{
				id = this->pseudonym->get_encoding(this->pseudonym);
				break;
			}
			/* FALL */
		case AT_PERMANENT_ID_REQ:
			id = this->permanent->get_encoding(this->permanent);
			break;
		default:
			break;
	}

	/* generate AT_NONCE_MT value */
	rng = this->crypto->get_rng(this->crypto);
	free(this->nonce.ptr);
	rng->allocate_bytes(rng, NONCE_LEN, &this->nonce);

	message = simaka_message_create(FALSE, in->get_identifier(in), EAP_SIM,
									SIM_START, this->crypto);
	message->add_attribute(message, AT_SELECTED_VERSION, version);
	message->add_attribute(message, AT_NONCE_MT, this->nonce);
	if (id.len)
	{
		message->add_attribute(message, AT_IDENTITY, id);
	}
	*out = message->generate(message, chunk_empty);
	message->destroy(message);

	return NEED_MORE;
}

/**
 * process an EAP-SIM/Request/Challenge message
 */
static status_t process_challenge(private_eap_sim_peer_t *this,
								  simaka_message_t *in, eap_payload_t **out)
{
	simaka_message_t *message;
	enumerator_t *enumerator;
	simaka_attribute_t type;
	chunk_t data, rands = chunk_empty, kcs, kc, sreses, sres;
	identification_t *peer;

	if (this->tries-- <= 0)
	{
		/* give up without notification. This hack is required as some buggy
		 * server implementations won't respect our client-error. */
		return FAILED;
	}

	enumerator = in->create_attribute_enumerator(in);
	while (enumerator->enumerate(enumerator, &type, &data))
	{
		switch (type)
		{
			case AT_RAND:
				rands = data;
				break;
			default:
				if (!simaka_attribute_skippable(type))
				{
					*out = create_client_error(this, in->get_identifier(in),
											   SIM_UNABLE_TO_PROCESS);
					enumerator->destroy(enumerator);
					return NEED_MORE;
				}
				break;
		}
	}
	enumerator->destroy(enumerator);

	peer = this->permanent;
	if (this->pseudonym)
	{
		peer = this->pseudonym;
	}

	/* excepting two or three RAND, each 16 bytes. We require two valid
	 * and different RANDs */
	if ((rands.len != 2 * SIM_RAND_LEN && rands.len != 3 * SIM_RAND_LEN) ||
		memeq(rands.ptr, rands.ptr + SIM_RAND_LEN, SIM_RAND_LEN))
	{
		DBG1(DBG_IKE, "no valid AT_RAND received");
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_INSUFFICIENT_CHALLENGES);
		return NEED_MORE;
	}
	/* get two or three KCs/SRESes from SIM using RANDs */
	kcs = kc = chunk_alloca(rands.len / 2);
	sreses = sres = chunk_alloca(rands.len / 4);
	while (rands.len >= SIM_RAND_LEN)
	{
		if (!get_triplet(this, peer, rands.ptr, sres.ptr, kc.ptr))
		{
			DBG1(DBG_IKE, "unable to get EAP-SIM triplet");
			*out = create_client_error(this, in->get_identifier(in),
									   SIM_UNABLE_TO_PROCESS);
			return NEED_MORE;
		}
		DBG3(DBG_IKE, "got triplet for RAND %b\n  Kc %b\n  SRES %b",
			 rands.ptr, SIM_RAND_LEN, sres.ptr, SIM_SRES_LEN, kc.ptr, SIM_KC_LEN);
		kc = chunk_skip(kc, SIM_KC_LEN);
		sres = chunk_skip(sres, SIM_SRES_LEN);
		rands = chunk_skip(rands, SIM_RAND_LEN);
	}

	data = chunk_cata("cccc", kcs, this->nonce, this->version_list, version);
	free(this->msk.ptr);
	this->msk = this->crypto->derive_keys_full(this->crypto, peer, data);

	/* Verify AT_MAC attribute, signature is over "EAP packet | NONCE_MT", and
	 * parse() again after key derivation, reading encrypted attributes */
	if (!in->verify(in, this->nonce) || !in->parse(in))
	{
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_UNABLE_TO_PROCESS);
		return NEED_MORE;
	}

	enumerator = in->create_attribute_enumerator(in);
	while (enumerator->enumerate(enumerator, &type, &data))
	{
		switch (type)
		{
			case AT_NEXT_PSEUDONYM:
				set_pseudonym(this, data);
				break;
			default:
				break;
		}
	}
	enumerator->destroy(enumerator);

	/* build response with AT_MAC, built over "EAP packet | n*SRES" */
	message = simaka_message_create(FALSE, in->get_identifier(in), EAP_SIM,
									SIM_CHALLENGE, this->crypto);
	*out = message->generate(message, sreses);
	message->destroy(message);
	return NEED_MORE;
}

/**
 * process an EAP-SIM/Request/Notification message
 */
static status_t process_notification(private_eap_sim_peer_t *this,
									 simaka_message_t *in, eap_payload_t **out)
{
	simaka_message_t *message;
	enumerator_t *enumerator;
	simaka_attribute_t type;
	chunk_t data;
	bool success = TRUE;

	enumerator = in->create_attribute_enumerator(in);
	while (enumerator->enumerate(enumerator, &type, &data))
	{
		if (type == AT_NOTIFICATION)
		{
			u_int16_t code;

			memcpy(&code, data.ptr, sizeof(code));
			code = ntohs(code);

			/* test success bit */
			if (!(data.ptr[0] & 0x80))
			{
				success = FALSE;
				DBG1(DBG_IKE, "received EAP-SIM notification error '%N'",
					 simaka_notification_names, code);
			}
			else
			{
				DBG1(DBG_IKE, "received EAP-SIM notification '%N'",
					 simaka_notification_names, code);
			}
		}
		else if (!simaka_attribute_skippable(type))
		{
			success = FALSE;
			break;
		}
	}
	enumerator->destroy(enumerator);

	if (success)
	{	/* empty notification reply */
		message = simaka_message_create(FALSE, in->get_identifier(in), EAP_SIM,
										SIM_NOTIFICATION, this->crypto);
		*out = message->generate(message, chunk_empty);
		message->destroy(message);
	}
	else
	{
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_UNABLE_TO_PROCESS);
	}
	return NEED_MORE;
}

/**
 * Implementation of eap_method_t.process
 */
static status_t process(private_eap_sim_peer_t *this,
						eap_payload_t *in, eap_payload_t **out)
{
	simaka_message_t *message;
	status_t status;

	message = simaka_message_create_from_payload(in, this->crypto);
	if (!message)
	{
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_UNABLE_TO_PROCESS);
		return NEED_MORE;
	}
	if (!message->parse(message))
	{
		message->destroy(message);
		*out = create_client_error(this, in->get_identifier(in),
								   SIM_UNABLE_TO_PROCESS);
		return NEED_MORE;
	}
	switch (message->get_subtype(message))
	{
		case SIM_START:
			status = process_start(this, message, out);
			break;
		case SIM_CHALLENGE:
			status = process_challenge(this, message, out);
			break;
		case SIM_NOTIFICATION:
			status = process_notification(this, message, out);
			break;
		default:
			DBG1(DBG_IKE, "unable to process EAP-SIM subtype %N",
				 simaka_subtype_names, message->get_subtype(message));
			*out = create_client_error(this, in->get_identifier(in),
									   SIM_UNABLE_TO_PROCESS);
			status = NEED_MORE;
			break;
	}
	message->destroy(message);
	return status;
}

/**
 * Implementation of eap_method_t.initiate
 */
static status_t initiate(private_eap_sim_peer_t *this, eap_payload_t **out)
{
	/* peer never initiates */
	return FAILED;
}

/**
 * Implementation of eap_method_t.get_type.
 */
static eap_type_t get_type(private_eap_sim_peer_t *this, u_int32_t *vendor)
{
	*vendor = 0;
	return EAP_SIM;
}

/**
 * Implementation of eap_method_t.get_msk.
 */
static status_t get_msk(private_eap_sim_peer_t *this, chunk_t *msk)
{
	if (this->msk.ptr)
	{
		*msk = this->msk;
		return SUCCESS;
	}
	return FAILED;
}

/**
 * Implementation of eap_method_t.is_mutual.
 */
static bool is_mutual(private_eap_sim_peer_t *this)
{
	return TRUE;
}

/**
 * Implementation of eap_method_t.destroy.
 */
static void destroy(private_eap_sim_peer_t *this)
{
	this->permanent->destroy(this->permanent);
	DESTROY_IF(this->pseudonym);
	this->crypto->destroy(this->crypto);
	free(this->version_list.ptr);
	free(this->nonce.ptr);
	free(this->msk.ptr);
	free(this);
}

/*
 * Described in header.
 */
eap_sim_peer_t *eap_sim_peer_create(identification_t *server,
									identification_t *peer)
{
	private_eap_sim_peer_t *this = malloc_thing(private_eap_sim_peer_t);

	this->public.interface.initiate = (status_t(*)(eap_method_t*,eap_payload_t**))initiate;
	this->public.interface.process = (status_t(*)(eap_method_t*,eap_payload_t*,eap_payload_t**))process;
	this->public.interface.get_type = (eap_type_t(*)(eap_method_t*,u_int32_t*))get_type;
	this->public.interface.is_mutual = (bool(*)(eap_method_t*))is_mutual;
	this->public.interface.get_msk = (status_t(*)(eap_method_t*,chunk_t*))get_msk;
	this->public.interface.destroy = (void(*)(eap_method_t*))destroy;

	this->crypto = simaka_crypto_create();
	if (!this->crypto)
	{
		free(this);
		return NULL;
	}
	this->permanent = peer->clone(peer);
	this->pseudonym = NULL;
	this->tries = MAX_TRIES;
	this->version_list = chunk_empty;
	this->nonce = chunk_empty;
	this->msk = chunk_empty;

	return &this->public;
}

