/*!
 * \file sick_message.c
 * \brief Implements the sick_message_t
 *
 * Code by Jason C. Derenick and Thomas H. Miller.
 * Contact derenick(at)lehigh(dot)edu
 *
 * The Sick LIDAR Matlab/C++ Toolbox
 * Copyright (c) 2008, Jason C. Derenick and Thomas H. Miller
 * All rights reserved.
 *
 * This software is released under a BSD Open-Source License.
 * See http://sicktoolbox.sourceforge.net
 */

/* Implementation dependencies */
#include <sickld/sick_message.h>
#include <comm/hd_utils.h>
#include <sickld/sick_base.h> // for byye-order conversions where necessary

/**
 * \brief constructor
 * \param[in,out] *skm The sick_message_t
 * \return E_RESULT,E_ERROR_BAD_ALLOCATE if failed.
 */
e_int32 skm_create(sick_message_t *skm) {
	e_assert(skm, E_ERROR_BAD_ALLOCATE);
	memset(skm, 0, sizeof(sick_message_t));
	skm->state = 1;
	skm_clear(skm);
	return E_OK;
}

/**
 * \brief destructor
 * \param[in,out] *skm The sick_message_t
 * \return
 */
void skm_release(sick_message_t *skm) {
	memset(skm, 0, sizeof(sick_message_t));
}

e_int32 skm_state(sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR);
	return E_OK;
}

/**
 * \brief clear
 * \param[in] *skm The sick_message_t pointer
 * \return
 */
void skm_clear(sick_message_t *skm) {
	e_assert(skm&&skm->state);
	/* Reset the parent integer variables */
	skm->message_length = skm->payload_length = 0;

	/* Clear the message buffer */
	memset(skm->message_buffer, 0, MESSAGE_MAX_LENGTH);

	/* Set the flag indicating this message object/container is empty */
	skm->populated = false;
}

/*拷贝消息*/
void skm_copy(sick_message_t *skm_to, const sick_message_t *skm_from) {
	e_assert(skm_from&&skm_to);
	memcpy(skm_to, skm_from, sizeof(sick_message_t));
}

/**
 * \brief Constructs a Sick message given the parameter values
 * \param[in] skm The sick_message_t pointer
 * \param[in] *payload_buffer The payload of the message as an array of bytes
 * \param[in] payload_length The length of the payload in bytes
 * \return E_RESULT,E_ERROR_INVALID_HANDLER if skm is NULL
 */
e_int32 skm_build_message(sick_message_t *skm,
		const e_uint8 * const payload_buffer, const e_uint32 payload_length) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	/* Clear the object */
	skm_clear(skm);

	/* Assign the payload and message lengths */
	skm->payload_length = payload_length;
	skm->message_length = MESSAGE_HEADER_LENGTH + MESSAGE_TRAILER_LENGTH
			+ skm->payload_length;

	/* Copy the payload into the message buffer */
	memcpy(&skm->message_buffer[MESSAGE_HEADER_LENGTH], payload_buffer,
			skm->payload_length);

	/* Mark the object container as being populated */
	skm->populated = true;

	/*
	 * Set the message header!
	 */
	skm->message_buffer[0] = 0x02; // STX
	skm->message_buffer[1] = 'U'; // User
	skm->message_buffer[2] = 'S'; // Service
	skm->message_buffer[3] = 'P'; // Protocol

	/* Include the payload length in the header */
	e_uint32 payload_length_32 = host_to_sick_ld_byte_order32(
			(e_uint32) skm->payload_length);
	memcpy(&skm->message_buffer[4], &payload_length_32, 4);

	/*
	 * Set the message trailer (just a checksum)!
	 */
	skm->message_buffer[skm->message_length - 1] = hd_compute_xor(
			&skm->message_buffer[8], (e_uint32) skm->payload_length);

	return E_OK;
}

/**
 * \brief Parses a sequence of bytes into a SickLDMessage object
 * \param[in] skm The sick_message_t pointer
 * \param[in] *message_buffer A well-formed message to be parsed into the class' fields
 * \return E_RESULT,E_ERROR_INVALID_HANDLER if skm is NULL
 */
e_int32 skm_parse_message(sick_message_t *skm,
		const e_uint8 * const message_buffer) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	/* Clear the message container/object */
	skm_clear(skm);

	/* Mark the object as populated */
	skm->populated = true;

	/* Extract the payload length */
	e_uint32 payload_length_32 = 0;
	memcpy(&payload_length_32, &message_buffer[4], 4);
	skm->payload_length = (e_uint32) sick_ld_to_host_byte_order32(
			payload_length_32);

	/* Compute the total message length */
	skm->message_length = MESSAGE_HEADER_LENGTH + MESSAGE_TRAILER_LENGTH
			+ skm->payload_length;

	/* Copy the given packet into the buffer */
	memcpy(skm->message_buffer, message_buffer, skm->message_length);
	return E_OK;
}

