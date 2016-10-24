/* aws-s3-client.h
 *
 * Copyright © 2012-2016 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <libsoup/soup-date.h>

#include "aws-s3-client.h"

typedef struct
{
  AwsCredentials *creds;
  gchar *host;
  guint16 port;
  guint port_set : 1;
  guint secure : 1;
} AwsS3ClientPrivate;

typedef struct
{
  AwsS3ClientDataHandler handler;
  gpointer               handler_data;
  GDestroyNotify         handler_data_destroy;
} ReadState;

G_DEFINE_TYPE_WITH_PRIVATE (AwsS3Client, aws_s3_client, SOUP_TYPE_SESSION)

enum {
  PROP_0,
  PROP_CREDENTIALS,
  PROP_HOST,
  PROP_PORT,
  PROP_PORT_SET,
  PROP_SECURE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
read_state_free (gpointer data)
{
  ReadState *state = data;

  if (state != NULL)
    {
      if (state->handler_data_destroy != NULL)
        g_clear_pointer (&state->handler_data, state->handler_data_destroy);
      g_slice_free (ReadState, state);
    }
}

static ReadState *
read_state_new (AwsS3ClientDataHandler handler,
                gpointer               handler_data,
                GDestroyNotify         handler_data_destroy)
{
  ReadState *state;

  state = g_slice_new0 (ReadState);
  state->handler = handler;
  state->handler_data = handler_data;
  state->handler_data_destroy = handler_data_destroy;

  return state;
}

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
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_val_if_fail(AWS_IS_S3_CLIENT(client), NULL);

  return priv->creds;
}

void
aws_s3_client_set_credentials (AwsS3Client    *client,
                               AwsCredentials *credentials)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_if_fail (AWS_IS_S3_CLIENT (client));
  g_return_if_fail (!credentials || AWS_IS_CREDENTIALS (credentials));

  if (g_set_object (&priv->creds, credentials))
    g_object_notify_by_pspec (G_OBJECT (client), properties [PROP_CREDENTIALS]);
}

const gchar *
aws_s3_client_get_host (AwsS3Client *client)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_val_if_fail (AWS_IS_S3_CLIENT(client), NULL);

  return priv->host;
}

void
aws_s3_client_set_host (AwsS3Client *client,
                        const gchar *host)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_if_fail(AWS_IS_S3_CLIENT(client));

  if (g_strcmp0 (priv->host, host) != 0)
    {
      g_free (priv->host);
      priv->host = g_strdup (host);
      g_object_notify_by_pspec (G_OBJECT (client), properties [PROP_HOST]);
    }
}

guint16
aws_s3_client_get_port (AwsS3Client *client)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_val_if_fail (AWS_IS_S3_CLIENT (client), 0);

  return priv->port;
}

void
aws_s3_client_set_port (AwsS3Client *client,
                        guint16      port)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_if_fail(AWS_IS_S3_CLIENT(client));

  if (priv->port != port)
    {
      priv->port = port;
      priv->port_set = (port != 0);
      g_object_notify_by_pspec (G_OBJECT (client), properties [PROP_PORT]);
      g_object_notify_by_pspec (G_OBJECT (client), properties [PROP_PORT_SET]);
    }
}

gboolean
aws_s3_client_get_port_set (AwsS3Client *client)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_val_if_fail (AWS_IS_S3_CLIENT (client), FALSE);

  return priv->port_set;
}

gboolean
aws_s3_client_get_secure (AwsS3Client *client)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_val_if_fail (AWS_IS_S3_CLIENT (client), FALSE);

  return priv->secure;
}

void
aws_s3_client_set_secure (AwsS3Client *client,
                          gboolean     secure)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  g_return_if_fail (AWS_IS_S3_CLIENT (client));

  secure = !!secure;

  if (priv->secure != secure)
    {
      priv->secure = secure;
      g_object_notify_by_pspec (G_OBJECT (client), properties [PROP_SECURE]);
    }
}

static void
aws_s3_client_read_cb (SoupSession *session,
                       SoupMessage *message,
                       gpointer     user_data)
{
  g_autoptr(GTask) task = user_data;

  g_assert (G_IS_SIMPLE_ASYNC_RESULT (simple));
  g_assert (SOUP_IS_MESSAGE (message));

  /* We might have completed in got_chunk() from a handler */
  if (g_task_get_completed (task))
    return;

  if (SOUP_STATUS_IS_SUCCESSFUL (message->status_code))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_new_error (task,
                             AWS_S3_CLIENT_ERROR,
                             AWS_S3_CLIENT_ERROR_UNKNOWN,
                             "Request failed: %d",
                             message->status_code);
}

