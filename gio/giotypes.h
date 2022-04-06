/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __GIO_TYPES_H__
#define __GIO_TYPES_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gioenums.h>

G_BEGIN_DECLS

typedef struct _GAppLaunchContext             xapp_launch_context_t;
typedef struct _GAppInfo                      xapp_info_t; /* Dummy typedef */
typedef struct _GAsyncResult                  xasync_result_t; /* Dummy typedef */
typedef struct _GAsyncInitable                xasync_initable_t;
typedef struct _GBufferedInputStream          xbuffered_input_stream_t;
typedef struct _GBufferedOutputStream         xbuffered_output_stream_t;
typedef struct _GCancellable                  xcancellable_t;
typedef struct _GCharsetConverter             xcharset_converter_t;
typedef struct _GConverter                    xconverter_t;
typedef struct _GConverterInputStream         xconverter_input_stream_t;
typedef struct _GConverterOutputStream        xconverter_output_stream_t;
typedef struct _GDatagramBased                xdatagram_based_t;
typedef struct _GDataInputStream              xdata_input_stream_t;
typedef struct _GSimplePermission             xsimple_permission_t;
typedef struct _GZlibCompressor               xzlib_compressor_t;
typedef struct _GZlibDecompressor             xzlib_decompressor_t;

typedef struct _GSimpleActionGroup            xsimple_action_group_t;
typedef struct _GRemoteActionGroup            xremote_action_group_t;
typedef struct _GDBusActionGroup              xdbus_action_group_t;
typedef struct _GActionMap                    xaction_map_t;
typedef struct _GActionGroup                  xaction_group_t;
typedef struct _GPropertyAction               xproperty_action_t;
typedef struct _GSimpleAction                 xsimple_action_t;
typedef struct _GAction                       xaction_t;
typedef struct _xapplication                  xapplication_t;
typedef struct _GApplicationCommandLine       xapplication_command_line_t;
typedef struct _GSettingsBackend              xsettings_backend_t;
typedef struct _GSettings                     xsettings_t;
typedef struct _GPermission                   xpermission_t;

typedef struct _GMenuModel                    xmenu_model_t;
typedef struct _GNotification                 xnotification_t;

/**
 * xdrive_t:
 *
 * Opaque drive object.
 **/
typedef struct _GDrive                        xdrive_t; /* Dummy typedef */
typedef struct _GFileEnumerator               xfile_enumerator_t;
typedef struct _GFileMonitor                  xfile_monitor_t;
typedef struct _GFilterInputStream            xfilter_input_stream_t;
typedef struct _GFilterOutputStream           xfilter_output_stream_t;

/**
 * xfile_t:
 *
 * A handle to an object implementing the #xfile_iface_t interface.
 * Generally stores a location within the file system. Handles do not
 * necessarily represent files or directories that currently exist.
 **/
typedef struct _GFile                         xfile_t; /* Dummy typedef */
typedef struct _xfile_info                     xfile_info_t;

/**
 * xfile_attribute_matcher_t:
 *
 * Determines if a string matches a file attribute.
 **/
typedef struct _GFileAttributeMatcher         xfile_attribute_matcher_t;
typedef struct _GFileAttributeInfo            xfile_attribute_info_t;
typedef struct _GFileAttributeInfoList        xfile_attribute_info_list_t;
typedef struct _GFileDescriptorBased          xfile_descriptor_based_t;
typedef struct _GFileInputStream              xfile_input_stream_t;
typedef struct _GFileOutputStream             xfile_output_stream_t;
typedef struct _GFileIOStream                 xfile_io_stream_t;
typedef struct _GFileIcon                     xfile_icon_t;
typedef struct _xfilename_completer            xfilename_completer_t;


typedef struct _GIcon                         xicon_t; /* Dummy typedef */
typedef struct _GInetAddress                  xinet_address_t;
typedef struct _GInetAddressMask              xinet_address_mask_t;
typedef struct _GInetSocketAddress            xinet_socket_address_t;
typedef struct _GNativeSocketAddress          xnative_socket_address_t;
typedef struct _GInputStream                  xinput_stream_t;
typedef struct _GInitable                     xinitable_t;
typedef struct _xio_module                     xio_module_t;
typedef struct _GIOExtensionPoint             xio_extension_point_t;
typedef struct _GIOExtension                  xio_extension_t;

