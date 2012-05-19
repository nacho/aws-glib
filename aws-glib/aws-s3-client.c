/* aws-s3-client.c
 *
 * Copyright (C) 2012 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <libsoup/soup-date.h>

#include "aws-s3-client.h"

G_DEFINE_TYPE(AwsS3Client, aws_s3_client, SOUP_TYPE_SESSION_ASYNC)

struct _AwsS3ClientPrivate
{
   AwsCredentials *creds;
   gchar *host;
   guint16 port;
   gboolean port_set;
   gboolean secure;
};

enum
{
   PROP_0,
   PROP_CREDENTIALS,
   PROP_HOST,
   PROP_PORT,
   PROP_PORT_SET,
   PROP_SECURE,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

/**
 * aws_s3_client_get_credentials:
 * @client: (in): A #AwsS3Client.
 *
 * Fetches the #AwsCredentials for @client.
 *
 * Returns: (transfer none): An #AwsCredentials.
 */
AwsCredentials *
aws_s3_client_get_credentials (AwsS3Client *client)
{
   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), NULL);
   return client->priv->creds;
}

void
aws_s3_client_set_credentials (AwsS3Client    *client,
                               AwsCredentials *credentials)
{
   AwsS3ClientPrivate *priv;

   g_return_if_fail(AWS_IS_S3_CLIENT(client));
   g_return_if_fail(!credentials || AWS_IS_CREDENTIALS(credentials));

   priv = client->priv;

   g_clear_object(&priv->creds);
   priv->creds = g_object_ref(credentials);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_CREDENTIALS]);
}

const gchar *
aws_s3_client_get_host (AwsS3Client *client)
{
   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), NULL);
   return client->priv->host;
}

void
aws_s3_client_set_host (AwsS3Client *client,
                        const gchar *host)
{
   g_return_if_fail(AWS_IS_S3_CLIENT(client));

   g_free(client->priv->host);
   client->priv->host = g_strdup(host);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_HOST]);
}

guint16
aws_s3_client_get_port (AwsS3Client *client)
{
   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), 0);
   return client->priv->port;
}

void
aws_s3_client_set_port (AwsS3Client *client,
                        guint16      port)
{
   g_return_if_fail(AWS_IS_S3_CLIENT(client));
   client->priv->port = port;
   client->priv->port_set = (port != 0);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_PORT]);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_PORT_SET]);
}

gboolean
aws_s3_client_get_port_set (AwsS3Client *client)
{
   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), FALSE);
   return client->priv->port_set;
}

gboolean
aws_s3_client_get_secure (AwsS3Client *client)
{
   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), FALSE);
   return client->priv->secure;
}

void
aws_s3_client_set_secure (AwsS3Client *client,
                          gboolean     secure)
{
   g_return_if_fail(AWS_IS_S3_CLIENT(client));
   client->priv->secure = secure;
   g_object_notify_by_pspec(G_OBJECT(client),
                            gParamSpecs[PROP_SECURE]);
}

