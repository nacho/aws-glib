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

#define AWS_TYPE_CREDENTIALS (aws_credentials_get_type())

G_DECLARE_FINAL_TYPE (AwsCredentials, aws_credentials, AWS, CREDENTIALS, GObject)

AwsCredentials *aws_credentials_new            (const gchar    *access_key,
                                                const gchar    *secret_key);
const gchar    *aws_credentials_get_access_key (AwsCredentials *self);
void            aws_credentials_set_access_key (AwsCredentials *self,
                                                const gchar    *access_key);
const gchar    *aws_credentials_get_secret_key (AwsCredentials *self);
void            aws_credentials_set_secret_key (AwsCredentials *self,
                                                const gchar    *secret_key);
gchar          *aws_credentials_sign           (AwsCredentials *self,
                                                const gchar    *text,
                                                gssize          text_len,
                                                GChecksumType   digest_type);

G_END_DECLS

#endif /* AWS_CREDENTIALS_H */
