// Copyright (c) Kuba Szczodrzyński 2026-6-10.

#include "conn_priv.h"

#include <NetfindMessage.pb.h>

bool pb_encode_field_string(pb_ostream_t *stream, const pb_field_iter_t *field, void *const *arg) {
	const char *string = *arg;
	if (!string)
		return true;
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	if (!pb_encode_string(stream, (const void *)string, strlen(string)))
		return false;
	return true;
}

bool pb_decode_field_string(pb_istream_t *stream, const pb_field_t *field, void **arg) {
	char *string = malloc(stream->bytes_left + 1);
	if (!string)
		return false;
	string[stream->bytes_left] = '\0';
	if (!pb_read(stream, (void *)string, stream->bytes_left))
		return false;
	*arg = string;
	return true;
}

bool pb_encode_Message(pb_ostream_t *stream, const conn_msg_t *msg, unsigned int flags) {
	if (!msg->topic)
		return false;
	NetfindMessage message = {
		.has_type			= true,
		.type				= (MessageType)msg->type,
		.topic.funcs.encode = pb_encode_field_string,
		.topic.arg			= msg->topic,
		.value.funcs.encode = pb_encode_field_string,
		.value.arg			= msg->value,
		.has_created_at		= msg->type == CONN_MSG_PUB,
		.created_at			= msg->created_at,
		.has_updated_at		= msg->type == CONN_MSG_PUB,
		.updated_at			= msg->updated_at,
		.has_mode			= msg->type == CONN_MSG_PUB,
		.mode				= (PublishMode)msg->mode,
	};
	if (!pb_encode_ex(stream, &NetfindMessage_msg, &message, flags))
		return false;
	return true;
}

bool pb_decode_Message(pb_istream_t *stream, conn_msg_t *msg, unsigned int flags) {
	NetfindMessage message = {
		.topic.funcs.decode = pb_decode_field_string,
		.value.funcs.decode = pb_decode_field_string,
	};
	if (!pb_decode_ex(stream, &NetfindMessage_msg, &message, flags))
		goto err;

	if (!message.has_type)
		goto err;
	msg->type = (conn_msg_type_t)message.type;

	if (!message.topic.arg)
		goto err;
	msg->topic = message.topic.arg;
	msg->value = message.value.arg;

	if (message.type == MessageType_PUB && !message.has_created_at)
		goto err;
	msg->created_at = message.created_at;

	if (message.type == MessageType_PUB && !message.has_updated_at)
		goto err;
	msg->updated_at = message.updated_at;

	if (message.type == MessageType_PUB && !message.has_mode)
		goto err;
	msg->mode = (conn_pub_mode_t)message.mode;

	return true;

err:
	free(message.topic.arg);
	free(message.value.arg);
	return false;
}

bool pb_encode_field_messages(pb_ostream_t *stream, const pb_field_iter_t *field, void *const *arg) {
	const conn_msg_t *msgs = *arg;
	const conn_msg_t *msg  = NULL;
	DL_FOREACH(msgs, msg) {
		if (!pb_encode_tag_for_field(stream, field))
			return false;
		if (!pb_encode_Message(stream, msg, PB_ENCODE_DELIMITED))
			return false;
	}
	return true;
}

bool pb_decode_field_messages(pb_istream_t *stream, const pb_field_iter_t *field, void **arg) {
	conn_msg_t *msgs = *arg;
	conn_msg_t *msg	 = calloc(1, sizeof(*msg));
	if (!msg)
		return false;
	if (!pb_decode_Message(stream, msg, 0))
		goto err;
	DL_APPEND(msgs, msg);
	*arg = msgs;
	return true;

err:
	conn_msg_free(msg);
	return false;
}

bool pb_encode_MessageList(pb_ostream_t *stream, const conn_msg_t *msgs, unsigned int flags) {
	NetfindMessageList message_list = {
		.messages.funcs.encode = pb_encode_field_messages,
		.messages.arg		   = (void *)msgs,
	};
	if (!pb_encode_ex(stream, &NetfindMessageList_msg, &message_list, flags))
		return false;
	return true;
}

bool pb_decode_MessageList(pb_istream_t *stream, conn_msg_t **msgs, unsigned int flags) {
	NetfindMessageList message_list = {
		.messages.funcs.decode = pb_decode_field_messages,
		.messages.arg		   = NULL,
	};
	if (!pb_decode_ex(stream, &NetfindMessageList_msg, &message_list, flags))
		goto err;
	*msgs = message_list.messages.arg;
	return true;

err:
	conn_msgs_free(message_list.messages.arg);
	return false;
}