static void
aws_s3_client_read_cb (SoupSession *session,
                       SoupMessage *message,
                       gpointer     user_data)
{
   GSimpleAsyncResult *simple = user_data;

   g_assert(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (message->status_code == SOUP_STATUS_CANCELLED) {
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      return;
   }

   if (SOUP_STATUS_IS_SUCCESSFUL(message->status_code)) {
      g_print("Got response back. %d\n", message->status_code);
      g_simple_async_result_set_op_res_gboolean(simple, TRUE);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
   }
}

static void
aws_s3_client_read_got_chunk (SoupMessage        *message,
                              SoupBuffer         *buffer,
                              GSimpleAsyncResult *simple)
{
   AwsS3ClientDataHandler handler;
   gpointer handler_data;
   GObject *session;

   g_assert(SOUP_IS_MESSAGE(message));
   g_assert(buffer);
   g_assert(G_IS_SIMPLE_ASYNC_RESULT(simple));

   session = g_async_result_get_source_object(G_ASYNC_RESULT(simple));
   g_assert(AWS_IS_S3_CLIENT(session));

   handler = g_object_get_data(G_OBJECT(simple), "handler");
   handler_data = g_object_get_data(G_OBJECT(simple), "handler-data");
   g_assert(handler);

   if (!handler(AWS_S3_CLIENT(session), message, buffer, handler_data)) {
      g_simple_async_result_set_error(simple,
                                      AWS_S3_CLIENT_ERROR,
                                      AWS_S3_CLIENT_ERROR_CANCELLED,
                                      _("The request was cancelled."));
      soup_session_cancel_message(SOUP_SESSION(session), message,
                                  SOUP_STATUS_CANCELLED);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      return;
   }
}

static void
aws_s3_client_read_got_headers (SoupMessage        *message,
                                GSimpleAsyncResult *simple)
{
   GObject *session;

   g_assert(SOUP_IS_MESSAGE(message));
   g_assert(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (!SOUP_STATUS_IS_SUCCESSFUL(message->status_code)) {
      session = g_async_result_get_source_object(G_ASYNC_RESULT(simple));

      /*
       * Extract the given error type.
       */
      if (message->status_code == SOUP_STATUS_NOT_FOUND) {
         g_simple_async_result_set_error(simple,
                                         AWS_S3_CLIENT_ERROR,
                                         AWS_S3_CLIENT_ERROR_NOT_FOUND,
                                         _("The requested object was not found."));
         soup_session_cancel_message(SOUP_SESSION(session), message,
                                     message->status_code);
      } else if (SOUP_STATUS_IS_CLIENT_ERROR(message->status_code)) {
         g_simple_async_result_set_error(simple,
                                         AWS_S3_CLIENT_ERROR,
                                         AWS_S3_CLIENT_ERROR_BAD_REQUEST,
                                         _("The request was invalid."));
         soup_session_cancel_message(SOUP_SESSION(session), message,
                                     message->status_code);
      } else {
         g_simple_async_result_set_error(simple,
                                         AWS_S3_CLIENT_ERROR,
                                         AWS_S3_CLIENT_ERROR_UNKNOWN,
                                         _("An unknown error occurred."));
         soup_session_cancel_message(SOUP_SESSION(session), message,
                                     SOUP_STATUS_CANCELLED);
      }

      /*
       * Complete the async result from the main loop.
       */
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);

      return;
   }
}

void
aws_s3_client_read_async (AwsS3Client            *client,
                          const gchar            *bucket,
                          const gchar            *path,
                          AwsS3ClientDataHandler  handler,
                          gpointer                handler_data,
                          GDestroyNotify          handler_notify,
                          GCancellable           *cancellable,
                          GAsyncReadyCallback     callback,
                          gpointer                user_data)
{
   AwsS3ClientPrivate *priv;
   GSimpleAsyncResult *simple;
   SoupMessage *msg;
   SoupDate *date;
   GString *str;
   guint16 port;
   gchar *auth;
   gchar *date_str;
   gchar *signature;
   gchar *uri;

   g_return_if_fail(AWS_IS_S3_CLIENT(client));
   g_return_if_fail(bucket);
   g_return_if_fail(path);
   g_return_if_fail(g_utf8_validate(bucket, -1, NULL));
   g_return_if_fail(g_utf8_validate(path, -1, NULL));
   g_return_if_fail(handler);
   g_return_if_fail(!cancellable || G_IS_CANCELLABLE(cancellable));

   priv = client->priv;

   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      aws_s3_client_read_async);
   g_object_set_data(G_OBJECT(simple), "handler", handler);
   if (handler_notify) {
      g_object_set_data_full(G_OBJECT(simple), "handler-data",
                             handler_data, handler_notify);
   } else {
      g_object_set_data(G_OBJECT(simple), "handler-data", handler_data);
   }

   /*
    * Strip leading '/' from the path.
    */
   while (g_utf8_get_char(path) == '/') {
      path = g_utf8_next_char(path);
   }

   /*
    * Determine our connection port.
    */
   port = priv->port_set ? priv->port : (priv->secure ? 443 : 80);

   /*
    * Build our HTTP request message.
    */
   uri = g_strdup_printf("%s://%s:%d/%s/%s",
                         priv->secure ? "https" : "http",
                         priv->host,
                         port,
                         bucket,
                         path);
   msg = soup_message_new(SOUP_METHOD_GET, uri);
   soup_message_body_set_accumulate(msg->response_body, FALSE);
   g_signal_connect(msg,
                    "got-chunk",
                    G_CALLBACK(aws_s3_client_read_got_chunk),
                    simple);
   g_signal_connect(msg,
                    "got-headers",
                    G_CALLBACK(aws_s3_client_read_got_headers),
                    simple);

   /*
    * Set the Host header for systems that may be proxying.
    */
   soup_message_headers_append(msg->request_headers,
                               "Host", "s3.amazonaws.com");

   /*
    * Add the Date header which we need for signing.
    */
   date = soup_date_new_from_now(0);
   date_str = soup_date_to_string(date, SOUP_DATE_HTTP);
   soup_message_headers_append(msg->request_headers, "Date", date_str);

   /*
    * Sign our request.
    */
   str = g_string_new("GET\n\n\n");
   g_string_append_printf(str, "%s\n", date_str);
   g_string_append_printf(str, "/%s/%s", bucket, path);
   signature = aws_credentials_sign(priv->creds, str->str, str->len,
                                    G_CHECKSUM_SHA1);

   /*
    * Attach request signature to our headers.
    */
   auth = g_strdup_printf("AWS %s:%s",
                          aws_credentials_get_access_key(priv->creds),
                          signature);
   soup_message_headers_append(msg->request_headers, "Authorization", auth);

   /*
    * Submit our request to the target.
    */
   soup_session_queue_message(SOUP_SESSION(client), msg,
                              aws_s3_client_read_cb,
                              simple);

   /*
    * Cleanup.
    */
   g_free(auth);
   g_free(date_str);
   g_free(signature);
   g_free(uri);
   g_string_free(str, TRUE);
   soup_date_free(date);
}