/**
 * xio_scheduler_job_t:
 *
 * Opaque class for defining and scheduling IO jobs.
 **/
typedef struct _GIOSchedulerJob               xio_scheduler_job_t;
typedef struct _GIOStreamAdapter              xio_stream_adapter_t;
typedef struct _GLoadableIcon                 xloadable_icon_t; /* Dummy typedef */
typedef struct _xbytes_icon                    xbytes_icon_t;
typedef struct _GMemoryInputStream            xmemory_input_stream_t;
typedef struct _GMemoryOutputStream           xmemory_output_stream_t;

/**
 * xmount_t:
 *
 * A handle to an object implementing the #GMountIface interface.
 **/
typedef struct _GMount                        xmount_t; /* Dummy typedef */
typedef struct _GMountOperation               xmount_operation_t;
typedef struct _GNetworkAddress               xnetwork_address_t;
typedef struct _GNetworkMonitor               xnetwork_monitor_t;
typedef struct _GNetworkService               xnetwork_service_t;
typedef struct _xoutput_stream                 xoutput_stream_t;
typedef struct _xio_stream                    xio_stream_t;
typedef struct _GSimpleIOStream               xsimple_io_stream_t;
typedef struct _GPollableInputStream          xpollable_input_stream_t; /* Dummy typedef */
typedef struct _GPollableOutputStream         xpollable_output_stream_t; /* Dummy typedef */
typedef struct _GResolver                     xresolver_t;

/**
 * xresource_t:
 *
 * A resource bundle.
 *
 * Since: 2.32
 */
typedef struct _GResource                     xresource_t;
typedef struct _GSeekable                     xseekable__t;
typedef struct _GSimpleAsyncResult            xsimple_async_result_t;

/**
 * xsocket_t:
 *
 * A lowlevel network socket object.
 *
 * Since: 2.22
 **/
typedef struct _GSocket                       xsocket_t;

/**
 * xsocket_control_message_t:
 *
 * Base class for socket-type specific control messages that can be sent and
 * received over #xsocket_t.
 **/
typedef struct _GSocketControlMessage         xsocket_control_message_t;
/**
 * xsocket_client_t:
 *
 * A helper class for network clients to make connections.
 *
 * Since: 2.22
 **/
typedef struct _GSocketClient                               xsocket_client_t;
/**
 * xsocket_connection_t:
 *
 * A socket connection xio_stream_t object for connection-oriented sockets.
 *
 * Since: 2.22
 **/
typedef struct _xsocket_connection                           xsocket_connection_t;
/**
 * xsocket_listener_t:
 *
 * A helper class for network servers to listen for and accept connections.
 *
 * Since: 2.22
 **/
typedef struct _GSocketListener                             xsocket_listener_t;
/**
 * xsocket_service_t:
 *
 * A helper class for handling accepting incoming connections in the
 * glib mainloop.
 *
 * Since: 2.22
 **/
typedef struct _GSocketService                              xsocket_service_t;
typedef struct _GSocketAddress                xsocket_address_t;
typedef struct _GSocketAddressEnumerator      xsocket_address_enumerator_t;
typedef struct _GSocketConnectable            xsocket_connectable_t;
typedef struct _GSrvTarget                    xsrv_target_t;
typedef struct _GTask                         xtask_t;
/**
 * xtcp_connection_t:
 *
 * A #xsocket_connection_t for TCP/IP connections.
 *
 * Since: 2.22
 **/
typedef struct _GTcpConnection                              xtcp_connection_t;
typedef struct _GTcpWrapperConnection                       xtcp_wrapper_connection_t;
/**
 * xthreaded_socket_service_t:
 *
 * A helper class for handling accepting incoming connections in the
 * glib mainloop and handling them in a thread.
 *
 * Since: 2.22
 **/