/**
 * \brief dump the message content
 * \param[in] *skm The sick_message_t pointer
 * \return
 */
void skm_dump(const sick_message_t *skm) {
	e_uint32 i;
	e_assert((skm&&skm->state));
	DMSG((STDOUT, "Sick Message Dump:\r\n"));
	DMSG((STDOUT, "Payload length:%u\r\n",(unsigned int)skm->payload_length));
	DMSG((STDOUT, "Message length:%u\r\n",(unsigned int)skm->message_length));
	DMSG((STDOUT, "Message (hex):"));
	for (i = 0; i < skm->message_length; i++) {
		DMSG((STDOUT,"%X ",(int)skm->message_buffer[i]));
	}
	DMSG((STDOUT, "\r\n"));

	DMSG((STDOUT, "Message (ASCII):"));
	for (i = 0; i < skm->message_length; i++) {
		DMSG((STDOUT,"%X ",skm->message_buffer[i]));
	}
	DMSG((STDOUT,"\r\n"));
	DMSG((STDOUT, "Checksum:%u\r\n", (unsigned int) skm_get_checksum(skm)));
	DMSG(
			(STDOUT, "Service code:%u\r\n", (unsigned int) skm_get_service_code(skm)));
	DMSG(
			(STDOUT, "Service subcode:%u\r\n", (unsigned int) skm_get_service_subcode(skm)));
	DMSG((STDOUT, "\r\n"));
}

/**
 * \brief Get the length of the service code associated with the message
 * \param[in] *skm The sick_message_t pointer
 * \return E_ERROR_INVALID_VALUE if skm is NULL
 */
e_uint8 skm_get_service_code(const sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR);
	return skm->message_buffer[8];
}

/**
 * \brief Get the service sub-code associated with the message
 * \param[in] *skm The sick_message_t pointer
 * \return  E_ERROR_INVALID_VALUE if skm is NULL
 */
e_uint8 skm_get_service_subcode(const sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR);
	return skm->message_buffer[9];
}

/**
 * \brief Get the checksum for the packet
 * \param[in] *skm The sick_message_t pointer
 * \return  E_ERROR_INVALID_VALUE if skm is NULL
 */
e_uint8 skm_get_checksum(const sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR);
	return skm->message_buffer[skm->message_length - 1];
}

/**
 * \brief Get the populated for the packet
 * \param[in] *skm The sick_message_t pointer
 * \return  E_ERROR_INVALID_VALUE if skm is NULL
 */
e_uint8 skm_get_populated(const sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR);
	return skm->populated;
}

/**
 * \brief Get the message as a sequence of well-formed bytes
 * \param[in] *skm The sick_message_t pointer
 * \param *message_buffer Destination buffer for message contents
 * \return E_ERROR_INVALID_HANDLER if skm is NULL
 */
e_int32 skm_get_message(const sick_message_t *skm,
		e_uint8* const message_buffer) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	memcpy(message_buffer, skm->message_buffer, skm->message_length);
	return E_OK;
}

e_int32 skm_get_message_length(const sick_message_t *skm) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	return skm->message_length;
}

/**
 * \brief Get the payload contents as a sequence of well-formed bytes
 * \param[in] *skm The sick_message_t pointer
 * \param *payload_buffer Destination buffer for message payload contents
 * \return E_ERROR_INVALID_HANDLER if skm is NULL
 */
e_int32 skm_get_payload(const sick_message_t *skm,
		e_uint8 * const payload_buffer) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	memcpy(payload_buffer, &skm->message_buffer[MESSAGE_HEADER_LENGTH],
			skm->payload_length);
	return E_OK;
}

/***
 * \brief Get a specified sub-region of the payload buffer
 * \param[in] *skm The sick_message_t pointer
 * \param *payload_sub_buffer Destination buffer for message payload contents
 * \param start_idx The 0-indexed starting location for copying
 * \param stop_idx The 0-indexed stopping location for copying
 * \return E_ERROR_INVALID_HANDLER if skm is NULL
 */
e_int32 skm_get_payload_subregion(const sick_message_t *skm,
		e_uint8 * const payload_sub_buffer, const e_uint32 start_idx,
		const e_uint32 stop_idx) {
	e_assert((skm&&skm->state), E_ERROR_INVALID_HANDLER);
	/* Extract the subregion */
	memcpy(payload_sub_buffer,
			&skm->message_buffer[MESSAGE_HEADER_LENGTH + start_idx],
			stop_idx + 1);
	return E_OK;
}