static void
aws_s3_client_read_got_chunk (SoupMessage *message,
                              SoupBuffer  *buffer,
                              GTask       *task)
{
  AwsS3Client *client;
  ReadState *state;

  g_assert (SOUP_IS_MESSAGE (message));
  g_assert (buffer != NULL);
  g_assert (G_IS_TASK (task));

  client = g_task_get_source_object (task);
  g_assert (AWS_IS_S3_CLIENT (client));

  state = g_task_get_task_data (task);
  g_assert (state != NULL);
  g_assert (state->handler != NULL);

  if (!state->handler (client, message, buffer, state->handler_data))
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_CANCELLED,
                               "The request was cancelled");
      soup_session_cancel_message (SOUP_SESSION (client), message, SOUP_STATUS_CANCELLED);
   }
}

static void
aws_s3_client_read_got_headers (SoupMessage *message,
                                GTask       *task)
{
  AwsS3Client *client;

  g_assert (SOUP_IS_MESSAGE(message));
  g_assert (G_IS_TASK (task));

  client = g_task_get_source_object (task);
  g_assert (AWS_IS_S3_CLIENT (client));

  if (!SOUP_STATUS_IS_SUCCESSFUL (message->status_code))
    {
      /*
       * Extract the given error type.
       */
      if (message->status_code == SOUP_STATUS_NOT_FOUND)
        {
          g_task_return_new_error (task,
                                   AWS_S3_CLIENT_ERROR,
                                   AWS_S3_CLIENT_ERROR_NOT_FOUND,
                                   "The requested object was not found.");
         soup_session_cancel_message (SOUP_SESSION (client), message, message->status_code);
        }
      else if (SOUP_STATUS_IS_CLIENT_ERROR (message->status_code))
        {
          g_task_return_new_error (task,
                                   AWS_S3_CLIENT_ERROR,
                                   AWS_S3_CLIENT_ERROR_BAD_REQUEST,
                                   "The request was invalid.");
          soup_session_cancel_message (SOUP_SESSION (client), message, message->status_code);
        }
      else
        {
          g_task_return_new_error (task,
                                   AWS_S3_CLIENT_ERROR,
                                   AWS_S3_CLIENT_ERROR_UNKNOWN,
                                   "An unknown error occurred.");
          soup_session_cancel_message (SOUP_SESSION (client), message, SOUP_STATUS_CANCELLED);
        }
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
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);
  g_autoptr(GTask) task = NULL;
  g_autofree gchar *auth = NULL;
  g_autofree gchar *date_str = NULL;
  g_autofree gchar *signature = NULL;
  g_autofree gchar *uri = NULL;
  g_autoptr(SoupDate) date = NULL;
  g_autoptr(GString) str = NULL;
  g_autoptr(SoupMessage) message = NULL;
  ReadState *state;
  guint16 port;

  g_return_if_fail (AWS_IS_S3_CLIENT(client));
  g_return_if_fail (bucket);
  g_return_if_fail (path);
  g_return_if_fail (g_utf8_validate(bucket, -1, NULL));
  g_return_if_fail (g_utf8_validate(path, -1, NULL));
  g_return_if_fail (handler != NULL);

  task = g_task_new (client, cancellable, callback, user_data);
  g_task_set_source_tag (task, aws_s3_client_read_async);

  state = read_state_new (handler, handler_data, handler_notify);
  g_task_set_task_data (task, state, read_state_free);

  /*
   * Strip leading '/' from the path.
   */
  while (g_utf8_get_char (path) == '/')
    path = g_utf8_next_char (path);

  /*
   * Determine our connection port.
   */
  port = priv->port_set ? priv->port : (priv->secure ? 443 : 80);

  /*
   * Build our HTTP request message.
   */
  uri = g_strdup_printf ("%s://%s:%d/%s/%s",
                         priv->secure ? "https" : "http",
                         priv->host,
                         port,
                         bucket,
                         path);
  message = soup_message_new (SOUP_METHOD_GET, uri);
  soup_message_body_set_accumulate (message->response_body, FALSE);
  g_signal_connect_object (message,
                           "got-chunk",
                           G_CALLBACK (aws_s3_client_read_got_chunk),
                           task,
                           0);
  g_signal_connect_object (message,
                           "got-headers",
                           G_CALLBACK (aws_s3_client_read_got_headers),
                           task,
                           0);

  /*
   * Set the Host header for systems that may be proxying.
   */
  if (priv->host != NULL)
    soup_message_headers_append (message->request_headers, "Host", priv->host);

  /*
   * Add the Date header which we need for signing.
   */
  date = soup_date_new_from_now (0);
  date_str = soup_date_to_string (date, SOUP_DATE_HTTP);
  soup_message_headers_append (message->request_headers, "Date", date_str);

  /*
   * Sign our request.
   */
  str = g_string_new ("GET\n\n\n");
  g_string_append_printf (str, "%s\n", date_str);
  g_string_append_printf (str, "/%s/%s", bucket, path);
  signature = aws_credentials_sign (priv->creds, str->str, str->len, G_CHECKSUM_SHA1);

  /*
   * Attach request signature to our headers.
   */
  auth = g_strdup_printf ("AWS %s:%s",
                          aws_credentials_get_access_key (priv->creds),
                          signature);
  soup_message_headers_append (message->request_headers, "Authorization", auth);

  /*
   * Submit our request to the target.
   */
  soup_session_queue_message (SOUP_SESSION (client),
                              g_steal_pointer (&message),
                              aws_s3_client_read_cb,
                              g_steal_pointer (&task));
}