typedef struct _GThreadedSocketService                      xthreaded_socket_service_t;
typedef struct _GDtlsConnection               xdtls_connection_t;
typedef struct _GDtlsClientConnection         xdtls_client_connection_t; /* Dummy typedef */
typedef struct _GDtlsServerConnection         xdtls_server_connection_t; /* Dummy typedef */
typedef struct _GThemedIcon                   xthemed_icon_t;
typedef struct _GTlsCertificate               xtls_certificate_t;
typedef struct _GTlsClientConnection          xtls_client_connection_t; /* Dummy typedef */
typedef struct _GTlsConnection                xtls_connection_t;
typedef struct _GTlsDatabase                  xtls_database_t;
typedef struct _GTlsFileDatabase              xtls_file_database_t;
typedef struct _GTlsInteraction               xtls_interaction_t;
typedef struct _GTlsPassword                  xtls_password_t;
typedef struct _GTlsServerConnection          xtls_server_connection_t; /* Dummy typedef */
typedef struct _GVfs                          xvfs_t; /* Dummy typedef */

/**
 * xproxy_resolver_t:
 *
 * A helper class to enumerate proxies base on URI.
 *
 * Since: 2.26
 **/
typedef struct _GProxyResolver                xproxy_resolver_t;
typedef struct _GProxy			      xproxy_t;
typedef struct _GProxyAddress		      xproxy_address_t;
typedef struct _GProxyAddressEnumerator	      xproxy_address_enumerator_t;

/**
 * xvolume_t:
 *
 * Opaque mountable volume object.
 **/
typedef struct _xvolume                       xvolume_t; /* Dummy typedef */
typedef struct _xvolume_monitor                xvolume_monitor_t;

/**
 * xasync_ready_callback_t:
 * @source_object: (nullable): the object the asynchronous operation was started with.
 * @res: a #xasync_result_t.
 * @user_data: user data passed to the callback.
 *
 * Type definition for a function that will be called back when an asynchronous
 * operation within GIO has been completed. #xasync_ready_callback_t
 * callbacks from #xtask_t are guaranteed to be invoked in a later
 * iteration of the
 * [thread-default main context][g-main-context-push-thread-default]
 * where the #xtask_t was created. All other users of
 * #xasync_ready_callback_t must likewise call it asynchronously in a
 * later iteration of the main context.
 *
 * The asynchronous operation is guaranteed to have held a reference to
 * @source_object from the time when the `*_async()` function was called, until
 * after this callback returns.
 **/
typedef void (*xasync_ready_callback_t) (xobject_t *source_object,
				     xasync_result_t *res,
				     xpointer_t user_data);

/**
 * xfile_progress_callback_t:
 * @current_num_bytes: the current number of bytes in the operation.
 * @total_num_bytes: the total number of bytes in the operation.
 * @user_data: user data passed to the callback.
 *
 * When doing file operations that may take a while, such as moving
 * a file or copying a file, a progress callback is used to pass how
 * far along that operation is to the application.
 **/
typedef void (*xfile_progress_callback_t) (xoffset_t current_num_bytes,
                                       xoffset_t total_num_bytes,
                                       xpointer_t user_data);

/**
 * xfile_read_more_callback_t:
 * @file_contents: the data as currently read.
 * @file_size: the size of the data currently read.
 * @callback_data: (closure): data passed to the callback.
 *
 * When loading the partial contents of a file with xfile_load_partial_contents_async(),
 * it may become necessary to determine if any more data from the file should be loaded.
 * A #xfile_read_more_callback_t function facilitates this by returning %TRUE if more data
 * should be read, or %FALSE otherwise.
 *
 * Returns: %TRUE if more data should be read back. %FALSE otherwise.
 **/
typedef xboolean_t (* xfile_read_more_callback_t) (const char *file_contents,
                                            xoffset_t file_size,
                                            xpointer_t callback_data);

