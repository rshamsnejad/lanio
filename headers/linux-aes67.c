#include "linux-aes67.h"

void processGError(gchar *ErrorMessage, GError *ErrorStruct)
{
	if(ErrorStruct)
	{
		g_printerr("%s : %s\n", ErrorMessage, ErrorStruct->message);
		g_error_free(ErrorStruct);
		exit(EXIT_FAILURE);
	}
}

////////////////////////////////////////////////////////////////////////////////

gchar* getAddressStringFromSocket(GSocketAddress *SocketAddress)
{
	return g_inet_address_to_string
				 (
					 g_inet_socket_address_get_address
					 ((GInetSocketAddress *) SocketAddress)
				 );
}
