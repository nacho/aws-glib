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
#include <libsoup/soup-session-async.h>

#include "aws-credentials.h"

G_BEGIN_DECLS

#define AWS_TYPE_S3_CLIENT            (aws_s3_client_get_type())
#define AWS_S3_CLIENT_ERROR           (aws_s3_client_error_quark())
#define AWS_S3_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWS_TYPE_S3_CLIENT, AwsS3Client))
#define AWS_S3_CLIENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWS_TYPE_S3_CLIENT, AwsS3Client const))
#define AWS_S3_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  AWS_TYPE_S3_CLIENT, AwsS3ClientClass))
#define AWS_IS_S3_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWS_TYPE_S3_CLIENT))
#define AWS_IS_S3_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  AWS_TYPE_S3_CLIENT))
#define AWS_S3_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  AWS_TYPE_S3_CLIENT, AwsS3ClientClass))

typedef struct _AwsS3Client        AwsS3Client;
typedef struct _AwsS3ClientClass   AwsS3ClientClass;
typedef enum   _AwsS3ClientError   AwsS3ClientError;
typedef struct _AwsS3ClientPrivate AwsS3ClientPrivate;

typedef gboolean (*AwsS3ClientDataHandler) (AwsS3Client *client,
                                            SoupMessage *message,
                                            SoupBuffer  *buffer,
                                            gpointer     user_data);

enum _AwsS3ClientError
{
   AWS_S3_CLIENT_ERROR_BAD_REQUEST = 1,
   AWS_S3_CLIENT_ERROR_CANCELLED   = 2,
   AWS_S3_CLIENT_ERROR_UNKNOWN     = 3,
   AWS_S3_CLIENT_ERROR_NOT_FOUND   = 404,
};

struct _AwsS3Client
{
   SoupSessionAsync parent;

   /*< private >*/
   AwsS3ClientPrivate *priv;
};

struct _AwsS3ClientClass
{
   SoupSessionAsyncClass parent_class;
};

GQuark          aws_s3_client_error_quark     (void) G_GNUC_CONST;
AwsCredentials *aws_s3_client_get_credentials (AwsS3Client             *client);
void            aws_s3_client_set_credentials (AwsS3Client             *client,
                                               AwsCredentials          *credentials);
const gchar    *aws_s3_client_get_host        (AwsS3Client             *client);
guint16         aws_s3_client_get_port        (AwsS3Client             *client);
gboolean        aws_s3_client_get_port_set    (AwsS3Client             *client);
gboolean        aws_s3_client_get_secure      (AwsS3Client             *client);
GType           aws_s3_client_get_type        (void) G_GNUC_CONST;
void            aws_s3_client_read_async      (AwsS3Client             *client,
                                               const gchar             *bucket,
                                               const gchar             *path,
                                               AwsS3ClientDataHandler   handler,
                                               gpointer                 handler_data,
                                               GDestroyNotify           handler_notify,
                                               GCancellable            *cancellable,
                                               GAsyncReadyCallback      callback,
                                               gpointer                 user_data);
gboolean        aws_s3_client_read_finish     (AwsS3Client             *client,
                                               GAsyncResult            *result,
                                               GError                 **error);
void            aws_s3_client_set_host        (AwsS3Client             *client,
                                               const gchar             *host);
void            aws_s3_client_set_port        (AwsS3Client             *client,
                                               guint16                  port);
void            aws_s3_client_set_secure      (AwsS3Client             *client,
                                               gboolean                 secure);
void            aws_s3_client_write_async     (AwsS3Client             *client,
                                               const gchar             *bucket,
                                               const gchar             *path,
                                               GInputStream            *stream,
                                               GCancellable            *cancellable,
                                               GAsyncReadyCallback      callback,
                                               gpointer                 user_data);
gboolean        aws_s3_client_write_finish    (AwsS3Client             *client,
                                               GAsyncResult            *result,
                                               GError                 **error);


G_END_DECLS

#endif /* AWS_S3_CLIENT_H */