/**
 * xfile_measure_progress_callback_t:
 * @reporting: %TRUE if more reports will come
 * @current_size: the current cumulative size measurement
 * @num_dirs: the number of directories visited so far
 * @num_files: the number of non-directory files encountered
 * @user_data: the data passed to the original request for this callback
 *
 * This callback type is used by xfile_measure_disk_usage() to make
 * periodic progress reports when measuring the amount of disk spaced
 * used by a directory.
 *
 * These calls are made on a best-effort basis and not all types of
 * #xfile_t will support them.  At the minimum, however, one call will
 * always be made immediately.
 *
 * In the case that there is no support, @reporting will be set to
 * %FALSE (and the other values undefined) and no further calls will be
 * made.  Otherwise, the @reporting will be %TRUE and the other values
 * all-zeros during the first (immediate) call.  In this way, you can
 * know which type of progress UI to show without a delay.
 *
 * For xfile_measure_disk_usage() the callback is made directly.  For
 * xfile_measure_disk_usage_async() the callback is made via the
 * default main context of the calling thread (ie: the same way that the
 * final async result would be reported).
 *
 * @current_size is in the same units as requested by the operation (see
 * %XFILE_MEASURE_APPARENT_SIZE).
 *
 * The frequency of the updates is implementation defined, but is
 * ideally about once every 200ms.
 *
 * The last progress callback may or may not be equal to the final
 * result.  Always check the async result to get the final value.
 *
 * Since: 2.38
 **/
typedef void (* xfile_measure_progress_callback_t) (xboolean_t reporting,
                                               xuint64_t  current_size,
                                               xuint64_t  num_dirs,
                                               xuint64_t  num_files,
                                               xpointer_t user_data);

/**
 * GIOSchedulerJobFunc:
 * @job: a #xio_scheduler_job_t.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 * @user_data: the data to pass to callback function
 *
 * I/O Job function.
 *
 * Long-running jobs should periodically check the @cancellable
 * to see if they have been cancelled.
 *
 * Returns: %TRUE if this function should be called again to
 *    complete the job, %FALSE if the job is complete (or cancelled)
 **/
typedef xboolean_t (*GIOSchedulerJobFunc) (xio_scheduler_job_t *job,
					 xcancellable_t    *cancellable,
					 xpointer_t         user_data);

/**
 * xsimple_async_thread_func_t:
 * @res: a #xsimple_async_result_t.
 * @object: a #xobject_t.
 * @cancellable: optional #xcancellable_t object, %NULL to ignore.
 *
 * Simple thread function that runs an asynchronous operation and
 * checks for cancellation.
 **/
typedef void (*xsimple_async_thread_func_t) (xsimple_async_result_t *res,
                                        xobject_t *object,
                                        xcancellable_t *cancellable);

/**
 * xsocket_source_func_t:
 * @socket: the #xsocket_t
 * @condition: the current condition at the source fired.
 * @user_data: data passed in by the user.
 *
 * This is the function type of the callback used for the #xsource_t
 * returned by xsocket_create_source().
 *
 * Returns: it should return %FALSE if the source should be removed.
 *
 * Since: 2.22
 */
typedef xboolean_t (*xsocket_source_func_t) (xsocket_t *socket,
				       xio_condition_t condition,
				       xpointer_t user_data);

/**
 * xdatagram_based_source_func_t:
 * @datagram_based: the #xdatagram_based_t
 * @condition: the current condition at the source fired
 * @user_data: data passed in by the user
 *
 * This is the function type of the callback used for the #xsource_t
 * returned by g_datagram_based_create_source().
 *
 * Returns: %G_SOURCE_REMOVE if the source should be removed,
 *   %G_SOURCE_CONTINUE otherwise
 *
 * Since: 2.48
 */
typedef xboolean_t (*xdatagram_based_source_func_t) (xdatagram_based_t *datagram_based,
                                              xio_condition_t    condition,
                                              xpointer_t        user_data);

/**
 * xinput_vector_t:
 * @buffer: Pointer to a buffer where data will be written.
 * @size: the available size in @buffer.
 *
 * Structure used for scatter/gather data input.
 * You generally pass in an array of #GInputVectors
 * and the operation will store the read data starting in the
 * first buffer, switching to the next as needed.
 *
 * Since: 2.22
 */
typedef struct _xinput_vector_t xinput_vector_t;

struct _xinput_vector_t {
  xpointer_t buffer;
  xsize_t size;
};

