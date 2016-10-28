/* aws-credentials.c
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

#include <string.h>

#include "aws-credentials.h"

struct _AwsCredentials
{
  GObject parent_instance;

  gchar *access_key;
  gchar *secret_key;
};

G_DEFINE_TYPE (AwsCredentials, aws_credentials, G_TYPE_OBJECT)

enum {
   PROP_0,
   PROP_ACCESS_KEY,
   PROP_SECRET_KEY,
   N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
str_zero_and_free (gchar *str)
{
  if (str != NULL)
    {
      /* We need something more likely to not be
       * optimized out here...
       */
      memset (str, 0, strlen (str));
      g_free (str);
    }
}

AwsCredentials *
aws_credentials_new (const gchar *access_key,
                     const gchar *secret_key)
{
  return g_object_new (AWS_TYPE_CREDENTIALS,
                       "access-key", access_key,
                       "secret-key", secret_key,
                       NULL);
}

const gchar *
aws_credentials_get_access_key (AwsCredentials *self)
{
  g_return_val_if_fail (AWS_IS_CREDENTIALS (self), NULL);

  return self->access_key;
}

const gchar *
aws_credentials_get_secret_key (AwsCredentials *self)
{
  g_return_val_if_fail (AWS_IS_CREDENTIALS (self), NULL);

  return self->secret_key;
}

void
aws_credentials_set_access_key (AwsCredentials *self,
                                const gchar    *access_key)
{
  g_return_if_fail (AWS_IS_CREDENTIALS (self));

  if (g_strcmp0 (self->access_key, access_key) != 0)
    {
      str_zero_and_free (self->access_key);
      self->access_key = g_strdup (self->access_key);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACCESS_KEY]);
    }
}

void
aws_credentials_set_secret_key (AwsCredentials *self,
                                const gchar    *secret_key)
{
  g_return_if_fail (AWS_IS_CREDENTIALS (self));

  if (g_strcmp0 (self->secret_key, secret_key) != 0)
    {
      str_zero_and_free (self->secret_key);
      self->secret_key = g_strdup (self->secret_key);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SECRET_KEY]);
    }
}

/**
 * aws_credentials_sign:
 * @self: A #AwsCredentials.
 * @text: The text to sign.
 * @text_len: The length of @text or -1 if it is NUL-terminated.
 *
 * Generate a signed checksum for the given text.
 *
 * Returns: (transfer full): The signature.
 */
gchar *
aws_credentials_sign (AwsCredentials *self,
                      const gchar    *text,
                      gssize          text_len,
                      GChecksumType   digest_type)
{
  g_autofree guint8 *digest = NULL;
  g_autofree gchar *signature = NULL;
  g_autoptr(GHmac) hmac = NULL;
  gsize digest_len = 0;

  g_return_val_if_fail (AWS_IS_CREDENTIALS (self), NULL);
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (self->secret_key != NULL, NULL);
  g_return_val_if_fail (digest_type == G_CHECKSUM_SHA1 ||
                        digest_type == G_CHECKSUM_SHA256, NULL);

  if (text_len < 0)
    text_len = strlen(text);

  if ((digest_len = g_checksum_type_get_length (digest_type)) < 1)
    {
      g_warning ("Invalid digest type requested!");
      return NULL;
    }

  digest = g_malloc (digest_len);
  hmac = g_hmac_new (digest_type,
                     (const guint8 *)self->secret_key,
                     strlen (self->secret_key));
  g_hmac_update (hmac, (const guint8 *)text, text_len);
  g_hmac_get_digest (hmac, digest, &digest_len);
  signature = g_base64_encode (digest, digest_len);

  return g_steal_pointer (&signature);
}

static void
aws_credentials_finalize (GObject *object)
{
  AwsCredentials *self = (AwsCredentials *)object;

  g_clear_pointer (&self->access_key, str_zero_and_free);
  g_clear_pointer (&self->secret_key, str_zero_and_free);

  G_OBJECT_CLASS (aws_credentials_parent_class)->finalize (object);
}

static void
aws_credentials_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  AwsCredentials *credentials = AWS_CREDENTIALS (object);

  switch (prop_id)
    {
    case PROP_ACCESS_KEY:
      g_value_set_string (value, aws_credentials_get_access_key (credentials));
      break;

    case PROP_SECRET_KEY:
      g_value_set_string (value, aws_credentials_get_secret_key (credentials));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
aws_credentials_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  AwsCredentials *credentials = AWS_CREDENTIALS (object);

  switch (prop_id)
    {
    case PROP_ACCESS_KEY:
      aws_credentials_set_access_key (credentials, g_value_get_string (value));
      break;

    case PROP_SECRET_KEY:
      aws_credentials_set_secret_key (credentials, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
aws_credentials_class_init (AwsCredentialsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = aws_credentials_finalize;
  object_class->get_property = aws_credentials_get_property;
  object_class->set_property = aws_credentials_set_property;

  properties [PROP_ACCESS_KEY] =
    g_param_spec_string ("access-key",
                         "Access Key",
                         "Amazon AWS Access Key.",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties [PROP_SECRET_KEY] =
    g_param_spec_string ("secret-key",
                         "Secret Key",
                         "Amazon AWS Secret Key.",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
aws_credentials_init (AwsCredentials *credentials)
{
  aws_credentials_set_access_key (credentials, "");
  aws_credentials_set_secret_key (credentials, "");
}
