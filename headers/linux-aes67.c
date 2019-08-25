#include "linux-aes67.h"

void ProcessGError(gchar *ErrorMessage, GError *ErrorStruct)
{
	if(ErrorStruct)
	{
		g_printerr("%s : %s", ErrorMessage, ErrorStruct->message);
		g_error_free(ErrorStruct);
		exit(EXIT_FAILURE);
	}
}