/**
 * xinput_message_t:
 * @address: (optional) (out) (transfer full): return location
 *   for a #xsocket_address_t, or %NULL
 * @vectors: (array length=num_vectors) (out): pointer to an
 *   array of input vectors
 * @num_vectors: the number of input vectors pointed to by @vectors
 * @bytes_received: (out): will be set to the number of bytes that have been
 *   received
 * @flags: (out): collection of #GSocketMsgFlags for the received message,
 *   outputted by the call
 * @control_messages: (array length=num_control_messages) (optional)
 *   (out) (transfer full): return location for a
 *   caller-allocated array of #GSocketControlMessages, or %NULL
 * @num_control_messages: (out) (optional): return location for the number of
 *   elements in @control_messages
 *
 * Structure used for scatter/gather data input when receiving multiple
 * messages or packets in one go. You generally pass in an array of empty
 * #GInputVectors and the operation will use all the buffers as if they
 * were one buffer, and will set @bytes_received to the total number of bytes
 * received across all #GInputVectors.
 *
 * This structure closely mirrors `struct mmsghdr` and `struct msghdr` from
 * the POSIX sockets API (see `man 2 recvmmsg`).
 *
 * If @address is non-%NULL then it is set to the source address the message
 * was received from, and the caller must free it afterwards.
 *
 * If @control_messages is non-%NULL then it is set to an array of control
 * messages received with the message (if any), and the caller must free it
 * afterwards. @num_control_messages is set to the number of elements in
 * this array, which may be zero.
 *
 * Flags relevant to this message will be returned in @flags. For example,
 * `MSG_EOR` or `MSG_TRUNC`.
 *
 * Since: 2.48
 */
typedef struct _xinput_message_t xinput_message_t;

struct _xinput_message_t {
  xsocket_address_t         **address;

  xinput_vector_t            *vectors;
  xuint_t                    num_vectors;

  xsize_t                    bytes_received;
  xint_t                     flags;

  xsocket_control_message_t ***control_messages;
  xuint_t                   *num_control_messages;
};

/**
 * xoutput_vector_t:
 * @buffer: Pointer to a buffer of data to read.
 * @size: the size of @buffer.
 *
 * Structure used for scatter/gather data output.
 * You generally pass in an array of #GOutputVectors
 * and the operation will use all the buffers as if they were
 * one buffer.
 *
 * Since: 2.22
 */
typedef struct _xoutput_vector xoutput_vector_t;

struct _xoutput_vector {
  xconstpointer buffer;
  xsize_t size;
};

/**
 * xoutput_message_t:
 * @address: (nullable): a #xsocket_address_t, or %NULL
 * @vectors: pointer to an array of output vectors
 * @num_vectors: the number of output vectors pointed to by @vectors.
 * @bytes_sent: initialize to 0. Will be set to the number of bytes
 *     that have been sent
 * @control_messages: (array length=num_control_messages) (nullable): a pointer
 *   to an array of #GSocketControlMessages, or %NULL.
 * @num_control_messages: number of elements in @control_messages.
 *
 * Structure used for scatter/gather data output when sending multiple
 * messages or packets in one go. You generally pass in an array of
 * #GOutputVectors and the operation will use all the buffers as if they
 * were one buffer.
 *
 * If @address is %NULL then the message is sent to the default receiver
 * (as previously set by xsocket_connect()).
 *
 * Since: 2.44
 */
typedef struct _xoutput_message xoutput_message_t;

struct _xoutput_message {
  xsocket_address_t         *address;

  xoutput_vector_t          *vectors;
  xuint_t                   num_vectors;

  xuint_t                   bytes_sent;

  xsocket_control_message_t **control_messages;
  xuint_t                   num_control_messages;
};

