/* aws-credentials.h
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

#ifndef AWS_CREDENTIALS_H
#define AWS_CREDENTIALS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define AWS_TYPE_CREDENTIALS            (aws_credentials_get_type())
#define AWS_CREDENTIALS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWS_TYPE_CREDENTIALS, AwsCredentials))
#define AWS_CREDENTIALS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWS_TYPE_CREDENTIALS, AwsCredentials const))
#define AWS_CREDENTIALS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  AWS_TYPE_CREDENTIALS, AwsCredentialsClass))
#define AWS_IS_CREDENTIALS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWS_TYPE_CREDENTIALS))
#define AWS_IS_CREDENTIALS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  AWS_TYPE_CREDENTIALS))
#define AWS_CREDENTIALS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  AWS_TYPE_CREDENTIALS, AwsCredentialsClass))

typedef struct _AwsCredentials        AwsCredentials;
typedef struct _AwsCredentialsClass   AwsCredentialsClass;
typedef struct _AwsCredentialsPrivate AwsCredentialsPrivate;

struct _AwsCredentials
{
   GObject parent;

   /*< private >*/
   AwsCredentialsPrivate *priv;
};

struct _AwsCredentialsClass
{
   GObjectClass parent_class;
};

AwsCredentials *aws_credentials_new            (const gchar    *access_key,
                                                const gchar    *secret_key);
const gchar    *aws_credentials_get_access_key (AwsCredentials *credentials);
const gchar    *aws_credentials_get_secret_key (AwsCredentials *credentials);
GType           aws_credentials_get_type       (void) G_GNUC_CONST;
void            aws_credentials_set_access_key (AwsCredentials *credentials,
                                                const gchar    *access_key);
void            aws_credentials_set_secret_key (AwsCredentials *credentials,
                                                const gchar    *secret_key);
gchar          *aws_credentials_sign           (AwsCredentials *credentials,
                                                const gchar    *text,
                                                gssize          text_len,
                                                GChecksumType   digest_type);

G_END_DECLS

#endif /* AWS_CREDENTIALS_H */
