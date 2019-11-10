#include <glib.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <stdlib.h>


void activate(GApplication *App)
{
    g_print("I'm activated !\n");
}

void startup(GApplication *App)
{
    g_application_hold(App);
    g_print("I'm started !\n");
}

gboolean printShit(gpointer Data)
{
    gchar *String = (gchar*) Data;
    g_print("%s", String);
    return TRUE;
}

gboolean callback_terminate(gpointer Data)
{
    gint *Signal = (gint*) Data;
    gchar *String = NULL;

    if(*Signal == SIGINT)
        String = "Keyboard Interrupt";
    else if(*Signal == SIGTERM)
        String = "SIGTERM";

    g_printerr("%s. Terminating\n", String);

    // g_free(String);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    GApplication *App;
    gint Status;

    App = g_application_new("io.myapp.test", G_APPLICATION_IS_SERVICE);

    g_signal_connect(App, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(App, "startup", G_CALLBACK(startup), NULL);

    g_timeout_add
    (
        1 * 1000,
        (GSourceFunc) printShit,
        "LOL\n"
    );

    gint SignalInterrupt = SIGINT;
    g_unix_signal_add
    (
        SignalInterrupt,
        (GSourceFunc) callback_terminate,
        &SignalInterrupt
    );
    gint SignalTerminate = SIGTERM;
    g_unix_signal_add
    (
        SignalTerminate,
        (GSourceFunc) callback_terminate,
        &SignalTerminate
    );

    Status = g_application_run(G_APPLICATION(App), argc, argv);
    g_object_unref(App);

    return Status;
}