typedef struct _GCredentials                  xcredentials_t;
typedef struct _GUnixCredentialsMessage       xunix_credentials_message_t;
typedef struct _GUnixFDList                   xunix_fd_list_t;
typedef struct _GDBusMessage                  xdbus_message_t;
typedef struct _GDBusConnection               xdbus_connection_t;
typedef struct _GDBusProxy                    xdbus_proxy_t;
typedef struct _GDBusMethodInvocation         xdbus_method_invocation_t;
typedef struct _xdbus_server                   xdbus_server_t;
typedef struct _GDBusAuthObserver             xdbus_auth_observer_t;
typedef struct _GDBusErrorEntry               xdbus_error_entry_t;
typedef struct _GDBusInterfaceVTable          xdbus_interface_vtable_t;
typedef struct _GDBusSubtreeVTable            xdbus_subtree_vtable_t;
typedef struct _GDBusAnnotationInfo           xdbus_annotation_info_t;
typedef struct _GDBusArgInfo                  xdbus_arg_info_t;
typedef struct _GDBusMethodInfo               xdbus_method_info_t;
typedef struct _GDBusSignalInfo               xdbus_signalInfo_t;
typedef struct _GDBusPropertyInfo             xdbus_property_info_t;
typedef struct _GDBusInterfaceInfo            xdbus_interface_info_t;
typedef struct _GDBusNodeInfo                 xdbus_node_info_t;

/**
 * xcancellable_source_func_t:
 * @cancellable: the #xcancellable_t
 * @user_data: data passed in by the user.
 *
 * This is the function type of the callback used for the #xsource_t
 * returned by xcancellable_source_new().
 *
 * Returns: it should return %FALSE if the source should be removed.
 *
 * Since: 2.28
 */
typedef xboolean_t (*xcancellable_source_func_t) (xcancellable_t *cancellable,
					    xpointer_t      user_data);

/**
 * xpollable_source_func_t:
 * @pollable_stream: the #xpollable_input_stream_t or #xpollable_output_stream_t
 * @user_data: data passed in by the user.
 *
 * This is the function type of the callback used for the #xsource_t
 * returned by g_pollable_input_stream_create_source() and
 * xpollable_output_stream_create_source().
 *
 * Returns: it should return %FALSE if the source should be removed.
 *
 * Since: 2.28
 */
typedef xboolean_t (*xpollable_source_func_t) (xobject_t  *pollable_stream,
					 xpointer_t  user_data);

typedef struct _GDBusInterface              xdbus_interface_t; /* Dummy typedef */
typedef struct _GDBusInterfaceSkeleton      xdbus_interface_skeleton_t;
typedef struct _GDBusObject                 xdbus_object_t;  /* Dummy typedef */
typedef struct _GDBusObjectSkeleton         xdbus_object_skeleton_t;
typedef struct _GDBusObjectProxy            xdbus_object_proxy_t;
typedef struct _GDBusObjectManager          xdbus_object_manager_t;  /* Dummy typedef */
typedef struct _GDBusObjectManagerClient    xdbus_object_manager_client_t;
typedef struct _GDBusObjectManagerServer    xdbus_object_manager_server_t;

/**
 * xdbus_proxy_type_func_t:
 * @manager: A #xdbus_object_manager_client_t.
 * @object_path: The object path of the remote object.
 * @interface_name: (nullable): The interface name of the remote object or %NULL if a #xdbus_object_proxy_t #xtype_t is requested.
 * @user_data: User data.
 *
 * Function signature for a function used to determine the #xtype_t to
 * use for an interface proxy (if @interface_name is not %NULL) or
 * object proxy (if @interface_name is %NULL).
 *
 * This function is called in the
 * [thread-default main loop][g-main-context-push-thread-default]
 * that @manager was constructed in.
 *
 * Returns: A #xtype_t to use for the remote object. The returned type
 *   must be a #xdbus_proxy_t or #xdbus_object_proxy_t -derived
 *   type.
 *
 * Since: 2.30
 */
typedef xtype_t (*xdbus_proxy_type_func_t) (xdbus_object_manager_client_t   *manager,
                                     const xchar_t                *object_path,
                                     const xchar_t                *interface_name,
                                     xpointer_t                    user_data);

typedef struct _GTestDBus xtest_dbus_t;

/**
 * xsubprocess_t:
 *
 * A child process.
 *
 * Since: 2.40
 */
typedef struct _xsubprocess                   xsubprocess_t;
/**
 * xsubprocess_launcher_t:
 *
 * Options for launching a child process.
 *
 * Since: 2.40
 */
typedef struct _xsubprocess_launcher           xsubprocess_launcher_t;

G_END_DECLS

#endif /* __GIO_TYPES_H__ */