gboolean
aws_s3_client_read_finish (AwsS3Client   *client,
                           GAsyncResult  *result,
                           GError       **error)
{
  g_return_val_if_fail (AWS_IS_S3_CLIENT (client), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
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
  g_autoptr(GTask) task = NULL;

  g_return_if_fail (AWS_IS_S3_CLIENT (client));
  g_return_if_fail (bucket);
  g_return_if_fail (path);
  g_return_if_fail (G_IS_INPUT_STREAM (stream));

  task = g_task_new (client, cancellable, callback, user_data);
  g_task_set_source_tag (task, aws_s3_client_write_async);

  g_task_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Writing support has not been implemented");
}

gboolean
aws_s3_client_write_finish (AwsS3Client   *client,
                            GAsyncResult  *result,
                            GError       **error)
{
  g_return_val_if_fail (AWS_IS_S3_CLIENT (client), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
aws_s3_client_finalize (GObject *object)
{
  AwsS3Client *self = (AwsS3Client *)object;
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (self);

  g_clear_pointer (&priv->host, g_free);
  g_clear_object (&priv->creds);

  G_OBJECT_CLASS (aws_s3_client_parent_class)->finalize (object);
}

static void
aws_s3_client_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  AwsS3Client *self = AWS_S3_CLIENT (object);

  switch (prop_id)
    {
    case PROP_CREDENTIALS:
      g_value_set_object (value, aws_s3_client_get_credentials (self));
      break;

    case PROP_HOST:
      g_value_set_string (value, aws_s3_client_get_host (self));
      break;

    case PROP_PORT:
      g_value_set_uint (value, aws_s3_client_get_port (self));
      break;

    case PROP_PORT_SET:
      g_value_set_boolean (value, aws_s3_client_get_port_set (self));
      break;

    case PROP_SECURE:
      g_value_set_boolean (value, aws_s3_client_get_secure (self));
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
  AwsS3Client *self = AWS_S3_CLIENT (object);

  switch (prop_id)
    {
    case PROP_CREDENTIALS:
      aws_s3_client_set_credentials (self, g_value_get_object (value));
      break;

    case PROP_HOST:
      aws_s3_client_set_host (self, g_value_get_string (value));
      break;

    case PROP_PORT:
      aws_s3_client_set_port (self, g_value_get_uint (value));
      break;

    case PROP_SECURE:
      aws_s3_client_set_secure (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
aws_s3_client_class_init (AwsS3ClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = aws_s3_client_finalize;
  object_class->get_property = aws_s3_client_get_property;
  object_class->set_property = aws_s3_client_set_property;

  properties [PROP_CREDENTIALS] =
    g_param_spec_object ("credentials",
                         "Credentials",
                         "The credentials for your AWS account.",
                         AWS_TYPE_CREDENTIALS,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties [PROP_HOST] =
    g_param_spec_string ("host",
                         "Host",
                         "The hostname of the AWS server.",
                         "s3.amazonaws.com",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties [PROP_PORT] =
    g_param_spec_uint ("port",
                       "Port",
                       "The port of the AWS server.",
                       1,
                       G_MAXUSHORT,
                       80,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties [PROP_PORT_SET] =
    g_param_spec_boolean("port-set",
                         "Port Set",
                         "If the port number has been set.",
                         FALSE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties [PROP_SECURE] =
    g_param_spec_boolean("secure",
                         _("Secure"),
                         _("If HTTPS is required."),
                         TRUE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
aws_s3_client_init (AwsS3Client *client)
{
  AwsS3ClientPrivate *priv = aws_s3_client_get_instance_private (client);

  priv->host = g_strdup_printf ("s3.amazonaws.com");
  priv->creds = aws_credentials_new ("", "");
  priv->secure = TRUE;
}

GQuark
aws_s3_client_error_quark (void)
{
  return g_quark_from_static_string ("aws-s3-client-error-quark");
}
