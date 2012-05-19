/* aws-credentials.c
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
#include <string.h>

#include "aws-credentials.h"

G_DEFINE_TYPE(AwsCredentials, aws_credentials, G_TYPE_OBJECT)

struct _AwsCredentialsPrivate
{
   gchar *access_key;
   gchar *secret_key;
};

enum
{
   PROP_0,
   PROP_ACCESS_KEY,
   PROP_SECRET_KEY,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

AwsCredentials *
aws_credentials_new (const gchar *access_key,
                     const gchar *secret_key)
{
   return g_object_new(AWS_TYPE_CREDENTIALS,
                       "access-key", access_key,
                       "secret-key", secret_key,
                       NULL);
}

const gchar *
aws_credentials_get_access_key (AwsCredentials *credentials)
{
   g_return_val_if_fail(AWS_IS_CREDENTIALS(credentials), NULL);
   return credentials->priv->access_key;
}

const gchar *
aws_credentials_get_secret_key (AwsCredentials *credentials)
{
   g_return_val_if_fail(AWS_IS_CREDENTIALS(credentials), NULL);
   return credentials->priv->secret_key;
}

void
aws_credentials_set_access_key (AwsCredentials *credentials,
                                const gchar    *access_key)
{
   g_return_if_fail(AWS_IS_CREDENTIALS(credentials));

   access_key = access_key ? access_key : "";
   g_free(credentials->priv->access_key);
   credentials->priv->access_key = g_strdup(access_key);
   g_object_notify_by_pspec(G_OBJECT(credentials),
                            gParamSpecs[PROP_ACCESS_KEY]);
}

void
aws_credentials_set_secret_key (AwsCredentials *credentials,
                                const gchar    *secret_key)
{
   g_return_if_fail(AWS_IS_CREDENTIALS(credentials));

   secret_key = secret_key ? secret_key : "";
   g_free(credentials->priv->secret_key);
   credentials->priv->secret_key = g_strdup(secret_key);
   g_object_notify_by_pspec(G_OBJECT(credentials),
                            gParamSpecs[PROP_SECRET_KEY]);
}

/**
 * aws_credentials_sign:
 * @credentials: (in): A #AwsCredentials.
 * @text: (in): The text to sign.
 * @text_len: (in): The length of @text or -1 if it is NUL-terminated.
 *
 * Generate a signed checksum for the given text.
 *
 * Returns: (transfer full): The signature.
 */
gchar *
aws_credentials_sign (AwsCredentials *credentials,
                      const gchar    *text,
                      gssize          text_len,
                      GChecksumType   digest_type)
{
   AwsCredentialsPrivate *priv;
   guint8 *digest = NULL;
   gchar *signature;
   GHmac *hmac;
   gsize digest_len = 0;

   g_return_val_if_fail(AWS_IS_CREDENTIALS(credentials), NULL);
   g_return_val_if_fail(text, NULL);
   g_return_val_if_fail(digest_type == G_CHECKSUM_SHA1 ||
                        digest_type == G_CHECKSUM_SHA256, NULL);

   priv = credentials->priv;

   if (text_len < 0) {
      text_len = strlen(text);
   }

   if ((digest_len = g_checksum_type_get_length(digest_type)) < 1) {
      g_warning("Invalid digest type requested!");
      return NULL;
   }

   digest = g_malloc(digest_len);
   hmac = g_hmac_new(digest_type, (const guint8 *)priv->secret_key,
                     strlen(priv->secret_key));
   g_hmac_update(hmac, (guint8 *)text, text_len);
   g_hmac_get_digest(hmac, digest, &digest_len);
   signature = g_base64_encode(digest, digest_len);

   g_free(digest);
   g_hmac_unref(hmac);

   return signature;
}

static void
aws_credentials_finalize (GObject *object)
{
   AwsCredentialsPrivate *priv;

   priv = AWS_CREDENTIALS(object)->priv;

   if (priv->access_key) {
      memset(priv->access_key, 0, strlen(priv->access_key));
      g_free(priv->access_key);
   }

   if (priv->secret_key) {
      memset(priv->secret_key, 0, strlen(priv->secret_key));
      g_free(priv->secret_key);
   }

   G_OBJECT_CLASS(aws_credentials_parent_class)->finalize(object);
}

static void
aws_credentials_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
   AwsCredentials *credentials = AWS_CREDENTIALS(object);

   switch (prop_id) {
   case PROP_ACCESS_KEY:
      g_value_set_string(value, aws_credentials_get_access_key(credentials));
      break;
   case PROP_SECRET_KEY:
      g_value_set_string(value, aws_credentials_get_secret_key(credentials));
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
   AwsCredentials *credentials = AWS_CREDENTIALS(object);

   switch (prop_id) {
   case PROP_ACCESS_KEY:
      aws_credentials_set_access_key(credentials, g_value_get_string(value));
      break;
   case PROP_SECRET_KEY:
      aws_credentials_set_secret_key(credentials, g_value_get_string(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
aws_credentials_class_init (AwsCredentialsClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = aws_credentials_finalize;
   object_class->get_property = aws_credentials_get_property;
   object_class->set_property = aws_credentials_set_property;
   g_type_class_add_private(object_class, sizeof(AwsCredentialsPrivate));

   gParamSpecs[PROP_ACCESS_KEY] =
      g_param_spec_string("access-key",
                          _("Access Key"),
                          _("Amazon AWS Access Key."),
                          "",
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_ACCESS_KEY,
                                   gParamSpecs[PROP_ACCESS_KEY]);

   gParamSpecs[PROP_SECRET_KEY] =
      g_param_spec_string("secret-key",
                          _("Secret Key"),
                          _("Amazon AWS Secret Key."),
                          "",
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_SECRET_KEY,
                                   gParamSpecs[PROP_SECRET_KEY]);
}

static void
aws_credentials_init (AwsCredentials *credentials)
{
   credentials->priv = G_TYPE_INSTANCE_GET_PRIVATE(credentials,
                                                   AWS_TYPE_CREDENTIALS,
                                                   AwsCredentialsPrivate);
   aws_credentials_set_access_key(credentials, "");
   aws_credentials_set_secret_key(credentials, "");
}