gboolean
aws_s3_client_read_finish (AwsS3Client   *client,
                           GAsyncResult  *result,
                           GError       **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   return ret;
}

void
aws_s3_client_write_async (AwsS3Client         *client,
                           const gchar         *bucket,
                           const gchar         *path,
                           GInputStream        *stream,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
   g_return_if_fail(AWS_IS_S3_CLIENT(client));
   g_return_if_fail(bucket);
   g_return_if_fail(path);
   g_return_if_fail(G_IS_INPUT_STREAM(stream));
   g_return_if_fail(callback);

}

gboolean
aws_s3_client_write_finish (AwsS3Client   *client,
                            GAsyncResult  *result,
                            GError       **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   g_return_val_if_fail(AWS_IS_S3_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   return ret;
}

static void
aws_s3_client_finalize (GObject *object)
{
   AwsS3ClientPrivate *priv = AWS_S3_CLIENT(object)->priv;
   g_free(priv->host);
   g_clear_object(&priv->creds);
   G_OBJECT_CLASS(aws_s3_client_parent_class)->finalize(object);
}

static void
aws_s3_client_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
   AwsS3Client *client = AWS_S3_CLIENT(object);

   switch (prop_id) {
   case PROP_CREDENTIALS:
      g_value_set_object(value, aws_s3_client_get_credentials(client));
      break;
   case PROP_HOST:
      g_value_set_string(value, aws_s3_client_get_host(client));
      break;
   case PROP_PORT:
      g_value_set_uint(value, aws_s3_client_get_port(client));
      break;
   case PROP_PORT_SET:
      g_value_set_boolean(value, aws_s3_client_get_port_set(client));
      break;
   case PROP_SECURE:
      g_value_set_boolean(value, aws_s3_client_get_secure(client));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
aws_s3_client_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
   AwsS3Client *client = AWS_S3_CLIENT(object);

   switch (prop_id) {
   case PROP_CREDENTIALS:
      aws_s3_client_set_credentials(client, g_value_get_object(value));
      break;
   case PROP_HOST:
      aws_s3_client_set_host(client, g_value_get_string(value));
      break;
   case PROP_PORT:
      aws_s3_client_set_port(client, g_value_get_uint(value));
      break;
   case PROP_SECURE:
      aws_s3_client_set_secure(client, g_value_get_boolean(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
aws_s3_client_class_init (AwsS3ClientClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = aws_s3_client_finalize;
   object_class->get_property = aws_s3_client_get_property;
   object_class->set_property = aws_s3_client_set_property;
   g_type_class_add_private(object_class, sizeof(AwsS3ClientPrivate));

   gParamSpecs[PROP_CREDENTIALS] =
      g_param_spec_object("credentials",
                          _("Credentials"),
                          _("The credentials for your AWS account."),
                          AWS_TYPE_CREDENTIALS,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_CREDENTIALS,
                                   gParamSpecs[PROP_CREDENTIALS]);

   gParamSpecs[PROP_HOST] =
      g_param_spec_string("host",
                          _("Host"),
                          _("The hostname of the AWS server."),
                          "s3.amazonaws.com",
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_HOST,
                                   gParamSpecs[PROP_HOST]);

   gParamSpecs[PROP_PORT] =
      g_param_spec_uint("port",
                        _("Port"),
                        _("The port of the AWS server."),
                        1,
                        G_MAXUSHORT,
                        80,
                        G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_PORT,
                                   gParamSpecs[PROP_PORT]);

   gParamSpecs[PROP_PORT_SET] =
      g_param_spec_boolean("port-set",
                          _("Port Set"),
                          _("If the port number has been set."),
                          FALSE,
                          G_PARAM_READABLE);
   g_object_class_install_property(object_class, PROP_PORT_SET,
                                   gParamSpecs[PROP_PORT_SET]);

   gParamSpecs[PROP_SECURE] =
      g_param_spec_boolean("secure",
                          _("Secure"),
                          _("If HTTPS is required."),
                          TRUE,
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_SECURE,
                                   gParamSpecs[PROP_SECURE]);
}

static void
aws_s3_client_init (AwsS3Client *client)
{
   client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client,
                                              AWS_TYPE_S3_CLIENT,
                                              AwsS3ClientPrivate);
   client->priv->host = g_strdup_printf("s3.amazonaws.com");
   client->priv->creds = aws_credentials_new("", "");
   client->priv->secure = TRUE;
}

GQuark
aws_s3_client_error_quark (void)
{
   return g_quark_from_static_string("aws-s3-client-error-quark");
}
