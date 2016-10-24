/* aws-s3-client.h
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

#ifndef AWS_S3_CLIENT_H
#define AWS_S3_CLIENT_H

#include <gio/gio.h>
#include <libsoup/soup.h>

#include "aws-credentials.h"

G_BEGIN_DECLS

#define AWS_TYPE_S3_CLIENT  (aws_s3_client_get_type())
#define AWS_S3_CLIENT_ERROR (aws_s3_client_error_quark())

G_DECLARE_DERIVABLE_TYPE (AwsS3Client, aws_s3_client, AWS, S3_CLIENT, SoupSession)

struct _AwsS3ClientClass
{
  SoupSessionClass parent_instance;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
  gpointer _reserved9;
  gpointer _reserved10;
  gpointer _reserved11;
  gpointer _reserved12;
  gpointer _reserved13;
  gpointer _reserved14;
  gpointer _reserved15;
  gpointer _reserved16;
};

typedef gboolean (*AwsS3ClientDataHandler) (AwsS3Client *client,
                                            SoupMessage *message,
                                            SoupBuffer  *buffer,
                                            gpointer     user_data);

typedef enum
{
   AWS_S3_CLIENT_ERROR_BAD_REQUEST = 1,
   AWS_S3_CLIENT_ERROR_CANCELLED   = 2,
   AWS_S3_CLIENT_ERROR_UNKNOWN     = 3,
   AWS_S3_CLIENT_ERROR_NOT_FOUND   = 404,
} AwsS3ClientError;

GQuark          aws_s3_client_error_quark     (void);
AwsCredentials *aws_s3_client_get_credentials (AwsS3Client             *self);
void            aws_s3_client_set_credentials (AwsS3Client             *self,
                                               AwsCredentials          *credentials);
const gchar    *aws_s3_client_get_host        (AwsS3Client             *self);
guint16         aws_s3_client_get_port        (AwsS3Client             *self);
gboolean        aws_s3_client_get_port_set    (AwsS3Client             *self);
gboolean        aws_s3_client_get_secure      (AwsS3Client             *self);
void            aws_s3_client_read_async      (AwsS3Client             *self,
                                               const gchar             *bucket,
                                               const gchar             *path,
                                               AwsS3ClientDataHandler   handler,
                                               gpointer                 handler_data,
                                               GDestroyNotify           handler_notify,
                                               GCancellable            *cancellable,
                                               GAsyncReadyCallback      callback,
                                               gpointer                 user_data);
gboolean        aws_s3_client_read_finish     (AwsS3Client             *self,
                                               GAsyncResult            *result,
                                               GError                 **error);
void            aws_s3_client_set_host        (AwsS3Client             *self,
                                               const gchar             *host);
void            aws_s3_client_set_port        (AwsS3Client             *self,
                                               guint16                  port);
void            aws_s3_client_set_secure      (AwsS3Client             *self,
                                               gboolean                 secure);
void            aws_s3_client_write_async     (AwsS3Client             *self,
                                               const gchar             *bucket,
                                               const gchar             *path,
                                               GInputStream            *stream,
                                               GCancellable            *cancellable,
                                               GAsyncReadyCallback      callback,
                                               gpointer                 user_data);
gboolean        aws_s3_client_write_finish    (AwsS3Client             *self,
                                               GAsyncResult            *result,
                                               GError                 **error);


G_END_DECLS

#endif /* AWS_S3_CLIENT_H */
